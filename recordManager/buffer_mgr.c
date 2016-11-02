//
// Created by Harshavardhan Patil on 10/26/16.
//
// This file has implementation of all the interfaces mentioned in buffer_mgr.h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"

#define MAX_FRAMES 100
#define MAX_PAGES 10000


// convenience macros
#define MAKE_BUFFER_MANAGER_INFO()					\
  ((BufferManagerInfo *) malloc (sizeof(BufferManagerInfo)))

#define MAKE_FRAME_NODE()					\
  ((FrameNode *) malloc (sizeof(FrameNode)))

#define MAKE_PAGE_DATA()					\
   calloc(PAGE_SIZE, sizeof(SM_PageHandle))

#define CHECK_BUFFER_VALIDITY(bm) \
do{ \
  if(bm == NULL || bm->pageFile == NULL ||  bm->numPages == 0){   \
   return  RC_BM_INVALID;   \
  } \
}while(0);

#define CHECK_BUFFER_AND_PAGE_VALIDITY(bm, page) \
do{ \
  if(bm == NULL || bm->pageFile == NULL ||  bm->numPages == 0 || page == NULL || (page->pageNum)<0){   \
   return  RC_BM_INVALID;   \
  } \
}while(0);

#define CHECK_BUFFER_AND_STRATEGY_VALIDITY(bm, pageFileName, numPages, strategy) \
do{ \
  if(bm == NULL || pageFileName == NULL || numPages <= 0 || strategy == -1){   \
   return  RC_BM_INVALID;   \
  } \
}while(0);

/* FrameNode stores data of one page (frame) of buffer pool*/
typedef struct FrameNode{
    int referenceBit; //Reference bit for CLOCK.
    int pageNumber; //pagenumber stored at this frame
    int frameNumber; //frameNumber in the buffer pool.
    bool dirty; //true if the page at this frame has been marked dirty,false otherwise.
    int fixCount; //The total number of pins to the page stored in this frame.
    char *data; //The char pointer to the data stored in this frame, corresponding to the pageNum.
    struct FrameNode *next; //Pointer to the next FrameNode
    struct FrameNode *previous; //Pointer to the previous FrameNode.

}FrameNode;

/* Additional data which will be required per buffer pool. Will be attached to BM_BufferPool->mgmtData*/
typedef struct BufferManagerInfo{
    int numOfDiskReads; //Number of disk read operations happened for this buffer pool.
    int numOfDiskWrites; //Number of disk writes operations happened for this buffer pool.
    int framesFilled; // Total number of frames present in pool
    int frameToPageId[MAX_FRAMES]; //Array of size MAX_FRAMES, at each index it stores the pageId stored at that frame index.
    bool dirtyFlagPerFrame[MAX_FRAMES]; //Array of size MAX_FRAMES, at each index it stores the dirty flag corresponding to page stored at that frame index.
    int pinCountsPerFrame[MAX_FRAMES]; //Array of size MAX_FRAMES, at each index it stores the pin count corresponding to the page stored at that frame index.
    int pageIdToFrameIndex[MAX_PAGES]; //Array of size MAX_PAGES, at each index it stores the FRAME ID in which the corresponding page is stored.
    FrameNode *headFrameNode; //Pointer to the head frameNode.
    FrameNode *tailFrameNode; //Pointer to the tail frameNode.
    FrameNode *referencedNode; //Referenced node for clock.

}BufferManagerInfo;

FrameNode *findFrameNodeByPageNum(FrameNode *pNode, PageNumber num);

/* To create a new frame node.*/
FrameNode *newNode(int i){

    FrameNode *node = MAKE_FRAME_NODE();
    node->pageNumber = NO_PAGE;
    node->frameNumber = i;
    node->dirty = 0;
    node->fixCount = 0;
    node->referenceBit = 0;
    node->data =  MAKE_PAGE_DATA();
    node->next = NULL;
    node->previous = NULL;
    return node;
}

/**
 * This method initialises the buffer pool.
 * 1) It checks if the input parameters passed is valid.
 * 2) If the pagefile input corresponds to a valid file.
 * 3) It initialises BM_BufferPool and BufferManagerInfo object properties to it's default values.
 * 4) It creates frameNodes equal to page size input.
 * 5) It closes the page File.
 * 6) It returnc RC_OK if buffer pool was successfully initialised.
 * @param bm
 * @param pageFileName
 * @param numPages
 * @param strategy
 * @param stratData
 * @return
 */
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData){
    SM_FileHandle fileHandle;
    int status;

    CHECK_BUFFER_AND_STRATEGY_VALIDITY(bm, pageFileName, numPages, strategy);

    if((status = openPageFile ((char *)pageFileName, &fileHandle)) != RC_OK){
        return status;
    }
    bm->numPages = numPages;
    bm->pageFile = pageFileName;
    bm->strategy = strategy;

    BufferManagerInfo *bmInfo = MAKE_BUFFER_MANAGER_INFO();
    bmInfo->numOfDiskReads = 0;
    bmInfo->numOfDiskWrites=0;

    memset(bmInfo->frameToPageId,NO_PAGE,MAX_FRAMES*sizeof(int));
    memset(bmInfo->dirtyFlagPerFrame,false,MAX_FRAMES*sizeof(bool));
    memset(bmInfo->pinCountsPerFrame,0,MAX_FRAMES*sizeof(int));
    memset(bmInfo->pageIdToFrameIndex,NO_PAGE,MAX_PAGES*sizeof(int));

    bmInfo->referencedNode = bmInfo->headFrameNode = bmInfo->tailFrameNode = newNode(0);
    int i;
    for(i=1; i<numPages; i++){
        FrameNode *temp = newNode(i);
        bmInfo->tailFrameNode->next = temp;
        temp->previous = bmInfo->tailFrameNode;
        bmInfo->tailFrameNode = temp;
    }

    bm->mgmtData = bmInfo;
    closePageFile(&fileHandle);
    return RC_OK;
}

/**
 * This method shuts down the buffer pool and releases all resources acquired.
 * 1) It checks if the input parameters passed is valid.
 * 2) It checks if there are any pinned page.
 * 3) If both the above condition passes, It will writes all dirty pages back to the disk.
 * 4) It free's the memory occupied by all FrameNodes corresponding to this buffer pool.
 * 5) It free's the buffer manager info.
 *
 * If the buffer pool has been shut down properly, it will return RC_OK.
 * Returns RC_OK
 * @param bm
 * @return
 */
RC shutdownBufferPool(BM_BufferPool *const bm){

    CHECK_BUFFER_VALIDITY(bm);

    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;

    int i;
    for(i=0; i<bm->numPages; i++){
        if(bufferManagerInfo->pinCountsPerFrame[i] > 0){
            return RC_BM_POOL_IN_USE;
        }
    }

    RC forceFlushPoolStatus = forceFlushPool(bm);
    if(forceFlushPoolStatus != RC_OK){
        return forceFlushPoolStatus;
    }

    FrameNode *current = bufferManagerInfo->headFrameNode;
    FrameNode *temp;
    while (current != NULL){
        temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
    bufferManagerInfo->headFrameNode = NULL;
    bufferManagerInfo->tailFrameNode = NULL;
    bufferManagerInfo->referencedNode = NULL;
    free(bufferManagerInfo);
    bm->numPages = 0;
    bm->mgmtData = NULL;
    return RC_OK;
}

/**
 * This mthod writes al dirty pages in the pool back to disk.
 * 1) It checks for the validity of the input parameters.
 * 2) It opens the page file.
 * 3) It iterates through the frame nodes, checking for dirty frames.
 * 4) When a frame is dirty, it will write the contents back to disk.
 * 5) Marks the frame as non dirty.
 * 6) Marks it non dirty in the array dirtyFlagPerFrame.
 * 7) Once all the dirty frames are written, it closes the page file.
 *
 * Returns RC_OK on success.
 * @param bm
 * @return
 */
RC forceFlushPool(BM_BufferPool *const bm){

    CHECK_BUFFER_VALIDITY(bm);

    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;
    FrameNode *current = bufferManagerInfo->headFrameNode;
    SM_FileHandle fileHandle;
    int status;
    if ((status=openPageFile ((char *)(bm->pageFile), &fileHandle)) != RC_OK){
        return status;
    }
    while (current != NULL) {
        if (current->dirty == true && current->fixCount==0) {
            if((status=writeBlock(current->pageNumber, &fileHandle, current->data)) != RC_OK){
                return status;
            }
            current->dirty = false;
            bufferManagerInfo->numOfDiskWrites++;
            bufferManagerInfo->dirtyFlagPerFrame[current->frameNumber]=false;
        }
        current = current->next;
    }
    closePageFile(&fileHandle);
    return RC_OK;
}

/**
 * This method marks the given page as dirty.
 * 1)It checks for validity of the inputs.
 * 2)It loops through the buffer pool, searching for the frame storing this page.
 * 3) If page isnot found it returns an error.
 * 4) Else it marks the FrameNode storing this page as dirty.
 * 5) It updates the "dirtyFlagPerFrame" array marking the frame corresponding to the page as dirty.
 *
 * Returns RC_OK on success.
 * @param bm
 * @param page
 * @return
 */
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){

    CHECK_BUFFER_AND_PAGE_VALIDITY(bm, page);

    BufferManagerInfo* bmInfo = bm->mgmtData;
    FrameNode* frameNodeToMarkDirty;
    if((frameNodeToMarkDirty=findFrameNodeByPageNum(bmInfo->headFrameNode,page->pageNum))==NULL){
        return RC_BM_INVALID_PAGE;
    }

    frameNodeToMarkDirty->dirty=true;
    bmInfo->dirtyFlagPerFrame[frameNodeToMarkDirty->frameNumber]=true;
    return RC_OK;
}



/**
 * Finds the frame corresponding to the page number passed.
 * @param currentNode
 * @param num
 * @return
 */
FrameNode *findFrameNodeByPageNum(FrameNode *currentNode, PageNumber num) {
    while(currentNode!=NULL){
        if(currentNode->pageNumber==num){
            return currentNode;
        }
        currentNode = currentNode->next;
    }
    return NULL;
}

/**
 * This method unpins the page.
 * 1) It checks the input for validity.
 * 2) It finds the FrameNode corresponding to the page passed.
 * 3) It checks if the fixcount is greater than 0, if yes decerements the fix count. It will update the array pinCountsPerFrame with the updated fix count.
 * 4) Else it will return an error message RC_BM_INVALID_UNPIN.
 *
 * Returns RC_OK on success.
 * @param bm
 * @param page
 * @return
 */
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){

    CHECK_BUFFER_AND_PAGE_VALIDITY(bm, page);
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;
    FrameNode *frameNodeToUnpin;

    if((frameNodeToUnpin=findFrameNodeByPageNum(bufferManagerInfo->headFrameNode,page->pageNum))==NULL){
        return RC_BM_INVALID_PAGE;
    }

    if(frameNodeToUnpin->fixCount > 0){
        frameNodeToUnpin->fixCount--;
        bufferManagerInfo->pinCountsPerFrame[frameNodeToUnpin->frameNumber] =  frameNodeToUnpin->fixCount;
    } else{
        return RC_BM_INVALID_UNPIN;
    }

    return RC_OK;
}

/**
 * This method will write the contents of the page back to disk.
 * 1) It will find the frame node corresponding to the page number.
 * 2) If the page is not found in the frame node list, it will return an error "RC_BM_INVALID_PAGE".
 * 3) Else it will open the pageFile.
 * 4) It writes the data of that page back to disk.
 * 5) Increment numOfDiskWrites.
 * 6) If the fic count is zero, set dirty flag back to false.
 * 7) Closes the page file.
 *
 * Returns RC_OK on success.
 * @param bm
 * @param page
 * @return
 */
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page) {

    CHECK_BUFFER_AND_PAGE_VALIDITY(bm, page);

    BufferManagerInfo *bmInfo = bm->mgmtData;
    FrameNode *frameToFlush;
    SM_FileHandle fileHandle;
    int status;

    if ((frameToFlush = findFrameNodeByPageNum(bmInfo->headFrameNode, page->pageNum)) == NULL) {
        return RC_BM_INVALID_PAGE;
    }

    if ((status = openPageFile(bm->pageFile, &fileHandle)) != RC_OK) {
        return status;
    }
    if ((status = writeBlock(frameToFlush->pageNumber, &fileHandle, frameToFlush->data)) != RC_OK) {
        return status;
    }
    (bmInfo->numOfDiskWrites)++;
    if (frameToFlush->fixCount == 0) {
        frameToFlush->dirty = false;
        bmInfo->dirtyFlagPerFrame[frameToFlush->frameNumber] = false;
    }
    closePageFile(&fileHandle);

    return  RC_OK;
}

/**
 * This method pins page when the replacement strategy is FIFO.
 *
 * 1) It checks if the page is existing in the buffer pool.
 * 2) If yes, it returns this frame increasing the pin count.
 * 3) Else it will check if there is an empty frame in the pool which can be used.
 * 4) If yes, it will use that frame node, read data from disk into that frame. Move that frame to the head of the list, and increment the fix count.
 * 5) Else it will check for a frame to replace. It will navigate from the tail and keep iterating till it finds a frame which has a fix count of zero.
 *    It will then write the contents of this frame back to disk if it is dirty, and read the contents of the page to be returned from the disk. It will
 *    then increment the fix count.
 * @param bm
 * @param page
 * @param num
 * @return
 */
int pinPageFIFO(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber num) {

    FrameNode *findFrameNode;
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *) bm->mgmtData;

    SM_FileHandle fileHandle;
    int status;

    if((findFrameNode = findFrameNodeByPageNum(bufferManagerInfo->headFrameNode, num)) != NULL){
        findFrameNode->fixCount++;
        bufferManagerInfo->pinCountsPerFrame[findFrameNode->frameNumber] = findFrameNode->fixCount;
        page->pageNum = num;
        page->data = findFrameNode->data;
        return RC_OK;
    }
    else if(bufferManagerInfo->framesFilled < bm->numPages){

        FrameNode *current =  bufferManagerInfo->headFrameNode;
        int frameNumber=0;
        while(current != NULL && current->pageNumber != NO_PAGE){
            current = current->next;
            frameNumber++;
        }
        if((status = openPageFile(bm->pageFile, &fileHandle)) != RC_OK){
            return status;
        }
        if((status = ensureCapacity(num+1, &fileHandle)) != RC_OK){
            return status;
        }
        if((status = readBlock(num, &fileHandle, current->data)) != RC_OK){
            return status;
        }
        if(current==bufferManagerInfo->tailFrameNode){

            bufferManagerInfo->tailFrameNode = current->previous;
            bufferManagerInfo->tailFrameNode->next = NULL;
            FrameNode *temp = bufferManagerInfo->headFrameNode;
            current->previous = NULL;
            current->next = bufferManagerInfo->headFrameNode;
            bufferManagerInfo->headFrameNode = current;
            temp->previous = current;
        }else if(current!=bufferManagerInfo->headFrameNode){

            current->next->previous = current->previous;
            current->previous->next = current->next;
            FrameNode *temp = bufferManagerInfo->headFrameNode;
            current->previous = NULL;
            current->next = bufferManagerInfo->headFrameNode;
            bufferManagerInfo->headFrameNode = current;
            temp->previous = current;
        }
        current->fixCount = 1;
        current->dirty=false;
        current->pageNumber=num;
        bufferManagerInfo->framesFilled++;
        bufferManagerInfo->numOfDiskReads++;

        bufferManagerInfo->pinCountsPerFrame[current->frameNumber] = current->fixCount;
        bufferManagerInfo->dirtyFlagPerFrame[current->frameNumber] = false;
        bufferManagerInfo->frameToPageId[current->frameNumber] = num;
        bufferManagerInfo->pageIdToFrameIndex[num] = current->frameNumber;

        page->pageNum = num;
        page->data = current->data;
        closePageFile(&fileHandle);
        return RC_OK;
    }
    else{
        int frameToBeReplaced = -1;
        FrameNode *temp = bufferManagerInfo->tailFrameNode;
        while(temp!=NULL){
            if(temp->fixCount == 0){
                frameToBeReplaced = temp->frameNumber;
                break;
            }
            temp = temp->previous;
        }
        if(frameToBeReplaced == -1){
            return RC_BM_TOO_MANY_CONNECTIONS;
        }

        if((status = openPageFile(bm->pageFile, &fileHandle)) != RC_OK){
            return status;
        }
        if(temp->dirty==true){
            if ((status = writeBlock(temp->pageNumber, &fileHandle, temp->data)) != RC_OK) {
                return status;
            }
            bufferManagerInfo->numOfDiskWrites++;
            temp->dirty=false;
            bufferManagerInfo->dirtyFlagPerFrame[temp->frameNumber] = false;
        }
        if((status = ensureCapacity(num+1, &fileHandle)) != RC_OK){
            return status;
        }
        if((status = readBlock(num, &fileHandle, temp->data)) != RC_OK){
            return status;
        }

        if(temp==bufferManagerInfo->tailFrameNode){
            bufferManagerInfo->tailFrameNode = temp->previous;
            bufferManagerInfo->tailFrameNode->next = NULL;
        }else {
            temp->next->previous = temp->previous;
            temp->previous->next = temp->next;
        }

        temp->previous = NULL;
        temp->next = bufferManagerInfo->headFrameNode;
        bufferManagerInfo->headFrameNode->previous=temp;
        bufferManagerInfo->headFrameNode = temp;

        temp->fixCount = 1;
        temp->pageNumber=num;
        bufferManagerInfo->pinCountsPerFrame[temp->frameNumber] = temp->fixCount;
        bufferManagerInfo->frameToPageId[temp->frameNumber] = num;
        bufferManagerInfo->numOfDiskReads++;
        bufferManagerInfo->pageIdToFrameIndex[num] = temp->frameNumber;

        page->pageNum = num;
        page->data = temp->data;
        closePageFile(&fileHandle);

        return RC_OK;
    }
}


FrameNode *pageInMemory(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum){

    FrameNode *findFrameNode;
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;

    if((findFrameNode = findFrameNodeByPageNum(bufferManagerInfo->headFrameNode, pageNum)) == NULL){
        return NULL;
    }
    page->pageNum = pageNum;
    page->data = findFrameNode->data;
    findFrameNode->fixCount++;
    bufferManagerInfo->pinCountsPerFrame[findFrameNode->frameNumber] = findFrameNode->fixCount;
    return findFrameNode;
}

void moveFrameToHead(BufferManagerInfo* bufferManagerInfo, FrameNode *updateNode){
    FrameNode* headNode = bufferManagerInfo->headFrameNode;
    FrameNode* tailNode = bufferManagerInfo->tailFrameNode;
    if(updateNode == headNode || headNode == NULL){
        return;
    }
    else if(updateNode == tailNode){
        FrameNode *temp = tailNode->previous;
        temp->next = NULL;
        bufferManagerInfo->tailFrameNode = temp;
    }
    else{
        updateNode->previous->next = updateNode->next;
        updateNode->next->previous = updateNode->previous;
    }

    updateNode->next = headNode;
    headNode->previous = updateNode;
    updateNode->previous = NULL;

    bufferManagerInfo->headFrameNode = updateNode;
    return;
}

RC updateFrameContentsWithCorrectDataOnReplace(BM_BufferPool *const bm, FrameNode *found, BM_PageHandle *const page, const PageNumber pageNum){

    SM_FileHandle fh;
    BufferManagerInfo *info = (BufferManagerInfo *)bm->mgmtData;
    RC status;

    if ((status = openPageFile ((char *)(bm->pageFile), &fh)) != RC_OK){
        return status;
    }

    if(found->dirty == 1){

        if((status = writeBlock(found->pageNumber,&fh, found->data)) != RC_OK){
            return status;
        }
        (info->numOfDiskWrites)++;
    }

    if((status = ensureCapacity(pageNum+1, &fh)) != RC_OK){
        return status;
    }

    if((status = readBlock(pageNum, &fh, found->data)) != RC_OK){
        return status;
    }
    info->numOfDiskReads++;

    closePageFile(&fh);

    return RC_OK;

}

RC updateFrameContentsWithCorrectDataOnUsingEmptyFrame(BM_BufferPool *const bm, FrameNode *found, BM_PageHandle *const page, const PageNumber pageNum){

    SM_FileHandle fh;
    BufferManagerInfo *info = (BufferManagerInfo *)bm->mgmtData;
    RC status;

    if ((status = openPageFile ((char *)(bm->pageFile), &fh)) != RC_OK){
        return status;
    }

    if((status = ensureCapacity(pageNum+1, &fh)) != RC_OK){
        return status;
    }

    if((status = readBlock(pageNum, &fh, found->data)) != RC_OK){
        return status;
    }
    info->numOfDiskReads++;

    closePageFile(&fh);

    return RC_OK;

}

void updateStats(FrameNode *temp,BufferManagerInfo *bufferManagerInfo,BM_PageHandle *const page,const PageNumber num) {
    page->pageNum = num;
    page->data = temp->data;

    temp->fixCount = 1;
    temp->dirty=false;
    temp->pageNumber=num;

    bufferManagerInfo->pinCountsPerFrame[temp->frameNumber] = temp->fixCount;
    bufferManagerInfo->dirtyFlagPerFrame[temp->frameNumber] = false;
    bufferManagerInfo->frameToPageId[temp->frameNumber] = num;
    bufferManagerInfo->pageIdToFrameIndex[num] = temp->frameNumber;


}

/**
 * This method pins page when the replacement strategy is LRU.
 *
 * 1) It checks if the page is existing in the buffer pool.
 * 2) If yes, it returns this frame increasing the pin count. And also moves this frame to the head of the list.
 * 3) Else it will check if there is an empty frame in the pool which can be used.
 * 4) If yes, it will use that frame node, read data from disk into that frame. Move that frame to the head of the list, and increment the fix count.
 * 5) Else it will check for a frame to replace. It will navigate from the tail and keep iterating till it finds a frame which has a fix count of zero.
 *    It will then write the contents of this frame back to disk if it is dirty, and read the contents of the page to be returned from the disk. It will
 *    then increment the fix count.
 * @param bm
 * @param page
 * @param num
 * @return
 */
int pinPageLRU(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber num) {
    FrameNode *found;
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;

    if((found = pageInMemory(bm, page, num)) != NULL){
        moveFrameToHead(bufferManagerInfo, found);
        return RC_OK;
    }
    if((bufferManagerInfo->framesFilled) < bm->numPages){
        found = bufferManagerInfo->headFrameNode;

        while(found != NULL && found->pageNumber != NO_PAGE){
            found = found->next;
        }
        bufferManagerInfo->framesFilled++;
        RC status;
        if((status = updateFrameContentsWithCorrectDataOnUsingEmptyFrame(bm, found, page, num)) != RC_OK){
            return status;
        }
        updateStats(found,bufferManagerInfo,page,num);
        moveFrameToHead(bufferManagerInfo, found);

    } else{

        found = bufferManagerInfo->tailFrameNode;
        int frameToBeReplaced = -1;
        while(found != NULL ){
            if(found->fixCount == 0){
                frameToBeReplaced = found->frameNumber;
                break;
            }
            found = found->previous;
        }

        /* If reached to head, it means no frames with fixed count 0 available.*/
        if (frameToBeReplaced == -1){
            return RC_BM_TOO_MANY_CONNECTIONS;
        }
        RC status;
        if((status = updateFrameContentsWithCorrectDataOnReplace(bm, found, page, num)) != RC_OK){
            return status;
        }
        updateStats(found,bufferManagerInfo,page,num);
        moveFrameToHead(bufferManagerInfo, found);

    }
    return RC_OK;
}

/**
 * This method pins page when the replacement strategy is CLOCK.
 *
 * 1) It checks if the page is existing in the buffer pool.
 * 2) If yes, it returns this frame increasing the pin count. And sets the reference bit of this node to 1 and marks this node as reference node.
 * 3) Else it will check if there is an empty frame in the pool which can be used.
 * 4) If yes, it will use that frame node, read data from disk into that frame. Increment the fix count. Mark the node as reference node and set reference bit to 1.
 * 5) Else it will check for a frame to replace. It will navigate from the referenceNode and keep iterating till it finds a frame which has a fix count of zero and reference bit of zero.
 *    In the way it marks all other reference bit to 0.
 *    When it finds the replacement node, It will  write the contents of this frame back to disk if it is dirty, and read the contents of the page to be returned from the disk. It will
 *    then increment the fix count. And mark this as reference node and set it's reference bit to 1.
 * @param bm
 * @param page
 * @param num
 * @return
 */
int pinPageCLOCK(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber num) {
    FrameNode *found;
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;
    if((found = pageInMemory(bm, page, num)) != NULL){
        bufferManagerInfo->referencedNode = found;
        found->referenceBit = 1;
        return RC_OK;
    }
    if((bufferManagerInfo->framesFilled) < bm->numPages){
        found = bufferManagerInfo->headFrameNode;

        while(found != NULL && found->pageNumber != NO_PAGE){
            found = found->next;
        }
        bufferManagerInfo->framesFilled++;
        RC status;
        if((status = updateFrameContentsWithCorrectDataOnUsingEmptyFrame(bm, found, page, num)) != RC_OK){
            return status;
        }
        found->referenceBit = 1;
        bufferManagerInfo->referencedNode = found;
        updateStats(found,bufferManagerInfo,page,num);

    } else{
        found = bufferManagerInfo->referencedNode;

        /* retrieve first frame with rf = 0 and set all bits to zero along the way */
        int frameCount=0;
        while (found != NULL &&frameCount<(2*(bm->numPages))){
            if(found->fixCount==0 && found->referenceBit==0){
                found->referenceBit = 1;
                break;
            }else{
                found->referenceBit = 0;
                frameCount++;
                if(found == bufferManagerInfo->tailFrameNode){
                    found=bufferManagerInfo->headFrameNode;
                }else{
                    found = found->next;
                }
            }
        }
        if (found == NULL || frameCount == (2*(bm->numPages))){
            return RC_BM_TOO_MANY_CONNECTIONS;
        }else if(found->referenceBit == 1){
            int status;
            if((status = updateFrameContentsWithCorrectDataOnReplace(bm, found, page, num)) != RC_OK){
                return status;
            }
            bufferManagerInfo->referencedNode = found;
            found->referenceBit = 1;
            updateStats(found,bufferManagerInfo,page,num);

        }

    }
    return RC_OK;
}

/**
 * Implements FIFO and LRU logic
 * @param bm
 * @param page
 * @param pageNum
 * @return
 */
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum){
    CHECK_BUFFER_VALIDITY(bm);
    if(pageNum<0){
        return RC_BM_INVALID_PAGE;
    }
    int status;
    if(bm->strategy==RS_FIFO){
        status = pinPageFIFO(bm,page,pageNum);
    }else if(bm->strategy==RS_LRU){
        status = pinPageLRU(bm,page,pageNum);
    }else if(bm->strategy==RS_CLOCK){
        status = pinPageCLOCK(bm,page,pageNum);
    }else{
        status = RC_BM_UNSUPPORTED_PAGE_STRATEGY;
    }
    return status;
}

/**
 * Returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame.
 * An empty page frame is represented using the constant NO_PAGE
 * @param bm
 * @return
 */
PageNumber *getFrameContents (BM_BufferPool *const bm){
    return ((BufferManagerInfo*)bm->mgmtData)->frameToPageId;
}

/**
 * Returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty.
 * Empty page frames are considered as clean.
 * @param bm
 * @return
 */
bool *getDirtyFlags (BM_BufferPool *const bm){
    return ((BufferManagerInfo*)bm->mgmtData)->dirtyFlagPerFrame;
}

/**
 * Returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame.
 * Return 0 for empty page frames.
 * @param bm
 * @return
 */
int *getFixCounts (BM_BufferPool *const bm){
    return ((BufferManagerInfo*)bm->mgmtData)->pinCountsPerFrame;
}

/**
 * Returns the number of pages that have been read from disk since a buffer pool has been initialized.
 * @param bm
 * @return
 */
int getNumReadIO (BM_BufferPool *const bm)
{
    return ((BufferManagerInfo *)bm->mgmtData)->numOfDiskReads;
}

/**
 * Returns the number of pages written to the page file since the buffer pool has been initialized.
 * @param bm
 * @return
 */
int getNumWriteIO (BM_BufferPool *const bm)
{
    return ((BufferManagerInfo *)bm->mgmtData)->numOfDiskWrites;
}
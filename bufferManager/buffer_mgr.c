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
  ((SM_PageHandle) malloc (sizeof(PAGE_SIZE)))

#define CHECK_BUFFER_VALIDITY(bufferManager) \
do{ \
  if(bm == NULL || bm->pageFile == NULL ||  bm->numPages == 0){   \
   return  RC_BM_INVALID;   \
  } \
}while(0);

#define CHECK_BUFFER_AND_PAGE_VALIDITY(bufferManager, page) \
do{ \
  if(bm == NULL || bm->pageFile == NULL ||  bm->numPages == 0 || page == NULL || page->pageNum<=0){   \
   return  RC_BM_INVALID;   \
  } \
}while(0);

#define CHECK_BUFFER_AND_STRATEGY_VALIDITY(bufferManager, strategy) \
do{ \
  if(bm == NULL || bm->pageFile == NULL || numPages <= 0 || strategy == NULL){   \
   return  RC_BM_INVALID;   \
  } \
}while(0);


/* FrameNode stores data of one page (frame) of buffer pool*/
typedef struct FrameNode{

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
    int frameToPageId[MAX_FRAMES]; //Array of size MAX_FRAMES, at each index it stores the pageId stored at that frame index.
    bool dirtyFlagPerFrame[MAX_FRAMES]; //Array of size MAX_FRAMES, at each index it stores the dirty flag corresponding to page stored at that frame index.
    int pinCountsPerFrame[MAX_FRAMES]; //Array of size MAX_FRAMES, at each index it stores the pin count corresponding to the page stored at that frame index.
    int pageIdToFrameIndex[MAX_PAGES]; //Array of size MAX_PAGES, at each index it stores the FRAME ID in which the corresponding page is stored.
    FrameNode *headFrameNode; //Pointer to the head frameNode.
    FrameNode *tailFrameNode; //Pointer to the tail frameNode.
}BufferManagerInfo;

FrameNode *findFrameNodeByPageNum(FrameNode *pNode, PageNumber num);

/* To create a new frame node.*/
FrameNode *newNode(int i){

    FrameNode *node = MAKE_FRAME_NODE();
    node->pageNumber = NO_PAGE;
    node->frameNumber = i;
    node->dirty = 0;
    node->fixCount = 0;
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

    CHECK_BUFFER_AND_STRATEGY_VALIDITY(bm, strategy);

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

    bmInfo->headFrameNode = bmInfo->tailFrameNode = newNode(0);
    for(int i = 1; i <numPages; i++){
        FrameNode *temp = newNode(i);
        bmInfo->tailFrameNode->next = temp;
        temp->previous =   bmInfo->tailFrameNode;
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
    while (current != NULL){
        current = current->next;
        free(bufferManagerInfo->headFrameNode->data);
        free(bufferManagerInfo->headFrameNode);
        bufferManagerInfo->headFrameNode = current;
    }

    bufferManagerInfo->tailFrameNode = NULL;
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
    SM_FileHandle *fileHandle;
    int status;

    if ((frameToFlush = findFrameNodeByPageNum(bmInfo->headFrameNode, page->pageNum)) == NULL) {
        return RC_BM_INVALID_PAGE;
    }

    if ((status = openPageFile(bm->pageFile, fileHandle)) != RC_OK) {
        return status;
    }
    if ((status = writeBlock(frameToFlush->pageNumber, fileHandle, frameToFlush->data)) != RC_OK) {
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

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum){

    CHECK_BUFFER_AND_PAGE_VALIDITY(bm, page);
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
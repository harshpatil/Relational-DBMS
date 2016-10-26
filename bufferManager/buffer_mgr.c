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

// convenience macros
#define MAKE_BUFFER_MANAGER_INFO()					\
  ((BufferManagerInfo *) malloc (sizeof(BufferManagerInfo)))

#define MAKE_FRAME_NODE()					\
  ((FrameNode *) malloc (sizeof(FrameNode)))

#define MAKE_PAGE_DATA()					\
  ((SM_PageHandle) malloc (sizeof(PAGE_SIZE)))

/* FrameNode stores data of one frame of buffer pool*/
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
    FrameNode *headFrameNode; //Pointer to the head frameNode.
    FrameNode *tailFrameNode; //Pointer to the tail frameNode.
}BufferManagerInfo;

FrameNode *findFrameNodeByPageNum(FrameNode *pNode, PageNumber num);

/* To create a new frame node.*/
FrameNode *newNode(int i){
    FrameNode *node = MAKE_FRAME_NODE;
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
    if(bm == NULL || bm->pageFile == NULL || numPages <= 0 || strategy == NULL){
        return RC_BM_INVALID;
    }
    if ((status = openPageFile ((char *)pageFileName, &fileHandle)) != RC_OK){
        return status;
    }
    bm->numPages = numPages;
    bm->pageFile = pageFileName;
    bm->strategy = strategy;

    BufferManagerInfo *bmInfo = MAKE_BUFFER_MANAGER_INFO();
    bmInfo->numOfDiskReads = 0;
    bmInfo->numOfDiskWrites=0;

    memset(bmInfo->frameToPageId,NO_PAGE,MAX_FRAMES*sizeof(int));
    memset(bmInfo->dirtyFlagPerFrame,NO_PAGE,MAX_FRAMES*sizeof(int));
    memset(bmInfo->pinCountsPerFrame,NO_PAGE,MAX_FRAMES*sizeof(int));

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

    if(bm == NULL || bm->pageFile == NULL || bm->numPages <= 0){
        return RC_BM_INVALID;
    }

    RC forceFlushPoolStatus = forceFlushPool(bm);
    if(forceFlushPoolStatus != RC_OK){
        return forceFlushPoolStatus;
    }

    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;
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
 * 2) 
 * @param bm
 * @return
 */
RC forceFlushPool(BM_BufferPool *const bm){

    if(bm == NULL || bm->pageFile == NULL || bm->numPages <= 0){
        return RC_BM_INVALID;
    }
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;
    FrameNode *current = bufferManagerInfo->headFrameNode;
    SM_FileHandle fileHandle;
    int status;
    while (current != NULL) {
        if (current->dirty == true) {
            if((status=writeBlock(current->pageNumber, &fileHandle, current->data)) != RC_OK){
                return status;
            }
            current->dirty = false;
            bufferManagerInfo->numOfDiskWrites++;
            bufferManagerInfo->dirtyFlagPerFrame[current->frameNumber]=false;
        }
        current = current->next;
    }
    return RC_OK;
}

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
 if( bm == NULL || bm->pageFile == NULL ||  bm->numPages == 0 || page == NULL || page->pageNum<=0) {
     return RC_BM_INVALID;
 }

 BufferManagerInfo* bmInfo = bm->mgmtData;
 FrameNode* frameNodeToMarkDirty;
 if((frameNodeToMarkDirty=findFrameNodeByPageNum(bmInfo->headFrameNode,page->pageNum))==NULL){
    return RC_BM_INVALID_PAGE;
 }
 frameNodeToMarkDirty->dirty=true;
 bmInfo->dirtyFlagPerFrame[frameNodeToMarkDirty->frameNumber]=true;
 return RC_OK;
}

FrameNode *findFrameNodeByPageNum(FrameNode *currentNode, PageNumber num) {
    while(currentNode!=NULL){
        if(currentNode->pageNumber==num){
            return currentNode;
        }
        currentNode = currentNode->next;
    }
    return NULL;
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){
    if( bm == NULL || bm->pageFile == NULL ||  bm->numPages == 0 || page == NULL || page->pageNum<=0) {
        return RC_BM_INVALID;
    }
    BufferManagerInfo* bmInfo = bm->mgmtData;
    FrameNode *frameToFlush;
    SM_FileHandle *fileHandle;
    int status;

    if((frameToFlush=findFrameNodeByPageNum(bmInfo->headFrameNode,page->pageNum))==NULL){
        return RC_BM_INVALID_PAGE;
    }
    if((status=openPageFile(bm->pageFile,fileHandle)) != RC_OK){
        return status;
    }
    if((status=writeBlock(frameToFlush->pageNumber,fileHandle,frameToFlush->data)) != RC_OK){
        return status;
    }
    (bmInfo->numOfDiskWrites)++;
    bmInfo->dirtyFlagPerFrame[frameToFlush->frameNumber]=true;
    closePageFile(&fileHandle);

    return  RC_OK;

}

PageNumber *getFrameContents (BM_BufferPool *const bm){
    return ((BufferManagerInfo*)bm->mgmtData)->frameToPageId;
}

bool *getDirtyFlags (BM_BufferPool *const bm){
    return ((BufferManagerInfo*)bm->mgmtData)->dirtyFlagPerFrame;
}

int *getFixCounts (BM_BufferPool *const bm){
    return ((BufferManagerInfo*)bm->mgmtData)->pinCountsPerFrame;
}

int getNumReadIO (BM_BufferPool *const bm)
{
    return ((BufferManagerInfo *)bm->mgmtData)->numOfDiskReads;
}

int getNumWriteIO (BM_BufferPool *const bm)
{
    return ((BufferManagerInfo *)bm->mgmtData)->numOfDiskWrites;
}
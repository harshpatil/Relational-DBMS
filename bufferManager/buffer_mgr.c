//
// Created by Harshavardhan Patil on 10/26/16.
//
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

/* FrameNode stores data of one page (frame) of buffer pool*/
typedef struct FrameNode{

    int pageNumber;
    int frameNumber;
    bool dirty;
    int fixCount;
    int requestCount;
    char *data;
    struct FrameNode *next;
    struct FrameNode *previous;

}FrameNode;

/* Additional data which will be required per buffer pool. Will be attached to BM_BufferPool->mgmtData*/
typedef struct BufferManagerInfo{
    int numOfDiskReads;
    int numOfDiskWrites;
    int frameToPageId[MAX_FRAMES];
    bool dirtyFlagPerFrame[MAX_FRAMES];
    int pinCountsPerFrame[MAX_FRAMES];
    FrameNode *headFrameNode;
    FrameNode *tailFrameNode;
}BufferManagerInfo;

FrameNode *findFrameNodeByPageNum(FrameNode *pNode, PageNumber num);

/* To create a new node for frame list*/
FrameNode *newNode(int i){

    FrameNode *node = MAKE_FRAME_NODE;
    node->pageNumber = NO_PAGE;
    node->frameNumber = i;
    node->dirty = 0;
    node->fixCount = 0;
    node->requestCount = 0;
    node->data =  MAKE_PAGE_DATA();
    node->next = NULL;
    node->previous = NULL;
    return node;
}

// Buffer Manager Interface Pool Handling
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
}


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
    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm){

    if(bm == NULL || bm->pageFile == NULL || bm->numPages <= 0){
        return RC_BM_INVALID;
    }
    BufferManagerInfo *bufferManagerInfo = (BufferManagerInfo *)bm->mgmtData;
    FrameNode *current = bufferManagerInfo->headFrameNode;
    SM_FileHandle fileHandle;

    while (current != NULL) {
        if (current->dirty == true) {
            if(writeBlock(current->pageNumber, &fileHandle, current->data) != RC_OK){
                return RC_WRITE_FAILED;
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
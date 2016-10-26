//
// Created by Harshavardhan Patil on 10/26/16.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MacTypes.h>
#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"

#define MAX_FRAMES 100

/* FrameNode stores data of one page (frame) of buffer pool*/
typedef struct FrameNode{

    int pageNumber;
    int frameNumber;
    bool dirty;
    int fixCount;
    int pageFrequency;
    char *data;
    struct FrameNode *next;
    struct FrameNode *previous;

}FrameNode;

// convenience macros
#define MAKE_BUFFER_MANAGER_INFO()					\
  ((BufferManagerInfo *) malloc (sizeof(BufferManagerInfo)))


/* Additional data which will be required per buffer pool. Will be attached to BM_BufferPool->mgmtData*/
typedef struct BufferManagerInfo{
    int numOfDiskReads;
    int numOfDiskWrites;
    int frameToPageId[MAX_FRAMES];
    bool dirtyFrames[MAX_FRAMES];
    int pinCountsPerFrame[MAX_FRAMES];
    FrameNode *headFrameNode;
    FrameNode *tailFrameNode;
}BufferManagerInfo;

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData){
    SM_FileHandle fileHandle;
    int status;
    if(bm->pageFile==NULL || numPages <= 0){
        return RC_BM_INVALID_PAGE;
    }
    if ((status = openPageFile ((char *)pageFileName, &fileHandle)) != RC_OK){
        return status;
    }
    BufferManagerInfo *bmInfo = MAKE_BUFFER_MANAGER_INFO();

}


RC shutdownBufferPool(BM_BufferPool *const bm){

    if(bm->numPages <= 0){
        return RC_INVALID_POOL;
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

    if(bm->numPages <= 0){
        return RC_INVALID_POOL;
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
        }
        current = current->next;
    }
    return RC_OK;
}
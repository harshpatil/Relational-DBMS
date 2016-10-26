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

typedef struct FrameNode{

    int pageNumber;
    int frameNumber;
    int dirty;
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
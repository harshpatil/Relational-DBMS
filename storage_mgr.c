//
// Created by Harshavardhan Patil on 9/23/16.
//
// This file has implementation of all the interfaces mentioned in storage_mgr.h
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "storage_mgr.h"
#include "dberror.h"


void initStorageManager(){

}

/**
 * This method checks if the file already exists, if yes it returns an error message.
 * Else, it creates a page file of size 1 where the entire page is filled with '\0' bytes.
 * It reserves the first page for storing the file page count, this is initialised to 1.
 * @param fileName
 * @return
 */
RC createPageFile (char *fileName){
    if(fopen(fileName,"r") != NULL){
        return RC_FILE_ALREADY_EXISTS;
    }else{
        FILE *filePointer = fopen(fileName, "w");
        char *totalPages, *firstPage;
        totalPages = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
        firstPage = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
        strcpy(totalPages,"1\n");
        fwrite(totalPages, PAGE_ELEMENT_SIZE, PAGE_SIZE, filePointer);
        fwrite(firstPage, PAGE_ELEMENT_SIZE, PAGE_SIZE, filePointer);
        free(totalPages);
        free(firstPage);
        fclose(filePointer);
        return RC_OK;
    }
}


RC openPageFile (char *fileName, SM_FileHandle *fHandle){
    return NULL;
}

RC closePageFile (SM_FileHandle *fHandle){
    return NULL;
}

RC destroyPageFile (char *fileName){
    return NULL;
}

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

int getBlockPos (SM_FileHandle *fHandle){
    return NULL;
}

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
  return NULL;
}


RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return NULL;
}

RC appendEmptyBlock (SM_FileHandle *fHandle){
    return NULL;
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
    return NULL;
}


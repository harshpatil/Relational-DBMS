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
 * This method create a page file with name "fileName"
 *
 * @param fileName
 * @return
 */
RC createPageFile (char *fileName){

    if(fopen(fileName,"r") != NULL)
    {
        return RC_FILE_ALREADY_EXISTS;
    }
    else
    {
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

    if(fopen(fileName,"r") == NULL)
    {
        printf("File does not exist !!");
        return RC_FILE_NOT_FOUND;
    }
    else
    {
        FILE *filePointer = fopen(fileName, "r+");
        char *readPage;
        readPage = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
        fgets(readPage, PAGE_SIZE, filePointer);
        puts(readPage);
        readPage = strtok(readPage, "\n");
        fHandle->fileName = fileName;
        fHandle->totalNumPages = atoi(readPage);
        fHandle->curPagePos = 0;
        fHandle->mgmtInfo = filePointer;

        free(readPage);
        fclose(filePointer);
        return RC_OK;
    }
}

RC closePageFile (SM_FileHandle *fHandle){
    return NULL;
}

RC destroyPageFile (char *fileName){

    if(fopen(fileName,"r") == NULL)
    {
        printf("File does not exist !!");
        return RC_FILE_NOT_FOUND;
    }
    else
    {
        printf("File exist so deleting!!");
        remove(fileName);
        return RC_OK;
    }
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


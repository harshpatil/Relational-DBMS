//
// Created by Harshavardhan Patil on 9/23/16.
//
// This file has implementation of all the interfaces mentioned in storage_mgr.h
#include <stdio.h>
#include <stdlib.h>
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
    if(fopen(fileName,"r") != NULL){
        return RC_FILE_ALREADY_EXISTS;
    }else{
        FILE *filePointer = fopen(fileName, "w");
        char *totalPages, *firstPage;
        printf("%d  first \n",totalPages);
        totalPages = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
        printf("%d second \n",totalPages);

        firstPage = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
      //  totalPages = "1\n";
        printf("%d %s third \n",totalPages,*totalPages);

        fwrite(totalPages, PAGE_ELEMENT_SIZE, PAGE_SIZE, filePointer);
        fwrite(firstPage, PAGE_ELEMENT_SIZE, PAGE_SIZE, filePointer);
        printf("%d fourth \n",totalPages);

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


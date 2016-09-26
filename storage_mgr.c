/*
 * Created by Harshavardhan Patil on 9/23/16.
 */
// This file has implementation of all the interfaces mentioned in storage_mgr.h
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "storage_mgr.h"
#include "dberror.h"

#define CHECK_FILE_VALIDITY(fileHandle)  \
do{ \
  if(fileHandle == NULL || fileHandle->mgmtInfo == NULL){   \
   return  RC_FILE_HANDLE_NOT_INIT;   \
  } \
}while(0);

void initStorageManager(){

}

/**
 * This method checks if the file already exists, if yes it returns an error message.
 * Else, it creates a page file of size 1 where the entire page is filled with '\0' bytes.
 * It reserves the first page for storing the file page count, this is initialised to 1.
 *
 * If the file is successfully created it returns RC_OK,
 * else it returns RC_FILE_ALREADY_EXISTS.
 * @param fileName
 * @return
 */
RC createPageFile (char *fileName){

    if(fopen(fileName,"r") != NULL){
        return RC_FILE_ALREADY_EXISTS;
    }
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


RC openPageFile (char *fileName, SM_FileHandle *fileHandle){

    if(fopen(fileName,"r") == NULL) {
        printf("File does not exist !!");
        return RC_FILE_NOT_FOUND;
    }
    FILE *filePointer = fopen(fileName, "r+");
    char *readPage;
    readPage = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
    fgets(readPage, PAGE_SIZE, filePointer);
    readPage = strtok(readPage, "\n");
    fileHandle->fileName = fileName;
    fileHandle->totalNumPages = atoi(readPage);
    fileHandle->curPagePos = 0;
    fileHandle->mgmtInfo = filePointer;
    free(readPage);
    return RC_OK;
}

/**
 * This method closes the file specified in the FileHandle.
 * If the file is successfully closed it returns RC_OK,
 * else it returns RC_FILE_NOT_FOUND.
 * @param fileHandle
 * @return
 */
RC closePageFile (SM_FileHandle *fileHandle){
    if(!fclose(fileHandle->mgmtInfo)){
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

RC destroyPageFile (char *fileName){

    if(!remove(fileName)){
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

/*
 *This function reads a page numbered with pageNum into memPage.
 *It first checks if the page number is valid or not.
 *If valid, it checks if the file pointer is available or not.
 *With valid file pointer, it reads the given page and current page position is increased.
 */
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


RC writeBlockData(int pageNum,SM_FileHandle *fileHandle, SM_PageHandle memPage) {
    if(fwrite(memPage, PAGE_ELEMENT_SIZE, PAGE_SIZE, fileHandle->mgmtInfo) != PAGE_SIZE*PAGE_ELEMENT_SIZE){
        return RC_WRITE_FAILED;
    }
    fileHandle -> curPagePos = pageNum;
    fflush(fileHandle ->mgmtInfo);
    return RC_OK;
}


RC writeBlock (int pageNum, SM_FileHandle *fileHandle, SM_PageHandle memPage){
    CHECK_FILE_VALIDITY(fileHandle);
    if(pageNum > fileHandle -> totalNumPages || pageNum < 0){
        return RC_WRITE_FAILED;
    }
    if(pageNum == fileHandle -> totalNumPages){
        RC retVal = appendEmptyBlock(fileHandle);
        if(retVal != RC_OK){
            return retVal;
        }
    }
    long curFilePos = ftell(fileHandle -> mgmtInfo);
    long newFilePos = (pageNum+1)*PAGE_SIZE*PAGE_ELEMENT_SIZE;
    if( curFilePos == newFilePos){
       return writeBlockData(pageNum,fileHandle,memPage);
    }
    if(fseek(fileHandle -> mgmtInfo,newFilePos,SEEK_SET)==0){
       return writeBlockData(pageNum,fileHandle,memPage);
    }
    return RC_WRITE_FAILED;
}

RC writeCurrentBlock (SM_FileHandle *fileHandle, SM_PageHandle memPage){
    return writeBlock(fileHandle -> curPagePos,fileHandle,memPage);
}

RC appendEmptyBlock (SM_FileHandle *fileHandle){

    CHECK_FILE_VALIDITY(fileHandle);

    int newPagePos = fileHandle->totalNumPages +1;
    if(fseek(fileHandle->mgmtInfo,newPagePos*PAGE_SIZE*PAGE_ELEMENT_SIZE ,SEEK_SET)==0){

        SM_PageHandle pageHandle = (char *) calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
        writeBlockData(fileHandle->totalNumPages,fileHandle,pageHandle);
        fileHandle->totalNumPages = fileHandle->totalNumPages + 1;
        rewind(fileHandle->mgmtInfo);
        fprintf(fileHandle->mgmtInfo,"%d\n",fileHandle->totalNumPages);
        fseek(fileHandle->mgmtInfo,(fileHandle->totalNumPages)*PAGE_SIZE*PAGE_ELEMENT_SIZE ,SEEK_SET);
        fflush(fileHandle->mgmtInfo);
        free(pageHandle);
        return RC_OK;
    }
    return RC_WRITE_FAILED;
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fileHandle){

    CHECK_FILE_VALIDITY(fileHandle);
    if(fileHandle->totalNumPages >= numberOfPages){
        return RC_OK;
    }
    int pagesToBeAdded = numberOfPages - fileHandle->totalNumPages;
    for(int i=0; i<pagesToBeAdded; i++){
       RC ret_val = appendEmptyBlock(fileHandle);
        if(ret_val != RC_OK){
            return ret_val;
        }
    }
    return RC_OK;
}


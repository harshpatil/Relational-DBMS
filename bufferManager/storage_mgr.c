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

/**
 * This method opens the file in read write mode if the file exists else throws an error.
 * It also initiliases SM_FileHandle with details such as totalNumPages, curPagePos, mgmtInfo and fileName of the
 * file passed
 *
 * This methoD returns RC_OK on success and RC_FILE_NOT_FOUND on failure.
 * @param fileName
 * @param fileHandle
 * @return
 */
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
 * It also de-initialises SM_FileHandle object.
 *
 * If the file is successfully closed it returns RC_OK,
 * else it returns RC_FILE_NOT_FOUND.
 * @param fileHandle
 * @return
 */
RC closePageFile (SM_FileHandle *fileHandle){
    if(!fclose(fileHandle->mgmtInfo)){
        fileHandle->totalNumPages=0;
        fileHandle->curPagePos=0;
        fileHandle->fileName=NULL;
        fileHandle->mgmtInfo=NULL;
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

/**
 * This method removes the file specified from the system.
 *
 * If the file is successfully removed it returns RC_OK,
 * else it returns RC_FILE_NOT_FOUND.
 * @param fileName
 * @return
 */
RC destroyPageFile (char *fileName){

    if(!remove(fileName)){
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}


/**
 * This method reads a block of data from the file at 'pageNum' position and writes to "memPage".
 * 1) Checks for validity of file
 * 2) Checks for validity of page number.
 * 3) It moves the internal file pointer to the position from where to read using fseek.
 * 4) If seek fails it returns an error.
 * 5) Else it reads 4096 elements (element size 1 byte) from the file using fread.
 * 6) If fread successfully reads 4096 bytes it will update curPagePos to 'pageNum' and return success.
 * 7) Else it will fail with an error.
 *
 * It returns RC_OK on success. It can fail with RC_FILE_HANDLE_NOT_INIT(invalid file), RC_READ_NON_EXISTING_PAGE ( invalid page number)
 * and RC_READ_FAILED ( failed fseek or fread operation ).
 * @param pageNum
 * @param fileHandle
 * @param memPage
 * @return
 */
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    CHECK_FILE_VALIDITY(fHandle);
    if(fHandle->totalNumPages <= pageNum || pageNum < -1)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    if(fseek(fHandle->mgmtInfo,(pageNum+1)*PAGE_SIZE,SEEK_SET)!=0){  //moves pointer to pageNum block of file pointed to by memPage page
        return RC_READ_FAILED;
    }

    int elementsRead =fread(memPage,PAGE_ELEMENT_SIZE,PAGE_SIZE,fHandle->mgmtInfo);      //reads the pageNum block
    if(elementsRead != PAGE_SIZE){
        return RC_READ_FAILED;
    }
    fHandle->curPagePos=pageNum;
    return RC_OK;
}

/**
 * Returns the curPagePos of the file specified by fileHandle.
 * @param fHandle
 * @return
 */
int getBlockPos (SM_FileHandle *fHandle){
    CHECK_FILE_VALIDITY(fHandle);
    return fHandle->curPagePos;
}

/**
 * Reads a block of data from curPagePos of the file onto memPage.
 * It internally calls readBlock seeting pagePos = 0
 * This method returns RC_OK on success . It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_READ_NON_EXISTING_PAGE(invalid page number),
 * RC_READ_FAILED (file seek or read operation failure).
 * @param fileHandle
 * @param memPage
 * @return
 */
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(0,fHandle,memPage);
}

/**
 * Reads data from the block previous to the curPagePos of the file onto memPage.
 * It internally calls readBlock seeting pagePos = fHandle->curPagePos-1
 * This method returns RC_OK on success . It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_READ_NON_EXISTING_PAGE(invalid page number),
 * RC_READ_FAILED (file seek or read operation failure).
 * @param fileHandle
 * @param memPage
 * @return
 */
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->curPagePos-1,fHandle,memPage);
}

/**
 * Reads a block of data from curPagePos of the file onto memPage.
 * It internally calls readBlock seeting pagePos = fHandle->curPagePos
 * This method returns RC_OK on success . It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_READ_NON_EXISTING_PAGE(invalid page number),
 * RC_READ_FAILED (file seek or read operation failure).
 * @param fileHandle
 * @param memPage
 * @return
 */
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->curPagePos,fHandle,memPage);
}

/**
 * Reads data from the block next to the curPagePos of the file onto memPage.
 * It internally calls readBlock seeting pagePos = fHandle->curPagePos+1
 * This method returns RC_OK on success . It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_READ_NON_EXISTING_PAGE(invalid page number),
 * RC_READ_FAILED (file seek or read operation failure).
 * @param fileHandle
 * @param memPage
 * @return
 */
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->curPagePos+1,fHandle,memPage);
}

/**
 * Reads data from the last block file onto memPage.
 * It internally calls readBlock seeting pagePos = fHandle->totalNumPages-1
 * This method returns RC_OK on success . It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_READ_NON_EXISTING_PAGE(invalid page number),
 * RC_READ_FAILED (file seek or read operation failure).
 * @param fileHandle
 * @param memPage
 * @return
 */
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(fHandle->totalNumPages-1,fHandle,memPage);
}

/**
 * This method writes the data in memPage to the file specified by fileHandleand updates the curPagePos in fileHandle.
 *
 * It returns RC_OK on success and RC_WRITE_FAILED on failure.
 * @param pageNum
 * @param fileHandle
 * @param memPage
 * @return
 */
RC writeBlockData(int pageNum,SM_FileHandle *fileHandle, SM_PageHandle memPage) {
    if(fwrite(memPage, PAGE_ELEMENT_SIZE, PAGE_SIZE, fileHandle->mgmtInfo) != PAGE_SIZE*PAGE_ELEMENT_SIZE){
        return RC_WRITE_FAILED;
    }
    fileHandle -> curPagePos = pageNum;
    fflush(fileHandle ->mgmtInfo);
    return RC_OK;
}

/**
 * This method writes data from 'memPage' into the file at 'pageNum' position.
 * 1) Checks for validity of file
 * 2) Checks for validity of page number.
 * 3) If page num is equal to totalpages, it appends and empty page and uses the new page for writing.
 * 4) It determines the current position of the internal file pointer
 * 5) Calculates the new position of the file pointer where data has to be written.
 * 6) If the curPagePos is same as newPos it directly calls writeBlock
 * 7) Else it first moves the internal file pointer to newPos using fseek
 * 8) It than calls writeBlockData
 *
 * It returns RC_OK on success and RC_WRITE_FAILED/RC_WRITE_FAILED on error.
 * @param pageNum
 * @param fileHandle
 * @param memPage
 * @return
 */
RC writeBlock (int pageNum, SM_FileHandle *fileHandle, SM_PageHandle memPage){
    CHECK_FILE_VALIDITY(fileHandle);
    if(pageNum > fileHandle -> totalNumPages || pageNum < 0){
        return RC_WRITE_NON_EXISTING_PAGE;
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

/**
 * Writes data from memPage onto the curPagePos of the file.
 * It internally calls writeBlock seeting pagePos = curPagePos of file handle
 *
 * This method returns RC_OK on success and RC_WRITE_FAILED/RC_WRITE_FAILED on error.
 * @param fileHandle
 * @param memPage
 * @return
 */
RC writeCurrentBlock (SM_FileHandle *fileHandle, SM_PageHandle memPage){
    return writeBlock(fileHandle -> curPagePos,fileHandle,memPage);
}

/**
 * This method appends an empty block to the file.
 * 1)It moves the file pointer to the newPos where empty page needs to be added
 * 2)If the above step is successfull it appends and empty block data
 * 3)Increment the filehandle->totalNumPages
 * 4)Rewinds pointer to beginning of file
 * 6)Updates totalNumPages at the beginning of the file
 * 6)Moves file pointer back the npos of the newly added page.
 *
 * This method return RC_OK on success and RC_FILE_HANDLE_NOT_INIT/RC_WRITE_FAILED ON FAILURE.
 * @param fileHandle
 * @return
 */
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

/**
 * This method increases the totalNumPages in the file to numberOfPages passed.
 * 1) Checks for the validity of the file
 * 2) If the specified number of pages already exists it returns.
 * 3) It determines the number of pages to be appended and calls appendEmptyBlock for each one of them.
 *
 * This method returns RC_OK on success and RC_FILE_HANDLE_NOT_INIT/RC_WRITE_FAILED ON FAILURE.
 * @param numberOfPages
 * @param fileHandle
 * @return
 */
RC ensureCapacity (int numberOfPages, SM_FileHandle *fileHandle){

    CHECK_FILE_VALIDITY(fileHandle);
    if(fileHandle->totalNumPages >= numberOfPages){
        return RC_OK;
    }
    int pagesToBeAdded = numberOfPages - fileHandle->totalNumPages;
    int i;
    for(i=0; i<pagesToBeAdded; i++){
        RC ret_val = appendEmptyBlock(fileHandle);
        if(ret_val != RC_OK){
            return ret_val;
        }
    }
    return RC_OK;
}


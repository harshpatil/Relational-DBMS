//
// Created by Sowmya Parameshwara on 9/26/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTFILE "additional_test.bin"

/* prototypes for test functions */
static void testWriteBlock();
static void testAppendEmptyBlock();
static void testEnsureCapacity();
static void testWriteBlockAtAHigherPage();
static void testWriteCurrentBlock();


/* main function running all tests */
int main (void) {

    testWriteBlock();
    testAppendEmptyBlock();
    testEnsureCapacity();
    testWriteBlockAtAHigherPage();
    testWriteCurrentBlock();
    return 0;
}

void testWriteBlock(){

    if(fopen(TESTFILE,"r") != NULL){
        destroyPageFile(TESTFILE);
    }

    SM_FileHandle fileHandle;
    SM_PageHandle pageHandle;
    pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);

    // Create File
    TEST_CHECK(createPageFile (TESTFILE));
    // Open page file
    TEST_CHECK(openPageFile (TESTFILE, &fileHandle));

    // Write 4096 H char in page 1
    for (int i=0; i<PAGE_SIZE; i++){
        pageHandle[i] = 'H';
    }
    TEST_CHECK(writeBlock (0, &fileHandle, pageHandle));

    // Write 4096 S char in page 2
    for (int i=0; i<PAGE_SIZE; i++){
        pageHandle[i] = 'S';
    }
    TEST_CHECK(writeBlock (1, &fileHandle, pageHandle));

    // Assert total pages stored in page number -1
    TEST_CHECK(readBlock(-1, &fileHandle, pageHandle));
    ASSERT_TRUE(pageHandle[0]=='2', "Total number of pages are 2");

    // Assert value stored in page 1
    TEST_CHECK(readBlock(0, &fileHandle, pageHandle));
    for(int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(pageHandle[i]=='H', "Page 1 has 4096 'H' char");
    }

    // Assert value stored in page 2
    TEST_CHECK(readBlock(1, &fileHandle, pageHandle));
    for(int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(pageHandle[i]== 'S', "Page 2 has 4096 'S' char");
    }

    closePageFile (&fileHandle);
    destroyPageFile (TESTFILE);
    TEST_DONE();
}

void testAppendEmptyBlock(){

    if(fopen(TESTFILE,"r") != NULL){
        destroyPageFile(TESTFILE);
    }

    SM_FileHandle fileHandle;
    SM_PageHandle pageHandle;
    pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);

    // Create File
    TEST_CHECK(createPageFile (TESTFILE));
    // Open page file
    TEST_CHECK(openPageFile (TESTFILE, &fileHandle));

    // Append empty page
    TEST_CHECK(appendEmptyBlock(&fileHandle));

    // Append another empty page
    TEST_CHECK(appendEmptyBlock(&fileHandle));

    // Assert total pages in file
    TEST_CHECK(readBlock(-1,&fileHandle,pageHandle));
    ASSERT_TRUE(pageHandle[0]=='3', "Total number of pages are 3");

    closePageFile (&fileHandle);
    destroyPageFile (TESTFILE);
    TEST_DONE();
}

void testEnsureCapacity(){

    if(fopen(TESTFILE,"r") != NULL){
        destroyPageFile(TESTFILE);
    }

    SM_FileHandle fileHandle;
    SM_PageHandle pageHandle;
    pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);

    // Create File
    TEST_CHECK(createPageFile (TESTFILE));
    // Open page file
    TEST_CHECK(openPageFile (TESTFILE, &fileHandle));

    // Check ensure capacity by passing totalPages = 1, No new page should get added
    TEST_CHECK(ensureCapacity(1, &fileHandle));

    // Assert total pages in file
    TEST_CHECK(readBlock(-1,&fileHandle,pageHandle));
    ASSERT_TRUE(pageHandle[0]=='1', "Total number of pages are : 1");

    // Check ensure capacity by passing totalPages = 4, 3 new pages should get added
    TEST_CHECK(ensureCapacity(4, &fileHandle));

    // Assert total pages in file
    TEST_CHECK(readBlock(-1,&fileHandle,pageHandle));
    ASSERT_TRUE(pageHandle[0]=='4', "Total number of pages are : 4");

    closePageFile (&fileHandle);
    destroyPageFile (TESTFILE);
    TEST_DONE();
}

void testWriteBlockAtAHigherPage(){

    if(fopen(TESTFILE,"r") != NULL){
        destroyPageFile(TESTFILE);
    }

    SM_FileHandle fileHandle;
    SM_PageHandle pageHandle;
    pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);

    // Create File
    TEST_CHECK(createPageFile (TESTFILE));
    // Open page file
    TEST_CHECK(openPageFile (TESTFILE, &fileHandle));

    // Write 4096 H char in page 3
    for (int i=0; i<PAGE_SIZE; i++){
        pageHandle[i] = 'H';
    }
    RC result = writeBlock(2, &fileHandle, pageHandle);
    ASSERT_EQUALS_INT(result, RC_WRITE_FAILED, "Write failed as we are trying to write to a page which is greater than total number of pages + 1");

    // Write 4096 H char in page 2
    for (int i=0; i<PAGE_SIZE; i++){
        pageHandle[i] = 'S';
    }
    result = writeBlock(1, &fileHandle, pageHandle);
    ASSERT_EQUALS_INT(result, RC_OK, "Wrote at page 2");

    // Assert total pages in file
    TEST_CHECK(readBlock(-1,&fileHandle,pageHandle));
    ASSERT_TRUE(pageHandle[0]=='2', "Total number of pages are : 2");

    closePageFile (&fileHandle);
    destroyPageFile (TESTFILE);
    TEST_DONE();
}

void testWriteCurrentBlock(void){

    if(fopen(TESTFILE,"r") != NULL){
        destroyPageFile(TESTFILE);
    }

    SM_FileHandle fileHandle;
    SM_PageHandle pageHandle;
    pageHandle = (SM_PageHandle) malloc(PAGE_SIZE);

    createPageFile (TESTFILE);
    openPageFile (TESTFILE, &fileHandle);
    appendEmptyBlock(&fileHandle);

    // Write 4096 H char at current position, which is page 2
    for (int i=0; i<PAGE_SIZE; i++) {
        pageHandle[i] = 'H';
    }
    TEST_CHECK(writeCurrentBlock(&fileHandle, pageHandle));

    // Assert value stored in page 1
    TEST_CHECK(readBlock(0, &fileHandle, pageHandle));
    for(int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(pageHandle[i]==0, "Page 1 has 4096 0 char");
    }

    // Assert value stored in page 2
    TEST_CHECK(readBlock(1, &fileHandle, pageHandle));
    for(int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(pageHandle[i]=='H', "Page 2 has 4096 'H' char");
    }

    // Ensure capacity 6
    ensureCapacity(6, &fileHandle);

    // Write 4096 S char at current position, which is page 6
    for (int i=0; i<PAGE_SIZE; i++) {
        pageHandle[i] = 'S';
    }
    TEST_CHECK(writeCurrentBlock(&fileHandle, pageHandle));

    // Assert value stored in page 4
    TEST_CHECK(readBlock(3, &fileHandle, pageHandle));
    for(int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(pageHandle[i]==0, "Page 4 has 4096 0 char");
    }

    // Assert value stored in page 6
    TEST_CHECK(readBlock(5, &fileHandle, pageHandle));
    for(int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(pageHandle[i]=='S', "Page 6 has 4096 'S' char");
    }

    closePageFile (&fileHandle);
    destroyPageFile (TESTFILE);
    TEST_DONE();

}
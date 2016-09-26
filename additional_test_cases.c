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
#define TESTPF "additional_test.bin"

/* prototypes for test functions */
static void testWriteMethod(void);

/* main function running all tests */
int main (void) {
    testName = "";
    testWriteMethod();
    return 0;
}

void testWriteMethod(void){

    SM_FileHandle fh;
    SM_PageHandle ph,ph1;
    ph = (SM_PageHandle) malloc(PAGE_SIZE);
    ph1 = (SM_PageHandle) malloc(PAGE_SIZE);

    printf("Executing testWriteMethod \n");

    testName = "test Write Method \n";
    TEST_CHECK(createPageFile (TESTPF));
    TEST_CHECK(openPageFile (TESTPF, &fh));

    // change ph to be a string and write that one to disk
    for (int i=0; i<PAGE_SIZE; i++){
        ph[i] = 'a';
    }
    TEST_CHECK(writeBlock (0, &fh, ph));
    printf("writing first block\n");

    TEST_CHECK(appendEmptyBlock(&fh));
    ASSERT_TRUE(fh.totalNumPages==2, "Total number of pages updated correcly in file handle");

    TEST_CHECK(readBlock(-1,&fh,ph));
    printf("ph[0] : %c \n",ph);
    ASSERT_TRUE(ph[0]=='2', "Total number of pages updated correcly in file");


   TEST_CHECK(readBlock(0,&fh,ph));
    for (int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(ph[i]=='a', "Page 0 has only 'a' ");
    }

    TEST_CHECK(readBlock(1,&fh,ph));
    for (int i=0; i<PAGE_SIZE; i++){
        ASSERT_TRUE(ph[i]==0, "Page two has \0 data");
    }


    TEST_CHECK(ensureCapacity(2,&fh));
    TEST_CHECK(readBlock(-1,&fh,ph));
    printf("ph[0] : %c \n",ph);
    ASSERT_TRUE(ph[0]=='2', "Total number of pages updated to 2 correcly in file\n");



    TEST_CHECK(ensureCapacity(5,&fh));
    TEST_CHECK(readBlock(-1,&fh,ph));
    printf("ph[0] : %c \n",ph);
    ASSERT_TRUE(ph[0]=='5', "Total number of pages updated to 5 correcly in file\n");


    TEST_CHECK(closePageFile (&fh));
//    TEST_CHECK(destroyPageFile (TESTPF));

    TEST_DONE();
}

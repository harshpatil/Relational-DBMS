//
// Created by Sowmya Parameshwara on 11/3/16.
//

#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

typedef struct RMTableMgmtData
{
    int noOfTuples;
    int firstFreePageNumber;
    BM_PageHandle ph;
    BM_BufferPool bm;
} RMTableMgmtData;

extern RC initRecordManager (void *mgmtData){
    return RC_OK;
}

extern RC shutdownRecordManager (){
    return RC_OK;
}

extern RC createTable (char *name, Schema *schema){
    SM_FileHandle fh;
    RC rc;
    if((rc = createPageFile(name)) != RC_OK){
        return rc;
    }
    if((rc = openPageFile(name,&fh)) != RC_OK){
        return rc;
    }

    char *metaData = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);

    strcpy(metaData,"1\n"); //Number of pages in the file

    *(int*)metaData = 0; // Number of tuples
    metaData+= sizeof(int); //increment char pointer

    *(int*)metaData = 1; // First free page is 1 because page 0 is reserved for metadata
    metaData += sizeof(int); //increment char pointer

    *(int*)metaData = schema->numAttr; //set num of attributes
    metaData += sizeof(int);


    for(i=0; i<schema->numAttr; i++)
    {

        strncpy(metaData, schema->attrNames[i], 10);	// set Attribute Name
        metaData += 10;

        *(int*)metaData = (int)schema->dataTypes[i];	// Set the Data Types of Attribute
        metaData += sizeof(int);

        *(int*)metaData = (int) schema->typeLength[i];	// Set the typeLength of Attribute
        metaData += sizeof(int);

    }
    for(i=0; i<schema->keySize; i++) {
        *(int*)metaData = schema->keySize; // set keys for the schema
        metaData += sizeof(int);
    }

    if((rc=writeBlock(0, &fh, metaData))!=RC_OK){ // Write all meta data info To 0th page of file
        return rc;
    }

    if((rc=closePageFile(&fh))!=RC_OK){
        return rc;
    }
    return RC_OK;
}



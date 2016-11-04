//
// Created by Sowmya Parameshwara on 11/3/16.
//

#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <string.h>
#include <stdlib.h>

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
    int i;
    char *metaData = calloc(PAGE_SIZE, PAGE_ELEMENT_SIZE);
    strcpy(metaData,"1\n");

    *(int*)metaData = 0; // Number of tuples
    metaData+= sizeof(int); //increment char pointer

    for (i=0; i < PAGE_SIZE; i++)
        printf("File contents : %s \n",metaData[i]);

    *(int*)metaData = 1; // First free page is 1 because page 0 is reserved for metadata
    metaData += sizeof(int); //increment char pointer

    *(int*)metaData = schema->numAttr; //set num of attributes
    metaData += sizeof(int);

    for(i=0; i<schema->numAttr; i++)
    {

        strncpy(metaData, schema->attrNames[i], 20);	// set Attribute Name,assuming max field name is 20
        metaData += 20;

        *(int*)metaData = (int)schema->dataTypes[i];	// Set the Data Types of Attribute
        metaData += sizeof(int);

        *(int*)metaData = (int) schema->typeLength[i];	// Set the typeLength of Attribute
        metaData += sizeof(int);

    }
    for(i=0; i<schema->keySize; i++) {
        *(int*)metaData = schema->keyAttrs[i]; // set keys for the schema
        metaData += sizeof(int);
    }

    if((rc=writeBlock(0, &fh, metaData))!=RC_OK){ // Write all meta data info To 0th page of file
        return rc;
    }
    SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
    readBlock(0,&fh,ph);
    for (i=0; i < PAGE_SIZE; i++)
        printf("File contents : %s \n",metaData);

    if((rc=closePageFile(&fh))!=RC_OK){
        return rc;
    }
    return RC_OK;
}


extern RC openTable (RM_TableData *rel, char *name) {
    char *metadata;
    RC rc;
    int i;
    Schema *schema = (Schema *) malloc(sizeof(Schema));


    RMTableMgmtData *tableMgmtData = (RMTableMgmtData *) malloc(sizeof(RMTableMgmtData));
    rel->mgmtData = tableMgmtData;
    rel->name = name;
    if ((rc = initBufferPool(&tableMgmtData->bm, name, 10, RS_LRU, NULL)) != RC_OK) {
        return rc;
    }
    if ((rc = pinPage(&tableMgmtData->bm, &tableMgmtData->ph, 0)) != RC_OK) { // pinning the 0th page which has meta data
       return rc;
    }
    metadata = (char*) tableMgmtData->ph.data;
    tableMgmtData->noOfTuples= *(int*)metadata;
    metadata+= sizeof(int);
    tableMgmtData->firstFreePageNumber= *(int*)metadata;
    metadata+= sizeof(int);
    schema->numAttr = *(int*)metadata;
    metadata+= sizeof(int);

    schema->attrNames= (char**) malloc( sizeof(char*)*schema->numAttr);
    schema->dataTypes= (DataType*) malloc( sizeof(DataType)*schema->numAttr);
    schema->typeLength= (int*) malloc( sizeof(int)*schema->numAttr);
    for(i=0; i < schema->numAttr; i++)
        schema->attrNames[i]= (char*) malloc(20); //20 is max field length

    for(i=0; i < schema->numAttr; i++)
    {

        strncpy(schema->attrNames[i], metadata, 20);
        metadata += 20;

        schema->dataTypes[i]= *(int*)metadata;
        metadata += sizeof(int);

        schema->typeLength[i]= *(int*)metadata;
        metadata+=sizeof(int);
    }
    schema->keySize = *(int*)metadata;
    metadata+= sizeof(int);

    for(i=0; i<schema->keySize; i++) {
        schema->keyAttrs[i] = *(int*)metadata;
        metadata += sizeof(int);
    }
    rel->schema = schema;

    if((rc = unpinPage(&tableMgmtData->bm, &tableMgmtData->ph)) != RC_OK){
        return rc;
    }
    return RC_OK;
}

extern RC closeTable (RM_TableData *rel){
    return RC_OK;
}
extern RC deleteTable (char *name){
    return RC_OK;
}
extern int getNumTuples (RM_TableData *rel){
    return 0;
}

extern RC insertRecord (RM_TableData *rel, Record *record){
    return RC_OK;
}

extern RC deleteRecord (RM_TableData *rel, RID id){
    return RC_OK;
}

extern RC updateRecord (RM_TableData *rel, Record *record){
    return RC_OK;
}

extern RC getRecord (RM_TableData *rel, RID id, Record *record){
    return RC_OK;
}

extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){
    return RC_OK;
}
extern RC next (RM_ScanHandle *scan, Record *record){
    return RC_OK;
}

extern RC closeScan (RM_ScanHandle *scan){
    return RC_OK;
}

extern int getRecordSize (Schema *schema){
    return 0;
}

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
    Schema *schema = malloc(sizeof(Schema));
    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;
    return schema;
}

extern RC freeSchema (Schema *schema){
    return RC_OK;
}

extern RC createRecord (Record **record, Schema *schema){
    return RC_OK;
}

extern RC freeRecord (Record *record){
    return RC_OK;
}

extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){
    return RC_OK;
}

extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){
    return RC_OK;
}



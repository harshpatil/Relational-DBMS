//
// Created by Sowmya Parameshwara on 11/3/16.
//

#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <string.h>
#include <stdlib.h>
#include <tgmath.h>

/*  RMTableMgmtData stores metadata like number of tuples, first free page number, recored size,
 *  buffer pool and page handle
 */
typedef struct RMTableMgmtData {

    int noOfTuples;
    int firstFreePageNumber;
    int recordSize;
    BM_PageHandle pageHandle;
    BM_BufferPool bufferPool;
} RMTableMgmtData;

/* RMScanMgmtData stores scan details and condition */
typedef struct RMScanMgmtData {

    BM_PageHandle ph;
    RID rid; // current row that is being scanned
    int count; // no. of tuples scanned till now
    Expr *condition; // expression to be checked

} RMScanMgmtData;

RC initRecordManager (void *mgmtData){
    return RC_OK;
}

RC shutdownRecordManager (){
    return RC_OK;
}

/*  This function creates a new table i.e a page file taking name of table and schema of table as parameters
 *  Meta data values are set and stored in page 1
 */
RC createTable (char *name, Schema *schema){

    SM_FileHandle fh;
    RC rc;
    if((rc = createPageFile(name)) != RC_OK){
        return rc;
    }
    if((rc = openPageFile(name,&fh)) != RC_OK){
        return rc;
    }
    int i;
    char data[PAGE_SIZE];
    char *metaData = data;
    memset(metaData, 0, PAGE_SIZE);


    *(int*)metaData = 0; // Number of tuples
    metaData = metaData+ sizeof(int); //increment char pointer

    *(int*)metaData = 2; // First free page is 1 because page 0 is reserved for metadata
    metaData = metaData + sizeof(int); //increment char pointer

    *(int*)metaData = getRecordSize(schema);
    metaData = metaData + sizeof(int); //increment char pointer

    *(int*)metaData = schema->numAttr; //set num of attributes
    metaData = metaData + sizeof(int);

    for(i=0; i<schema->numAttr; i++)
    {

        strncpy(metaData, schema->attrNames[i], 20);	// set Attribute Name, assuming max field name is 20
        metaData = metaData + 20;

        *(int*)metaData = (int)schema->dataTypes[i];	// Set the Data Types of Attribute
        metaData = metaData + sizeof(int);

        *(int*)metaData = (int) schema->typeLength[i];	// Set the typeLength of Attribute
        metaData = metaData + sizeof(int);

    }
    for(i=0; i<schema->keySize; i++) {
        *(int*)metaData = schema->keyAttrs[i]; // set keys for the schema
        metaData = metaData + sizeof(int);
    }

    if((rc=writeBlock(1, &fh, data))!=RC_OK){ // Write all meta data info To 0th page of file
        return rc;
    }

    if((rc=closePageFile(&fh))!=RC_OK){
        return rc;
    }
    return RC_OK;
}

/*  This function is called to open existing table
 *  Once table is opened successfuly, meta data stored in page 1 is copied to handler
 */
RC openTable (RM_TableData *rel, char *name) {

    SM_PageHandle  metadata;
    RC rc;
    int i;
    Schema *schema = (Schema *) malloc(sizeof(Schema));


    RMTableMgmtData *tableMgmtData = (RMTableMgmtData *) malloc(sizeof(RMTableMgmtData));
    rel->mgmtData = tableMgmtData;
    rel->name = name;
    if ((rc = initBufferPool(&tableMgmtData->bufferPool, name, 10, RS_LRU, NULL)) != RC_OK) {
        return rc;
    }
    if ((rc = pinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle, 1)) != RC_OK) { // pinning the 0th page which has meta data
       return rc;
    }
    metadata = (char*) tableMgmtData->pageHandle.data;
    tableMgmtData->noOfTuples= *(int*)metadata;
    metadata = metadata + sizeof(int);
    tableMgmtData->firstFreePageNumber= *(int*)metadata;
    metadata = metadata + sizeof(int);
    tableMgmtData->recordSize=*(int *)metadata;
    metadata = metadata + sizeof(int);
    schema->numAttr = *(int*)metadata;
    metadata = metadata + sizeof(int);

    schema->attrNames= (char**) malloc( sizeof(char*)*schema->numAttr);
    schema->dataTypes= (DataType*) malloc( sizeof(DataType)*schema->numAttr);
    schema->typeLength= (int*) malloc( sizeof(int)*schema->numAttr);
    for(i=0; i < schema->numAttr; i++) {
        schema->attrNames[i]= (char*) malloc(20); //20 is max field length
    }

    for(i=0; i < schema->numAttr; i++) {

        strncpy(schema->attrNames[i], metadata, 20);
        metadata = metadata + 20;

        schema->dataTypes[i]= *(int*)metadata;
        metadata = metadata + sizeof(int);

        schema->typeLength[i]= *(int*)metadata;
        metadata = metadata + sizeof(int);
    }
    schema->keySize = *(int*)metadata;
    metadata = metadata + sizeof(int);

    for(i=0; i<schema->keySize; i++) {
        schema->keyAttrs[i] = *(int*)metadata;
        metadata = metadata + sizeof(int);
    }
    rel->schema = schema;

    if((rc = unpinPage(&tableMgmtData->bufferPool, &tableMgmtData->pageHandle)) != RC_OK){
        return rc;
    }
    return RC_OK;
}

/* This function closes opened table
 *  Before closing, total number of records in table is updated at page 1
 */
RC closeTable (RM_TableData *rel){
    RC rc;
    RMTableMgmtData* rmTableMgmtData;
    SM_FileHandle fileHandle;
    rmTableMgmtData = rel->mgmtData;

    if ((rc = pinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle, 1)) != RC_OK) {
        return rc;
    }
    char * metaData = rmTableMgmtData->pageHandle.data;
    *(int*)metaData = rmTableMgmtData->noOfTuples;

    markDirty(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle);

    if ((rc = unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle)) != RC_OK) {
        return rc;
    }
    if((rc=shutdownBufferPool(&rmTableMgmtData->bufferPool)) != RC_OK){
        return rc;
    }
    rel->mgmtData = NULL;
    return RC_OK;
}

/*  This function deletes table taking table name as input
 */
RC deleteTable (char *name){
    RC rc;
    if((rc = destroyPageFile(name)) != RC_OK){
        return rc;
    }
    return RC_OK;
}

/* This function returns total number of records present in table
 * This value is fetched from page 1 of page file
 */
int getNumTuples (RM_TableData *rel){

    RMTableMgmtData *rmTableMgmtData;
    rmTableMgmtData = rel->mgmtData;
    return rmTableMgmtData->noOfTuples;
}

/** This method inserts records
 * 1) It pins the first free page in which the records are to be inserted
                2) It checks if there is enough space in this page to insert the record
                3) Else it will go to next page to find a free slot, it keeps doing this till it gets a free slot.
                4) It will write the new record information at this slot information and update rid information accordingly.
                5) It then marks the page dirty
                6) It then unpins the page
                7) It then increments the number of tuples and updates the pointer.
                8) If the records are inserted properly, it returns RC_OK
 * @param rel
 * @param record
 * @return
 */
RC insertRecord (RM_TableData *rel, Record *record){

    RMTableMgmtData *rmTableMgmtData;
    char *data, *slotAddress;
    rmTableMgmtData = rel->mgmtData;
    int recordSize = rmTableMgmtData->recordSize + 1;
    RID *rid = &record->id;
    rid->page = rmTableMgmtData->firstFreePageNumber;
    rid->slot = -1;

    pinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle, rid->page);
    data = rmTableMgmtData->pageHandle.data;

    int i;
    int totalSlots = floor(PAGE_SIZE/recordSize);
    for (i = 0; i < totalSlots; i++) {
        if (data[i * recordSize] != '#'){
            rid->slot = i;
            break;
        }
    }

    while(rid->slot == -1){

        unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle);
        rid->page++;	// increment page number
        pinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle, rid->page);
        for (i = 0; i < totalSlots; i++) {
            if (data[i * recordSize] != '#'){
                rmTableMgmtData->firstFreePageNumber = rid->page;
                rid->slot = i;
                break;
            }
        }
    }

    slotAddress = data;
    markDirty(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle);

    slotAddress = slotAddress + rid->slot * recordSize;
    *slotAddress = '#';
    slotAddress++;
    memcpy(slotAddress, record->data, recordSize-1);

    unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle);
    rmTableMgmtData->noOfTuples++;
    record->id = *rid;
    return RC_OK;
}

/**  This method deletes records
                    1) It pins the page where records has to be deleted
                    2) It deletes the records by replacing "#" symbol with "$" to indicate the deletion of the record.
                    3) It decrements the number of tuples.
                    4) It marks the page dirty
                    5) It then unpins the page
                    6)  If the records are deleted properly, RC_OK is returned
 * @param rel
 * @param id
 * @return
 */
RC deleteRecord (RM_TableData *rel, RID id){

    RMTableMgmtData *rmTableMgmtData = rel->mgmtData;
    pinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle, id.page);
    rmTableMgmtData->noOfTuples--; //update number of tuples
    char *slotAddress, *data;
    int recordSize = rmTableMgmtData->recordSize + 1;
    data = rmTableMgmtData->pageHandle.data;

    slotAddress = data;
    slotAddress = slotAddress + id.slot * recordSize;
    *slotAddress = '$'; // set tombstone '$' for deleted record

    markDirty(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle);
    unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle);

    return RC_OK;
}

/**
 *  : This method updates records
                1) It pins the page where records has to be updated
                2) It finds the slot id in the page where the record has to be updated.
                3) It updates the new info at the above address.
                4) It marks the page dirty
                5) It then unpins the page
                6)  If the records are updated properly, RC_OK is returned
 * @param rel
 * @param record
 * @return
 */
RC updateRecord (RM_TableData *rel, Record *record){

    RMTableMgmtData *rmTableMgmtData = rel->mgmtData;
    pinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle, record->id.page);
    char * data = rmTableMgmtData->pageHandle.data;
    RID id = record->id;
    RC rc;

    int recordSize = rmTableMgmtData->recordSize+1;
    data = data + id.slot * recordSize + 1;

    memcpy(data,record->data,recordSize - 1);

    if((rc = markDirty(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle)) != RC_OK){
        return rc;
    }

    if((rc = unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle)) != RC_OK){
        return rc;
    }
    return RC_OK;
}

/**
 * This method gets a record
                1) It checks if the input parameter is valid
                2) It pins the page from where record is needed
                3) It uses slot id to go to the addrress corresponding to the account.
                4) If "#" is found at above address, it is a valid record, else it returns record not found status.
                5) If record found, It copies the contents of the record into record->data.
                6) It unpins the page
                7) It returns RC_OK
 * @param rel
 * @param id
 * @param record
 * @return
 */
RC getRecord (RM_TableData *rel, RID id, Record *record){

    RC rc;
    RMTableMgmtData *rmTableMgmtData = rel->mgmtData;
    if((rc=pinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle, id.page)) != RC_OK){
        return rc;
    }
    int recordSize = rmTableMgmtData->recordSize+1;
    char * recordSlotAddress = rmTableMgmtData->pageHandle.data;
    recordSlotAddress = recordSlotAddress + (id.slot*recordSize);
    if(*recordSlotAddress != '#')
        return RC_TUPLE_WIT_RID_ON_EXISTING;
    else{
        char *target = record->data;
        memcpy(target,recordSlotAddress+1,recordSize-1);
        record->id = id;

    }
    if((rc=unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle)) != RC_OK){
        return rc;
    }
    return RC_OK;
}

/**
 * This method initialises the scan
                1) It initialises RMScanMgmtData structure
                2) If it is initialised, it returns RC_OK
 * @param rel
 * @param scan
 * @param cond
 * @return
 */
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){

    scan->rel = rel;

    RMScanMgmtData *rmScanMgmtData = (RMScanMgmtData*) malloc( sizeof(RMScanMgmtData));
    rmScanMgmtData->rid.page= 2;
    rmScanMgmtData->rid.slot= 0;
    rmScanMgmtData->count= 0;
    rmScanMgmtData->condition = cond;

    scan->mgmtData = rmScanMgmtData;

    return RC_OK;
}

/**
 * This method gives the tuple that satisfies the scan
                1) If there are no tuples in the table it returns RC_RM_NO_MORE_TUPLES
                2) Else it starts scanning from the page number stored in RM_ScannedData
                3) It obtains a record and checks for the scan condition.
                4) If above is satisfies it will return RC_OK and updates record->data with the record details obtained from page file.
                5) Else it will take the next record and checks for condition, it repeats this till it finds a record which satisfies the condition or the end of table is reached
                6) If no record is found satisfying the condition it returns RC_RM_NO_MORE_TUPLES
 * @param scan
 * @param record
 * @return
 */
RC next (RM_ScanHandle *scan, Record *record){

    RMScanMgmtData *scanMgmtData = (RMScanMgmtData*) scan->mgmtData;
    RMTableMgmtData *tmt;
    tmt = (RMTableMgmtData*) scan->rel->mgmtData;

    Value *result = (Value *) malloc(sizeof(Value));
    static char *data;

    int recordSize = tmt->recordSize+1;
    int totalSlots = floor(PAGE_SIZE/recordSize);

    if (tmt->noOfTuples == 0)
        return RC_RM_NO_MORE_TUPLES;


    while(scanMgmtData->count <= tmt->noOfTuples ){
        if (scanMgmtData->count <= 0)
        {
            scanMgmtData->rid.page = 2;
            scanMgmtData->rid.slot = 0;

            pinPage(&tmt->bufferPool, &scanMgmtData->ph, scanMgmtData->rid.page);
            data = scanMgmtData->ph.data;

        }else{
            scanMgmtData->rid.slot++;
            if(scanMgmtData->rid.slot >= totalSlots){
                scanMgmtData->rid.slot = 0;
                scanMgmtData->rid.page++;
            }

            pinPage(&tmt->bufferPool, &scanMgmtData->ph, scanMgmtData->rid.page);
            data = scanMgmtData->ph.data;
        }

        data = data + (scanMgmtData->rid.slot * recordSize)+1;

        record->id.page=scanMgmtData->rid.page;
        record->id.slot=scanMgmtData->rid.slot;
        scanMgmtData->count++;

        memcpy(record->data,data,recordSize-1);

        if (scanMgmtData->condition != NULL){
            evalExpr(record, (scan->rel)->schema, scanMgmtData->condition, &result);
        }else{
            result->v.boolV == TRUE; // when no condition return all records
        }

        if(result->v.boolV == TRUE){  //result was found
            unpinPage(&tmt->bufferPool, &scanMgmtData->ph);
            return RC_OK;
        }else{
            unpinPage(&tmt->bufferPool, &scanMgmtData->ph);
        }
    }

    scanMgmtData->rid.page = 2; //Resetting after scan is complete
    scanMgmtData->rid.slot = 0;
    scanMgmtData->count = 0;
    return RC_RM_NO_MORE_TUPLES;
}

/**
 * This method closes the scan
                1) It unpins the page
                2) Frees memory allocated with RMScanMgmtData
                3) If it closes properly, it returns RC_OK
 * @param scan
 * @return
 */
RC closeScan (RM_ScanHandle *scan){
    RMScanMgmtData *rmScanMgmtData= (RMScanMgmtData*) scan->mgmtData;
    RMTableMgmtData *rmTableMgmtData= (RMTableMgmtData*) scan->rel->mgmtData;

    if(rmScanMgmtData->count > 0){
        unpinPage(&rmTableMgmtData->bufferPool, &rmTableMgmtData->pageHandle); // unpin the page
    }

    // Free mgmtData
    free(scan->mgmtData);
    scan->mgmtData= NULL;
    return RC_OK;
}

/**
 * This method returns the record size
                1) It identifies the attributes from the passed schema
                2) It finds the corresponding datatype
                3) It then sums up the size that is occupied in bytes
                4) It then returns size occupied by all the attributes
 * @param schema
 * @return
 */
int getRecordSize (Schema *schema){

    int size = 0, i;
    for(i=0; i<schema->numAttr; ++i){
        switch (schema->dataTypes[i]){
            case DT_STRING:
                size = size + schema->typeLength[i];
                break;
            case DT_INT:
                size = size + sizeof(int);
                break;
            case DT_FLOAT:
                size = size + sizeof(float);
                break;
            case DT_BOOL:
                size = size + sizeof(bool);
                break;
            default:
                break;
        }
    }
    return size;
}

/**
 * This method creates a schema
                1) It allocates the memory to schema
                2) It then initialises the schema attributes
                3) It returns schema
 * @param numAttr
 * @param attrNames
 * @param dataTypes
 * @param typeLength
 * @param keySize
 * @param keys
 * @return
 */
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){

    Schema *schema = malloc(sizeof(Schema));
    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;
    return schema;
}

/**
 * This method frees the schema
                    1) It frees the memory associated with the schema
                    2) If the schema is freed, it returns RC_OK
 * @param schema
 * @return
 */
RC freeSchema (Schema *schema){

    free(schema);
    return RC_OK;
}

/**
 * This method creates a record
                1) It first obtains the size through the passed schema
                2) It then allocates memory
                3) It sets page and slot id to -1
                4) Once record is created, it returns RC_OK
 * @param record
 * @param schema
 * @return
 */
RC createRecord (Record **record, Schema *schema){

    int recordSize = getRecordSize(schema);

    Record *newRecord = (Record*) malloc(sizeof(Record));
    newRecord->data= (char*) malloc(recordSize); // Allocate memory for data of record

    newRecord->id.page= -1; // page number is not fixed for empty record which is in memory
    newRecord->id.slot= -1; // slot number is not fixed for empty record which is in memory
    *record = newRecord; // set tempRecord to Record
    return RC_OK;
}

/**
 * This method frees the record
                1) It frees the record
                2) If the record is freed, it returns RC_OK
 * @param record
 * @return
 */
RC freeRecord (Record *record){

    free(record);
    return RC_OK;
}

/**
 * This method determines the offset of the attribute in record
                1) This method determines the the offset of the attribute in record
                2) Once determined, it returns RC_OK
 * @param schema
 * @param attrNum
 * @param result
 * @return
 */
RC determineAttributOffsetInRecord (Schema *schema, int attrNum, int *result)
{
    int offset = 0;
    int attrPos = 0;

    for(attrPos = 0; attrPos < attrNum; attrPos++) {
        switch (schema->dataTypes[attrPos])
        {
            case DT_STRING:
                offset = offset + schema->typeLength[attrPos];
                break;
            case DT_INT:
                offset = offset + sizeof(int);
                break;
            case DT_FLOAT:
                offset = offset + sizeof(float);
                break;
            case DT_BOOL:
                offset = offset + sizeof(bool);
                break;
        }
    }
    *result = offset;
    return RC_OK;
}

/**
 * This method gets an attribute of the record
                1) It determines the attribute offset in the record using "determineAttributOffsetInRecord" method
                2) It copies the value from this offset based on the attributes type information into the value object passed
                3) Returns RC_OK on success
 * @param record
 * @param schema
 * @param attrNum
 * @param value
 * @return
 */
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){

    int offset = 0;
    determineAttributOffsetInRecord(schema, attrNum, &offset);
    Value *tempValue = (Value*) malloc(sizeof(Value));
    char *string = record->data;
    string += offset;
    switch(schema->dataTypes[attrNum])
    {
        case DT_INT:
            memcpy(&(tempValue->v.intV) ,string, sizeof(int));
            tempValue->dt = DT_INT;
            break;
        case DT_STRING:
            tempValue->dt = DT_STRING;
            int len = schema->typeLength[attrNum];
            tempValue->v.stringV = (char *) malloc(len + 1);
            strncpy(tempValue->v.stringV, string, len);
            tempValue->v.stringV[len] = '\0';
            break;
        case DT_FLOAT:
            tempValue->dt = DT_FLOAT;
            memcpy(&(tempValue->v.floatV),string, sizeof(float));
            break;
        case DT_BOOL:
            tempValue->dt = DT_BOOL;
            memcpy(&(tempValue->v.boolV),string, sizeof(bool));
            break;
    }
    *value = tempValue;
    return RC_OK;
}

/**
 * It sets the attributes of the record to the value passed for the attribute determined by "attrNum"
                1) It uses "determineAttributOffsetInRecord" for determining the offset at which the attibute is stored
                2) It uses the attribute type to write the new value at the offset determined above
                3) Once the attribute is set, RC_OK is returned
 * @param record
 * @param schema
 * @param attrNum
 * @param value
 * @return
 */
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value){
    int offset = 0;
    determineAttributOffsetInRecord(schema, attrNum, &offset);
    char *data = record->data;
    data = data + offset;

    switch(schema->dataTypes[attrNum])
    {
        case DT_INT:
            *(int *)data = value->v.intV;
            break;
        case DT_STRING:
        {
            char *buf;
            int len = schema->typeLength[attrNum];
            buf = (char *) malloc(len + 1);
            strncpy(buf, value->v.stringV, len);
            buf[len] = '\0';
            strncpy(data, buf, len);
            free(buf);
        }
            break;
        case DT_FLOAT:
            *(float *)data = value->v.floatV;
            break;
        case DT_BOOL:
            *(bool *)data = value->v.boolV;
            break;
    }

    return RC_OK;
}



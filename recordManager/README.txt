==========================================================================================================================================
                                                   # How to run the script #
==========================================================================================================================================

1) Running with default test cases :
   Compile : make test_assign_3
   Run     : ./test_assign_3

2) Running expr test cases :
   Compile : make test_expr
   Run     : ./test_expr

3) For rerun / clean up :
   make clean

==========================================================================================================================================
                                                    # Problem Statement #
==========================================================================================================================================

In this assignment you are creating a record manager. The record manager handles tables with a fixed schema. Clients can insert records,
delete records, update records, and scan through the records in a table. A scan is associated with a search condition and only returns
records that match the search condition. Each table should be stored in a separate page file and your record manager should access the
pages of the file through the buffer manager implemented in the last assignment.

One page file is used to store all records corresponding to a table. The beginning of each record is marked with the symbol "#" followed by
the record data. Whenever a record is deleted "#" symbol is replaced with "$" to indicate the deletion of the record.
Page 0 of the file is used by storage manager to store the total number of pages and other book keeping information.
Page 1 of the file is used by record manager to store bookkeeping information like : total number of tuples, first free page index where data
can be inserted, the record size, schema information and keys for the schema.

==========================================================================================================================================
                                                 # File list #
==========================================================================================================================================

1) buffer_mgr.h
2) buffer_mgr_stat.h
3) dberror.h
4) dt.h
5) expr.h
6) record_mgr.h
7) storage_mgr.h
8) tables.h
9) test_helper.h
10) buffer_mgr.c
11) buffer_mgr_stat.c
12) dberror.c
13) expr.c
14) record_mgr.c
15) rm_serializer.c
16) storage_mgr.c
17) test_assign3_1.c
18) test_expr.c
19) README.txt
20) MakeFile

==========================================================================================================================================
                                                 # Additional error codes #
==========================================================================================================================================

Additional error codes added in dberror.h :

1) #define RC_TUPLE_WIT_RID_ON_EXISTING 501

==========================================================================================================================================
                                                 # Problem Solution #
==========================================================================================================================================

RMTableMgmtData in Record Manager has information like noOfTuples, firstFreePageNumber, recordSize, the index
of the first free page. It also has pageHandle and bufferPool. That has information about page and bufferpool.

RMScanMgmtData  in Record Manager has information like rid, count(number of tuples scanned till now), scan condition and BM_PageHandle.
It has record id ie is the current row that is being scanned, count is the number of tuples scanned till now, expression is the condition
that is to be checked.

Following are the functionalities that have been implemented.

1) initRecordManager : This method initialises the record manager

2) shutdownRecordManager : This method shuuts down the record manager

3) createTable : This method creates a table
                1) It checks if the input parameter is valid
                2) Its creates and opens a pageFile
                3) It serializes information of schema into string which has to be stored in file
                4) It gets the size of schema and sets the number of attributes of the table
                5) It sets name, datatype and typelength of the attribute.
                6) It sets the keys to the schema
                7) It writes all the metadata to the 1st page ie bookkeeping page
                8) Then it closes the pageFile
                9) If the table is created properly, RC_OK is returned.

4) openTable : This method opens a table
                1) It checks if the input parameter is valid
                2) It initialises a bufferpool
                3) It pins the bookkeeping page having metadata
                4) It deserializes information of the table into RMTableMgmtData datastructure
                5) It unpins the bookkeeping page
                5) If the table is opened properly, RC_OK is returned.

5) closeTable : This method closes a table
                1) It checks if the input parameter is valid
                2) It pins the bookkeeping page
                3) Updates the numberoftuples information into the bookkeeping page
                4) It marks the page dirty
                5) It unpins the page
                6) It then shuts down the bufferpool
                7) If the buffer pool shuts down properly, RC_OK is returned.

6) deleteTable : This method deletes a table
                1) It checks if the input parameter is valid
                2) It destroys the page
                3) If the page is deleted properly, RC_OK is returned.

7) getNumTuples : This method returns number of tuples
                1) It returns the number of tuples from RMTableMgmtData datastructure.

8) insertRecord : This method inserts records
                1) It pins the first free page in which the records are to be inserted
                2) It checks if there is enough space in this page to insert the record
                3) Else it will go to next page to find a free slot, it keeps doing this till it gets a free slot.
                4) It will write the new record information at this slot information and update rid information accordingly.
                5) It then marks the page dirty
                6) It then unpins the page
                7) It then increments the number of tuples and updates the pointer.
                8) If the records are inserted properly, it returns RC_OK

9) deleteRecord : This method deletes records
                1) It pins the page where records has to be deleted
                2) It deletes the records by replacing "#" symbol with "$" to indicate the deletion of the record.
                3) It decrements the number of tuples.
                4) It marks the page dirty
                5) It then unpins the page
                6)  If the records are deleted properly, RC_OK is returned

10) updateRecord : This method updates records
                1) It pins the page where records has to be updated
                2) It finds the slot id in the page where the record has to be updated.
                3) It updates the new info at the above address.
                4) It marks the page dirty
                5) It then unpins the page
                6)  If the records are updated properly, RC_OK is returned

11) getRecord : This method gets a record
                1) It checks if the input parameter is valid
                2) It pins the page from where record is needed
                3) It uses slot id to go to the addrress corresponding to the account.
                4) If "#" is found at above address, it is a valid record, else it returns record not found status.
                5) If record found, It copies the contents of the record into record->data.
                6) It unpins the page
                7) It returns RC_OK

12) startScan : This method initialises the scan
                1) It initialises RMScanMgmtData structure
                2) If it is initialised, it returns RC_OK

13) next : This method gives the tuple that satisfies the scan
                1) If there are no tuples in the table it returns RC_RM_NO_MORE_TUPLES
                2) Else it starts scanning from the page number stored in RM_ScannedData
                3) It obtains a record and checks for the scan condition.
                4) If above is satisfies it will return RC_OK and updates record->data with the record details obtained from page file.
                5) Else it will take the next record and checks for condition, it repeats this till it finds a record which satisfies the condition or the end of table is reached
                6) If no record is found satisfying the condition it returns RC_RM_NO_MORE_TUPLES

14) closeScan : This method closes the scan
                1) It unpins the page
                2) Frees memory allocated with RMScanMgmtData
                3) If it closes properly, it returns RC_OK

15) getRecordSize : This method returns the record size
                1) It identifies the attributes from the passed schema
                2) It finds the corresponding datatype
                3) It then sums up the size that is occupied in bytes
                4) It then returns size occupied by all the attributes

16) createSchema : This method creates a schema
                1) It allocates the memory to schema
                2) It then initialises the schema attributes
                3) It returns schema

17) freeSchema : This method frees the schema
                    1) It frees the memory associated with the schema
                    2) If the schema is freed, it returns RC_OK

18) createRecord : This method creates a record
                1) It first obtains the size through the passed schema
                2) It then allocates memory
                3) It sets page and slot id to -1
                4) Once record is created, it returns RC_OK

19) freeRecord : This method frees the record
                1) It frees the record
                2) If the record is freed, it returns RC_OK

20) determineAttributOffsetInRecord : This method determines the offset of the attribute in record
                1) This method determines the the offset of the attribute in record
                2) Once determined, it returns RC_OK

21) getAttr : This method gets an attribute of the record
                1) It determines the attribute offset in the record using "determineAttributOffsetInRecord" method
                2) It copies the value from this offset based on the attributes type information into the value object passed
                3) Returns RC_OK on success

22) setAttr :  It sets the attributes of the record to the value passed for the attribute determined by "attrNum"
                1) It uses "determineAttributOffsetInRecord" for determining the offset at which the attibute is stored
                2) It uses the attribute type to write the new value at the offset determined above
                3) Once the attribute is set, RC_OK is returned

==========================================================================================================================================
                                                # Contributors #
==========================================================================================================================================

Group Name : Group 10
Leader Name : Sowmya Parameshwara

Team Member Details (in alphabetical order) :
1) Name : Chinmayee Hota
   CWID : A20375244
   Email : chota@hawk.iit.edu

2) Name : Harshavardhan Patil
   CWID : A20387828
   Email : hpatil2@hawk.iit.edu

3) Name : Rucha Dubbewar
   CWID : A20373886
   Email : rdubbewar@hawk.iit.edu

4) Name : Sowmya Parameshwara
   CWID : A20387836
   Email : sparameshwara@hawk.iit.edu

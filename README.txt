==========================================================================================================================================
                                                   # How to run the script #
==========================================================================================================================================

1) Running with default test cases :
   Compile : make test_assign_1
   Run     : ./test_assign_1

2) Running with additional test cases :
   Compile :  make additional_test_cases_1
   Run :  ./additional_test_cases_1

3) For rerun / clean up :
   make clean

==========================================================================================================================================
                                                 # Problem Statement #
==========================================================================================================================================

The goal of this assignment is to implement a simple storage manager - a module that is capable of reading blocks from a file
on disk into memory and writing blocks from memory to a file on disk. The storage manager deals with pages (blocks) of fixed size
(PAGE_SIZE=4096 bytes). In addition to reading and writing pages from a file, it provides methods for creating, opening,
and closing files. The storage manager has to maintain several types of information for an open file: The number of total pages in
the file, the current page position (for reading and writing), the file name, and a POSIX file descriptor or FILE pointer.


==========================================================================================================================================
                                                 # File list #
==========================================================================================================================================

1) dberror.h
2) storage_mgr.h
3) test_helper.h
4) dberror.c
5) storage_mgr.c
6) test_assign1_1.c
7) additional_test_cases.c
8) Makefile


==========================================================================================================================================
                                                 # Additional error codes #
==========================================================================================================================================

Additional error codes added in dberror.h :

1) RC_FILE_ALREADY_EXISTS = 5
2) RC_READ_FAILED = 6
3) RC_WRITE_NON_EXISTING_PAGE = 7


==========================================================================================================================================
                                                 # Additional files #
==========================================================================================================================================

1) additional_test_cases.c (Test case file)

==========================================================================================================================================
                                                 # Additional Test Cases #
==========================================================================================================================================

1) testWriteBlock(); -->
   Create file, open page, write data in 1st page, write data in 2nd page
    - Check if total pages in page -1 has got updated or not
    - Read block 0 and check if value returned is same as what was written. Do the same for block 1

2) testAppendEmptyBlock(); -->
   Create file, open page, append Empty Block, again append empty block
    - Check if total pages in page -1 has got updated or not

3) testEnsureCapacity();
   A) Create file, open page, call ensureCapacity with capacity=1;
    - Total pages in page -1 should not change
   B) Now call ensureCapacity with capacity=4
    - Check if 3 new files have got added and total pages in page -1 is updated to 4

4) testWriteBlockAtAHigherPage(); -->
   A) Create file, open page, call writeBlock at page 3
    - User should not be allowed to write
   B) Now try to write at page 2
    - Page 2 should get written

5) testWriteCurrentBlock(); -->
   A) Create file, open page, append an empty block, call writeCurrentBlock
    - Data should get written at page 2 and page 1 should have only 0's
   B) Now call ensure capacity with capacity=6 then call writeCurrentBlock
    - Data should be written at page 6. Page 3,4 & 5 should be empty

6) testReadIncorrectPage(); -->
   A) Create file, open page, call readBlock with pageNumber=-2
    - RC_READ_NON_EXISTING_PAGE error should be thrown
   B) Now call readBlock with pageNumber=100
    - RC_READ_NON_EXISTING_PAGE error should be thrown

7) testResetValuesOnFileClosure(); -->
   Create file, open page, write data at page 1, write data at page 2, call ClosePageFile()
    - fileHandle structure values should get reset to 0 or NULL

8) testReadPreviousBlock(); -->
   Create file, open page, write data at page 1, write data at page 2, call readPreviousBlock
    - Page 1 data should be retured

9) testReadCurrentBlock(); -->
   Create file, open file, write data in file 1, write data in file 2
   Call readCurrentBlock();
     -Page 2 should be returned

10) testReadLastBlock(); -->
    Create file, open file, write data in file 1, write data in file 2, write data in file 3
    Call readLastBlock();
     -Page 3 should be returned

11) testReadNextBlock(); -->
    Create file, open file, write data in file 1, write data in file 2
    Call readNextBlock();
     -Page 2 should be returned

12) testReadBlock(); -->
    Create file, open file, write data in file 1
    Call readBlock();
     -Page 1 should be returned

13) testGetBlockPos(); -->
    Create file, open file, write data in file 1, append page at the end of page
    Call getBlockPos();
     -Page 1 should be returned

14) testReadFirstBlock();
    Create file, open file, write data in file 1, write data in file 2
    Close file, open file
    Call readFirstBlolck();
     -Page 1 should be returned.

==========================================================================================================================================
                                                 # Problem Solution #
==========================================================================================================================================

SM_FileHandle has information about the currently open file. It holds information about the file name, total number of pages,
current page position and a file pointer. SM_PageHandle is a pointer to an area in the memory from which data will be written to
the file or the memory to which data is written from a file. The first page of the file is reserved for maintaining
booking information : totalNumPages. Page indexes start from 0. The storage manager only deals with pages (blocks) of fixed
size (4096 bytes). The application is single threaded. Read and write operations can be performed only after opening the file
till user closes it. Read and write operations work only on valid file and page position.

1) createPageFile :
   For the first time user needs to create a file before performing any further operations on it. A file with two pages
   will be created and totalNumPages is set to 1. We have stored "totalNumPages" at the first page (bookkeeping page page position -1),
   and the actual data exposed to user starts from second page(page postion 0) which currently is filled with '\0' bytes.
   On success it returns RC_OK, if the file already exists it fails with RC_FILE_ALREADY_EXISTS.

2) openPageFile :
   User needs to open the file after which he can perform any number of read and write operations before he closes it.
   On file open we save the details of the file( file name, file pointer, totalNumPages and curPagePos) in SM_FileHandle.
   CurPagePos will be set to 0, and totalNumPages will be set to the value stored in the file.
   On success it returns RC_OK, if the file does not exist it fails with RC_FILE_NOT_FOUND.

3) closePageFile :
   On close operation, the underlying file will be closed and the values in SM_FileHandle will be deinitialised.
   User can no longer perform read/write on the file until he opens it again.
   On success it returns RC_OK, if the file does not exist it fails with RC_FILE_NOT_FOUND.

4) destroyPageFile :
   On destroy operation, the file is removed.
   On success it returns RC_OK, if the file does not exist it fails with RC_FILE_NOT_FOUND.

5) readBlock :
   Read operation is allowed only if the file,page number are valid(-1 upto totalNumPages-1) and file is open. All read operations return a
   page (4096 bytes) of data. After read curPagePos is updated to the page position which was currently read.
   On success it returns RC_OK. It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_READ_NON_EXISTING_PAGE(invalid page number),
   RC_READ_FAILED (file seek or read operation failure).

6) getBlockPos:
   If the file is valid return curPagePos of the file.
   On success it returns RC_OK. If file is not valid it fails with CHECK_FILE_VALIDITY.

7) readFirstBlock :
   It reads the first block (pagePostion =0 ) from the file. Everything else is same as readBlock.

8) readPreviousBlock :
   It reads the block previous to the curPagePos (pagePostion = curPagePos-1 ) from the file. Everything else is same as readBlock.

9) readCurrentBlock :
   It reads the current block (pagePostion = curPagePos) from the file. Everything else is same as readBlock.

10) readNextBlock :
    It reads the block next to the curPagePos (pagePostion = curPagePos+1 ) from the file. Everything else is same as readBlock.

11) readLastBlock :
    It reads the last block (pagePostion = totalNumPages-1) from the file. Everything else is same as readBlock.

12) writeBlock :
    Write operation is allowed only if the file,page number are valid and file is open. A page is valid if it is an existing page, or a new page
    appended to the end. ie., if the file currently has 3 pages, write is allowed for page position 0,1,2 (The old data in this page is
    replaced with the new data) and write is allowed for page position 3, in this case the totalNumPages is updated to 3 and a new page
    with the passed data is appended to the file. All write operations write a  page (4096 bytes) of data. After write curPagePos is updated to the
    page position which was currently written.
    On success it returns RC_OK. It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_WRITE_NON_EXISTING_PAGE(invalid page number),
    RC_WRITE_FAILED (file seek or write operation failure).

13) writeCurrentBlock :
    Writes the data into the curPagePos of the file(pagePos = curPagePos). Everything else is same as writeBlock.

14) appendEmptyBlock :
    This operation is allowed if the file is valid and open. A new page with '\0' bytes is appended to the end of the file. TotalNumPages is
    incremented by 1 both in SM_FileHandle and the bookkeeping page of the file. CurPagePos will be set to the newly appended page position.
    On success it returns RC_OK. It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_WRITE_FAILED (file seek or write operation failure).

15) ensureCapacity :
    This operation is allowed only if the file is valid and open. If the required number of pages already exists it doesnt do anything,
    else it will append the required number of pages with '\0' bytes into the file. Update totalNumPages to the new size in both SM_FileHandle and
    the bookkeeping page of the file. Update curPagePos to the last page appended.
    On success it returns RC_OK. It can fail with RC_FILE_HANDLE_NOT_INIT (invalid file), RC_WRITE_FAILED (file seek or write operation failure).



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



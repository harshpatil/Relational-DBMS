==========================================================================================================================================
                                                   # How to run the script #
==========================================================================================================================================

1) Running with default test cases :
   Compile : make test_assign_1
   Run     : ./test_assign_1

2) Running with additional test cases :
   Compile :  make additional_test_cases
   Run :  ./additional_test_cases

3) For rerun / clean up :
   make clean

==========================================================================================================================================
                                                 # Problem Statement #
==========================================================================================================================================

You should implement a buffer manager in this assignment. The buffer manager manages a fixed number of pages in memory that represent pages
from a page file managed by the storage manager implemented in assignment 1. The memory pages managed by the buffer manager are called page
frames or frames for short. We call the combination of a page file and the page frames storing pages from that file a Buffer Pool. The
Buffer manager should be able to handle more than one open buffer pool at the same time. However, there can only be one buffer pool for
each page file. Each buffer pool uses one page replacement strategy that is determined when the buffer pool is initialized. You should at
least implement two replacement strategies FIFO and LRU. Your solution should implement all the methods defined in the buffer_mgr.h

==========================================================================================================================================
                                                 # File list #
==========================================================================================================================================

1) dberror.h
2) storage_mgr.h
3) test_helper.h
4) buffer_mgr.h
5) buffer_mgr_stat.h
6) dt.h
7) additional_test_cases.c
8) buffer_mgr.c
9) buffer_mgr_stat.c
10)dberror.c
11)storage_mgr.c
12)test_assign2_1.c
13)Makefile
14)README.txt

==========================================================================================================================================
                                                 # Additional error codes #
==========================================================================================================================================

Additional error codes added in dberror.h :

1) RC_BM_INVALID 401
2) RC_BM_INVALID_PAGE 402
3) RC_BM_POOL_IN_USE 403
4) RC_BM_INVALID_UNPIN 404
5) RC_BM_TOO_MANY_CONNECTIONS 405
5)

==========================================================================================================================================
                                                 # Additional files #
==========================================================================================================================================

1) additional_test_cases.c (Test case file)

==========================================================================================================================================
                                                 # Additional Test Cases #
==========================================================================================================================================



==========================================================================================================================================
                                                 # Problem Solution #
==========================================================================================================================================

BufferManagerInfo structure holds the book keeping information needed for the buffer pool. It maintains the informations
like : framesFilled, numOfDiskReads, numOfDiskWrites. It maintains arrays where the array index corresponds to the frame number
such as : frameToPageId, dirtyFlagPerFrame, pinCountsPerFrame. It maintains an array where the array index corresponds to
the page number : pageIdToFrameIndex. It also maintains a pointer to the head and tail frameNode.

FrameNode structure helps store information for each frame in the buffer pool. This structure holds the following information :
pageNumber, frameNumber, dirty flag, fixCount, pointer to the data location, pointer to the next and previous FrameNode in the buffer
pool.

All the framenodes in a buffer pool are maintained as a doubly linked list. The buffer pool has a pointer to the head and tail node of
this linked list.

Replacement Strategy : LRU and FIFO have been implemented.
In FIFO we add the frame which gets loaded with a disk page at the head of the linked list. If the frame exists in the
buffer pool we do not change it's position in the list. So the list is ordered based on time the frame was first used for a page.
So whenever we need to find a replacement frame, we navigate from the tail going backwards until we find a frameNode which has a fix count of zero.
Note : The frame number of a node is not dependent on the position of the node in the linked list.

In LRU we add the frame which gets loaded with a disk page at the head of the linked list. Even if an exiting page in the pool is pinned
we move the node corresponding to the page to the head of the list. So the list is ordered based on the access, with the least recently
accessed node at the end of the list. So whenever we need to find a replacement frame, we navigate from the tail going backwards until
we find a frameNode which has a fix count of zero.
Note : The frame number of a node is not dependent on the position of the node in the linked list.

1) initBufferPool : This method initialises the buffer pool.
                    i) It checks if the input parameters passed is valid.
                   ii) If the pagefile input corresponds to a valid file.
                  iii) It initialises BM_BufferPool and BufferManagerInfo object properties to it's default values.
                   iv) It creates frameNodes equal to page size input.
                    v) It closes the page File.
                   vi) It returnc RC_OK if buffer pool was successfully initialised.

2) shutdownBufferPool : This method shuts down the buffer pool and releases all resources acquired.
                         i) It checks if the input parameters passed is valid.
                        ii) It checks if there are any pinned page.
                       iii) If both the above condition passes, It will writes all dirty pages back to the disk.
                        iv) It free's the memory occupied by all FrameNodes corresponding to this buffer pool.
                         v) It free's the buffer manager info.
                        vi) If the buffer pool has been shut down properly, it will return RC_OK.

3) forceFlushPool : This mthod writes al dirty pages in the pool back to disk.
                    i) It checks for the validity of the input parameters.
                   ii) It opens the page file.
                  iii) It iterates through the frame nodes, checking for dirty frames.
                   iv) When a frame is dirty, it will write the contents back to disk.
                    v) Marks the frame as non dirty.
                   vi) Marks it non dirty in the array dirtyFlagPerFrame.
                  vii) Once all the dirty frames are written, it closes the page file.
                 viii) Returns RC_OK on success.

4) markDirty : This method marks the given page as dirty.
               i)It checks for validity of the inputs.
              ii)It loops through the buffer pool, searching for the frame storing this page.
             iii) If page isnot found it returns an error.
              iv) Else it marks the FrameNode storing this page as dirty.
               v) It updates the "dirtyFlagPerFrame" array marking the frame corresponding to the page as dirty.
              vi) Returns RC_OK on success.

5) unpinPage :  This method unpins the page.
                i) It checks the input for validity.
               ii) It finds the FrameNode corresponding to the page passed.
              iii) It checks if the fixcount is greater than 0, if yes decerements the fix count. It will update the array pinCountsPerFrame with the updated fix count.
               iv) Else it will return an error message RC_BM_INVALID_UNPIN.
                v) Returns RC_OK on success.

6) forcePage :  This method will write the contents of the page back to disk.
                i) It will find the frame node corresponding to the page number.
               ii) If the page is not found in the frame node list, it will return an error "RC_BM_INVALID_PAGE".
              iii) Else it will open the pageFile.
               iv) It writes the data of that page back to disk.
                v) Increment numOfDiskWrites.
               vi) If the fic count is zero, set dirty flag back to false.
              vii) Closes the page file.
             viii) Returns RC_OK on success.

7) pinPage :

8) getFrameContents : Returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame.
                      An empty page frame is represented using the constant NO_PAGE.

9) getDirtyFlags : Returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty.
                   Empty page frames are considered as clean.

10) getFixCounts : Returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame.
                   Return 0 for empty page frames.

11) getNumReadIO : Returns the number of pages that have been read from disk since a buffer pool has been initialized.

12) getNumWriteIO : Returns the number of pages written to the page file since the buffer pool has been initialized.
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




CC=gcc
CFLAGS=-I.

dberror.o: dberror.c
	$(CC) -o dberror.o -c dberror.c

storage_mgr.o: storage_mgr.c
	$(CC) -o storage_mgr.o -c storage_mgr.c

buffer_mgr.o: buffer_mgr.c
	$(CC) -o buffer_mgr.o -c buffer_mgr.c

test_assign2_1.o: test_assign2_1.c
	$(CC) -o test_assign2_1.o -c test_assign2_1.c

additional_test_cases.o: additional_test_cases.c
	$(CC) -o additional_test_cases.o -c additional_test_cases.c

test_assign_1: test_assign2_1.o buffer_mgr.o storage_mgr.o dberror.o
		$(CC) -o test_assign_1 test_assign2_1.o buffer_mgr.o storage_mgr.o dberror.o -I.

additional_test_cases: additional_test_cases.o buffer_mgr.o storage_mgr.o dberror.o
        $(CC) -o additional_test_cases additional_test_cases.o buffer_mgr.o storage_mgr.o dberror.o -I.

clean:
	rm test_assign_1 additional_test_cases test_assign2_1.o additional_test_cases.o buffer_mgr.o storage_mgr.o dberror.o

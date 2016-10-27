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

test_assign_2: test_assign2_1.o buffer_mgr.o storage_mgr.o dberror.o
		$(CC) -o test_assign_2 test_assign2_1.o buffer_mgr.o storage_mgr.o dberror.o -I.

clean:
	rm test_assign_2 test_assign2_1.o buffer_mgr.o storage_mgr.o dberror.o

CC=gcc
CFLAGS=-I.

dberror.o: dberror.c
	$(CC) -o dberror.o -c dberror.c

storage_mgr.o: storage_mgr.c
	$(CC) -o storage_mgr.o -c storage_mgr.c

buffer_mgr.o: buffer_mgr.c
	$(CC) -o buffer_mgr.o -c buffer_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c
	$(CC) -o buffer_mgr_stat.o -c buffer_mgr_stat.c

record_mgr.o: record_mgr.c
	$(CC) -o record_mgr.o -c record_mgr.c -lm

expr.o: expr.c
	$(CC) -o expr.o -c expr.c

rm_serializer.o: rm_serializer.c
	$(CC) -o rm_serializer.o -c rm_serializer.c

test_assign3_1.o: test_assign3_1.c
	$(CC) -o test_assign3_1.o -c test_assign3_1.c -lm

test_expr.o: test_expr.c
	$(CC) -o test_expr.o -c test_expr.c

test_assign_3: test_assign3_1.o buffer_mgr_stat.o buffer_mgr.o storage_mgr.o dberror.o record_mgr.o expr.o rm_serializer.o
	$(CC) -o test_assign_3 test_assign3_1.o buffer_mgr_stat.o buffer_mgr.o storage_mgr.o dberror.o record_mgr.o expr.o rm_serializer.o -I. -lm

test_expr: test_expr.o buffer_mgr_stat.o buffer_mgr.o storage_mgr.o dberror.o record_mgr.o expr.o rm_serializer.o
	$(CC) -o test_expr test_expr.o buffer_mgr_stat.o buffer_mgr.o storage_mgr.o dberror.o record_mgr.o expr.o rm_serializer.o -I. -lm

clean:
	rm test_assign_3 test_expr test_expr.o test_assign3_1.o rm_serializer.o expr.o record_mgr.o buffer_mgr.o storage_mgr.o dberror.o buffer_mgr_stat.o test_table_r test_table_t

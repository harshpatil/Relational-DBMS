//
// Created by Harshavardhan Patil on 10/26/16.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"

typedef struct frameNode{

    int pageNumber;
    int frameNumber;
    int dirty;
    int fixCount;
    int pageFrequency;
    char *data;
    struct frameNode *next;
    struct frameNode *previous;

}frameNode;

typedef struct frameList{

    frameNode *first;
    frameNode *last;

}frameList;

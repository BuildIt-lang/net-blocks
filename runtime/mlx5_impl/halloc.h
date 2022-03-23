#ifndef _HALLOC_H
#define _HALLOC_H
#include <infiniband/verbs.h>
#include <iostream>
#include "utils.h"
char * alloc_shared(int size, int * shm_key, struct ibv_pd*);

#endif

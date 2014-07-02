#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "reg_error.h"

#define HEAD_SIZE  sizeof(int)
#define MAX_MAM_COUNT 0xffff

#define _head(p) *(((int*)(p))-1)
#define _set_head(p, c) (_head(p) = (c))
#define _point(p) (((int*)(p))-1)


struct mem_info {
  char* file;
  int line;
  void* p;
  size_t size;
};


static struct mem_info mem_size_buf[MAX_MAM_COUNT];
static int MEM_LAST_FREE = 0;



static inline int _get_idx(){
  if(mem_size_buf[MEM_LAST_FREE].size){
    reg_painc("overfllow memory.");
  }
  int ret = MEM_LAST_FREE;

  for(int i=MEM_LAST_FREE+1; i<MAX_MAM_COUNT; i++){
    if(mem_size_buf[i].size == 0){
      MEM_LAST_FREE = i;
      break;
    }
  }
  return ret;
}

void* reg_malloc(size_t size, char* file, int line){
  int* ret = malloc(size + HEAD_SIZE);
  *ret = _get_idx();
  assert(mem_size_buf[*ret].size == 0);
  mem_size_buf[*ret].size = size;
  mem_size_buf[*ret].file = file;
  mem_size_buf[*ret].line = line;
  mem_size_buf[*ret].p = ret+1;

  return (void*)(ret+1);
}

void* reg_calloc(size_t count, size_t size, char* file, int line){
  size_t c_size = count*size;
  void* ret = reg_malloc(c_size, file, line);
  memset(ret, 0, c_size);
  return ret;
}

void* reg_realloc(void* p, size_t size, char* file, int line){
  int idx = _head(p);
  assert(idx >=0 && idx < MAX_MAM_COUNT);
  assert(mem_size_buf[idx].size > 0);

  int* ret = realloc(_point(p), size+HEAD_SIZE);
  mem_size_buf[idx].size = size;
  mem_size_buf[idx].line = line;
  mem_size_buf[idx].file = file;
  mem_size_buf[idx].p = ret+1;
  assert(*ret == idx);
  *ret = idx;

  printf("reg_realloc idx: %d\n", idx);
  return ret+1;
}

void reg_free(void* p){
  assert(p);
  int idx = _head(p);
  assert(idx>=0 && idx<MAX_MAM_COUNT && 
    mem_size_buf[idx].p == p && 
    mem_size_buf[idx].size > 0);
  
  free(_point(p));
  MEM_LAST_FREE = idx;
  mem_size_buf[idx].size = 0;
  mem_size_buf[idx].p = NULL;
}


// for test
void reg_dump(){
  int len = sizeof(mem_size_buf) / sizeof(mem_size_buf[0]);

  printf("-------memory check----------\n");
  for(int i=0; i<len; i++){
    size_t size = mem_size_buf[i].size;
    if(size){
      char* file = mem_size_buf[i].file;
      int line = mem_size_buf[i].line;
      void* point = mem_size_buf[i].p;
      printf("[memory leakly]: index: %d point: %p leakly size %zd   @file:  %s:%d\n", i, point, size, file, line);  
    }
  }
}




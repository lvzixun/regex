#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define HEAD_SIZE  sizeof(int)
#define MAX_MAM_COUNT 0xffff

#define _head(p) *(((int*)(p))-1)
#define _set_head(p, c) (_head(p) = (c))
#define _point(p) (((int*)(p))-1)


struct mem_info {
  char* file;
  int line;
  size_t size;
};


static struct mem_info mem_size_buf[MAX_MAM_COUNT];
static int MEM_LAST_FREE = 0;

void reg_painc(const char* str);

static inline int _get_idx(){
  if(MEM_LAST_FREE >= MAX_MAM_COUNT){
    reg_painc("overfllow memory.");
  }
  return MEM_LAST_FREE++;
}

static void _error(){
  exit(0);
}

void reg_error(const char* format, ...){
  char buf[0xff] = {0};
  va_list args;

  va_start(args, format);
  vsnprintf(buf, sizeof(buf)-1, format, args);
  printf("%s", buf);
  va_end(args);

  _error();
}


void reg_painc(const char* str){
  printf("[painc]: %s\n", str);
  _error();
}


void* reg_malloc(size_t size, char* file, int line){
  int* ret = malloc(size + HEAD_SIZE);
  *ret = _get_idx();
  mem_size_buf[*ret].size = size;
  mem_size_buf[*ret].file = file;
  mem_size_buf[*ret].line = line;

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
  _set_head(ret, size);
  return ret+1;
}


void reg_free(void* p){
  assert(p);
  int idx = _head(p);
  assert(idx>=0 && idx<MAX_MAM_COUNT);
  MEM_LAST_FREE = idx;
  mem_size_buf[idx].size = 0;
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
      printf("[memory leakly]: leakly size %zd   @file:  %s:%d\n", size, file, line);  
    }
  }
}










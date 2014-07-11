#ifndef _REG_MALLOC_H_
#define _REG_MALLOC_H_

#include <stdlib.h>
#include <assert.h>

typedef unsigned char byte;

#define REG_API

// for debug
// #define _DEBUG_ 

#ifdef _DEBUG_
  void* reg_malloc(size_t size, char* file, int line);
  void* reg_calloc(size_t count, size_t size, char* file, int line);
  void* reg_realloc(void* p, size_t size, char* file, int line);
  void reg_free(void* p);
  void reg_dump();

  #undef malloc
  #define malloc(size) reg_malloc(size, __FILE__, __LINE__)

  #undef calloc
  #define calloc(count, size) reg_calloc(count, size,  __FILE__, __LINE__)

  #undef realloc
  #define realloc(p, size) reg_realloc(p, size,  __FILE__, __LINE__)

  #undef free
  #define free reg_free
#else
  #define reg_dump(...) 
#endif
  
#endif
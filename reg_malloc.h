#include <stdlib.h>
#include <assert.h>


void reg_painc(const char* str);
void reg_error(const char* format, ...);

#define _DEBUG_

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
#endif
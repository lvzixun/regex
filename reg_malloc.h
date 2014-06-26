#include <stdlib.h>
#include <assert.h>

typedef unsigned char byte;
void reg_painc(const char* str);

#ifdef _DEBUG_
  void* reg_malloc(size_t size);
  void* reg_calloc(size_t count, size_t size);
  void* reg_realloc(void* p, size_t size);
  void reg_free(void* p);
  void reg_dump();

  #undef malloc
  #define malloc reg_malloc

  #undef calloc
  #define calloc reg_calloc

  #undef realloc
  #define realloc reg_realloc

  #undef free
  #define free reg_free
#endif
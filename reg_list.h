#ifndef _REG_LIST_H_
#define _REG_LIST_H_
#include "reg_malloc.h"

struct reg_list* list_new(size_t block, size_t count);
void list_free(struct reg_list* p);
void list_clear(struct reg_list* p);
size_t list_add(struct reg_list* p, void* value);
size_t list_len(struct reg_list* p);
void* list_idx(struct reg_list* p, size_t idx);
void* list_tail(struct reg_list* p);

typedef int(*campar)(const void*, const void*);
void list_sort(struct reg_list* p, campar func);


#endif
#include <string.h>
#include <stdio.h>
#include "reg_list.h"

struct reg_list {
  size_t size;
  size_t len;

  size_t block;
  byte* buf; 
};


struct reg_list* list_new(size_t block, size_t count){
  struct reg_list* ret = malloc(sizeof(struct reg_list));
  ret->size = count;
  ret->block = block;
  ret->len = 0;

  ret->buf = (byte*)calloc(count, block);
  return ret;
}

void list_free(struct reg_list* p){
  free(p->buf);
  free(p);
}

void list_clear(struct reg_list* p){
  p->len = 0;
}


size_t list_add(struct reg_list* p, void* value){
  if(p->len >= p->size){
    p->size *= 2;
    p->buf = (byte*)realloc(p->buf, p->size*p->block);
  }

  byte* dest = &(p->buf[p->len*p->block]);
  memcpy(dest, value, p->block);
  return p->len++;
}

void* list_tail(struct reg_list* p){
  if(p->len >= p->size) return NULL;
  return list_idx(p, p->len);
}

size_t list_len(struct reg_list* p){
  return p->len;
}

void* list_idx(struct reg_list* p, size_t pos){
  if(pos >= p->len) return NULL;
  return &(p->buf[p->block*pos]);
}


void list_sort(struct reg_list* p, campar sort){
  qsort(p->buf, p->len, p->block, sort);
}


void list_sort_subset(struct reg_list* p, size_t begin_idx, size_t len, campar sort){
  void* src  = p->buf + p->block*begin_idx;
  assert(begin_idx+len <= p->len);
  qsort(src, len, p->block, sort);
}

struct reg_list* list_copy(struct reg_list* src){
  assert(src);
  struct reg_list* ret = list_new(src->block, src->size);
  ret->len = src->len;
  memcpy(ret->buf, src->buf, src->block*src->len);
  return ret;
}



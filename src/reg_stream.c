#include "reg_malloc.h"
#include <string.h>
#include "reg_stream.h"

struct reg_stream{
  const unsigned char* buff;
  size_t size;
  size_t pos;
};


 struct reg_stream* stream_new(const unsigned char* str, size_t size){
  struct reg_stream* ret = malloc(sizeof(struct reg_stream));
  ret->buff = str;
  ret->size = size;
  ret->pos = 0;
  return ret;
}


void stream_free(struct reg_stream* p){
  free(p);
}


inline unsigned char stream_char(struct reg_stream* p){
  return p->buff[p->pos];
}

inline int stream_end(struct reg_stream* p){
  return p->pos >= p->size;
}

inline unsigned char stream_next(struct reg_stream* p){
  if(stream_end(p)) return '\0';

  return p->buff[(p->pos)++];
}

inline unsigned char stream_back(struct reg_stream* p){
  if(p->pos == 0) return '\0';

  return p->buff[(p->pos)--];
}

inline size_t stream_pos(struct reg_stream* p){
  return p->pos;
}


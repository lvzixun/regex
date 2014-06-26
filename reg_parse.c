#include <stdio.h>
#include "reg_malloc.h"
#include "reg_stream.h"
#include "reg_parse.h"

#define DEF_NDES_SIZE 128

#define _stream(p) (p)->stream
#define at_char(p) stream_char(_stream(p))
#define is_end(p) stream_end(_stream(p))
#define next_char(p) stream_next(_stream(p))

struct reg_parse {
  struct reg_stream* stream;

  size_t nodes_cap;
  size_t nodes_size;
  struct reg_ast_node nodes[0];
};

static void _parse(struct reg_parse* p);

struct reg_parse* parse_new(){
  struct reg_parse* ret = calloc(1, 
    sizeof(struct reg_parse) + sizeof(struct reg_ast_node)*DEF_NDES_SIZE);
  return ret;
}

void parse_exec(struct reg_parse* p, const unsigned char* str, size_t size){
  p->stream = stream_new(str, size);
  _parse(p);
}

static void _parse(struct reg_parse* p){
  assert(p->stream);
  unsigned char cur_char = 0;

  for(; is_end(p); next_char(p)){
    cur_char = at_char(p);

    switch(cur_char){
      case '[':
        break;
      case '(':
        break;
      case '*':
        break;
      case '|':
        break;
      case '\0':
        break;
      default:
        break;
    }
  }
}

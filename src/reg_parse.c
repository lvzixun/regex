#include <stdio.h>
#include <stdarg.h>
#include "reg_malloc.h"
#include "reg_stream.h"
#include "reg_parse.h"


#define DEF_NDES_SIZE 128

#define _stream(p)          (p)->stream
#define at_char(p)          stream_char(_stream(p))
#define is_end(p)           stream_end(_stream(p))
#define next_char(p)        stream_next(_stream(p))
#define cur_pos(p)          stream_pos(_stream(p))

#define expect_char(p, c)   do{ \
                                int pos = cur_pos(p); \
                                if(next_char(p) != (c)) \
                                  reg_error("expect char: %c at pos: %d\n", (c), pos); \
                            }while(0)   
#define unexpect_char(p)    do{ \
                                reg_error("unexpect char: %c at pos: %d\n", at_char(p), cur_pos(p)); \
                            }while(0)

struct reg_parse {
  struct reg_stream* stream;

  size_t nodes_cap;
  size_t nodes_size;
  struct reg_ast_node* nodes;
};

static struct reg_ast_node* _parse_exp(struct reg_parse* p);
static struct reg_ast_node* _parse_term(struct reg_parse* p);
static struct reg_ast_node* _parse_factor(struct reg_parse* p);
static struct reg_ast_node* _parse_repeated(struct reg_parse* p);

struct reg_parse* parse_new(){
  struct reg_parse* ret = calloc(1, sizeof(struct reg_parse));
  ret->nodes = calloc(1, sizeof(struct reg_ast_node)*DEF_NDES_SIZE);
  ret->nodes_cap =0;
  ret->nodes_size = DEF_NDES_SIZE;
  return ret;
}

void parse_clear(struct reg_parse* p){
  p->nodes_cap = 0;
}

void parse_free(struct reg_parse* p){
  if(p->stream) stream_free(p->stream);
  free(p->nodes);
  free(p);
}


struct reg_ast_node* parse_exec(struct reg_parse* p, const char* rule, size_t size){
  p->stream = stream_new((const unsigned char*)rule, size);
  struct reg_ast_node* ret =  _parse_exp(p);
  if(!is_end(p)) unexpect_char(p);

  return ret;
}


static inline struct reg_ast_node* _new_node(struct reg_parse* p){
  if(p->nodes_cap >= p->nodes_size){
    p->nodes_size *= 2;
    p->nodes = realloc(p->nodes, p->nodes_size * sizeof(struct reg_ast_node));
  }

  struct reg_ast_node* ret = &(p->nodes[p->nodes_cap++]);
  ret->childs[0] = NULL;
  ret->childs[1] = NULL;

  return ret;
}

static inline struct reg_ast_node* _gen_node(struct reg_parse* p, enum reg_op op, ...){
  struct reg_ast_node* ret = _new_node(p);
  ret->op = op;

  va_list args;

  va_start(args, op);
  switch(op){
    case op_range:{
       int begin = va_arg(args, int);
       int end   = va_arg(args, int);
      ret->value.range.begin = begin;
      ret->value.range.end = end;
      }break;
    default:
      break;
  }
  va_end(args);

  return ret;
}

// ------------------------ parse to ast --------------------------------------

// parse or
static struct reg_ast_node* _parse_exp(struct reg_parse* p){
  struct reg_ast_node* root = _parse_term(p);

  for(; !is_end(p); ){
    unsigned char cur_char = at_char(p);
    switch(cur_char){
      case '|':{
        expect_char(p, '|');
        struct reg_ast_node* tmp = _gen_node(p, op_or);
        tmp->childs[0] = root;
        tmp->childs[1] = _parse_term(p);
        if(tmp->childs[0]==NULL || tmp->childs[1]==NULL)
          reg_error("invalid op '|' at pos: %zd\n", cur_pos(p));
        root = tmp;
        }break;
      default:
        return root;
    }
  }

  return root;
}

// parse and 
static struct reg_ast_node* _parse_term(struct reg_parse* p){
  struct reg_ast_node* root = _parse_repeated(p);

  for(; root && !is_end(p); ){
    struct reg_ast_node* right = _parse_repeated(p);
    if(!right) return root;

    struct reg_ast_node* tmp = _gen_node(p, op_and);
    tmp->childs[0] = root;
    tmp->childs[1] = right;
    root = tmp;
  }

  return root;
}

// parse repeated
static struct reg_ast_node* _parse_repeated(struct reg_parse* p){
  struct reg_ast_node* root = _parse_factor(p);
  unsigned char cur_char = at_char(p);

  if(cur_char == '*'){
    struct reg_ast_node* tmp = _gen_node(p, op_rp);
    tmp->childs[0] = root;
    root = tmp;
    next_char(p);
  }
  return root;
}


// parse factors
static struct reg_ast_node* _parse_factor(struct reg_parse* p){
  struct reg_ast_node* ret = NULL;

  unsigned char cur_char = at_char(p);
  switch(cur_char){
    case '[':{
        expect_char(p, '[');
        int begin = next_char(p);
        expect_char(p, '-');
        int end   = next_char(p);
        expect_char(p, ']');
        ret = _gen_node(p, op_range, begin, end);
      }break;
    case '(':
      expect_char(p, '(');
      ret = _parse_exp(p);
      expect_char(p, ')');
      break;

    case ']':
    case ')':
    case '|':
      return NULL;
    default:
      ret = _gen_node(p, op_range, (int)cur_char, (int)cur_char);
      next_char(p);
      break;
  }

  return ret;
}


// for test
static char* op_map[] = {
  "op_nil",
  "op_and",
  "op_or",
  "op_rp",
  "op_range"
};


static void _parse_dump(struct reg_ast_node* root){
  if(!root) return;

  if(root->op == op_range)
    printf("[addr: %p] type: %s  value:[%c - %c] childs[%p, %p] \n", root, op_map[root->op], 
      root->value.range.begin, root->value.range.end,
      root->childs[0], root->childs[1]);
  else
    printf("[addr: %p] type: %s  childs[%p, %p]\n", root, 
      op_map[root->op],
      root->childs[0], root->childs[1]);

  _parse_dump(root->childs[0]);
  _parse_dump(root->childs[1]);
}

void parse_dump(struct reg_ast_node* root){
  printf("-------- parse_dump ----------\n");
  _parse_dump(root);  
}


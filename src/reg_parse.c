#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "reg_malloc.h"
#include "reg_stream.h"
#include "reg_parse.h"

#include "reg_error.h"


#define DEF_NDES_SIZE 128

#define _stream(p)          (p)->stream

#define at_char(p)          stream_char(_stream(p))
#define ahead_char(p, s)    stream_look(_stream(p), stream_pos(_stream(p))+(s))

#define is_end(p)           stream_end(_stream(p))
#define next_char(p)        stream_next(_stream(p))
#define cur_pos(p)          stream_pos(_stream(p))


#define swap(a, b)          do{int tmp = a; a=b; b=tmp;}while(0)

#define expect_char(p, c)   do{ \
                                int pos = cur_pos(p); \
                                if(next_char(p) != (c)) \
                                  reg_error(p->env, "expect char '%c' at pos: %d\n", (c), pos); \
                            }while(0)   
#define unexpect_char(p)    do{ \
                                char s[4] = {0}; \
                                s[0] = at_char(p); \
                                if(s[0] == 0) memcpy(s, "EOF", 3); \
                                reg_error(p->env, "unexpect char '%s' at pos: %d\n", s, cur_pos(p)); \
                            }while(0)


#define is_symbol(p, c)  (!(p->cmask[c]))
static char _escape_char[] = {
  ' ', '\n', '\r', '(',')', 
  '[', ']', '|', '*', '.', 
  '\\', '+', '.', '\t', '-', '$'
};

struct reg_parse {
  char  cmask[0xff];
  struct reg_env* env;

  struct reg_stream* stream;

  size_t nodes_cap;
  size_t nodes_size;
  struct reg_ast_node* nodes;

  int is_match_tail; // is matches the end of string
};

static inline struct reg_ast_node* _escape(struct reg_parse* p);
static struct reg_ast_node* _parse_exp(struct reg_parse* p);
static struct reg_ast_node* _parse_term(struct reg_parse* p);
static struct reg_ast_node* _parse_factor(struct reg_parse* p);
static struct reg_ast_node* _parse_repeated(struct reg_parse* p);

struct reg_parse* parse_new(struct reg_env* env){
  struct reg_parse* ret = calloc(1, sizeof(struct reg_parse));
  ret->env = env;
  ret->nodes = calloc(1, sizeof(struct reg_ast_node)*DEF_NDES_SIZE);
  ret->nodes_cap =0;
  ret->nodes_size = DEF_NDES_SIZE;
  ret->is_match_tail = 0;

  memset(ret->cmask, 0, sizeof(ret->cmask));
  for(int i=0; i<sizeof(_escape_char); i++){
    int c = _escape_char[i];
    assert(c>0 && c<0xff);
    ret->cmask[c] = 1;
  }

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

int parse_is_match_tail(struct reg_parse* p){
  return p->is_match_tail;
}

struct reg_ast_node* parse_exec(struct reg_parse* p, const char* rule, size_t size){
  p->stream = stream_new((const unsigned char*)rule, size);
  struct reg_ast_node* ret =  _parse_exp(p);
  if(!ret || !is_end(p)) unexpect_char(p);

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
          reg_error(p->env, "invalid op '|' at pos: %zd\n", cur_pos(p));
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
  enum reg_op op = op_nil;

  switch(cur_char){
    case '*':
      op = op_rps;
      break;

    case '?': 
      op = op_rpq;
      break;

    case '+':
      op = op_rpp;
      break;

    default:
      return root;
  }

  struct reg_ast_node* tmp = _gen_node(p, op);
  tmp->childs[0] = root;
  root = tmp;
  next_char(p);  
  return root;
}

inline static int _range_factor(struct reg_parse* p){
  int ret = at_char(p);

  printf("_range_factor: %c\n", (char)ret);
  switch(ret){
    case '\\':{
      struct reg_ast_node* ep = _escape(p);
      assert(ep);
      assert(ep->op == op_range);
      if(ep->value.range.begin != ep->value.range.end)
        reg_error(p->env, "invalid escape char at `[]`.");
      ret = ep->value.range.begin;
    }break;

    default:
      if(!is_symbol(p, ret))
        unexpect_char(p);
      next_char(p);
  }

  return ret;
}

inline static struct reg_ast_node* _range_term(struct reg_parse* p){
  struct reg_ast_node* ret = NULL;
  int begin = _range_factor(p);
  int end = begin;

  if(at_char(p) == '-'){
    expect_char(p, '-');
    end = _range_factor(p);
  }

  if(end < begin) swap(end, begin);
  ret = _gen_node(p, op_range, begin, end);
  return ret;
}


static struct reg_ast_node* _parse_range(struct reg_parse* p){
  expect_char(p, '[');

  struct reg_ast_node* ret = _range_term(p);
  unsigned char cur_char = at_char(p);
  while( cur_char && cur_char != ']' ){
    struct reg_ast_node* right = _range_term(p);
    assert(right);
    assert(ret);

    struct reg_ast_node* node = _gen_node(p, op_or);
    node->childs[0] = ret;
    node->childs[1] = right;
    ret = node;
    cur_char = at_char(p);
  }

  expect_char(p, ']');
  return ret;
}


// parse factors
static struct reg_ast_node* _parse_factor(struct reg_parse* p){
  struct reg_ast_node* ret = NULL;

  unsigned char cur_char = at_char(p);
  switch(cur_char){
    case '.':{
      ret = _gen_node(p, op_range, 0, 0xff);
      expect_char(p, '.');
    }break;

    case '$':{
      expect_char(p, '$');
      if(!is_end(p)) unexpect_char(p);
      p->is_match_tail = 1;
    }break;

    case '[':{
      ret = _parse_range(p);
    }break;

    case '(':
      expect_char(p, '(');
      ret = _parse_exp(p);
      expect_char(p, ')');
      break;

    case ')':
    case ']':
    case '|':
      return NULL;

    // escape char
    case '\\':{
      ret = _escape(p);
    }break;

    default:
      if(!is_symbol(p, cur_char))
        unexpect_char(p);

      ret = _gen_node(p, op_range, (int)cur_char, (int)cur_char);
      next_char(p);
      break;
  }

  return ret;
}

static inline struct reg_ast_node* _escape(struct reg_parse* p){
  struct reg_ast_node* ret = NULL;
  expect_char(p, '\\');
  int cur_char = at_char(p);

  switch(cur_char){
    case 's':{  
      int escape_char = ' '; 
      ret = _gen_node(p, op_range, escape_char, escape_char);
      next_char(p);
    }break;

    case '\\':{
      int escape_char = '\\'; 
      ret = _gen_node(p, op_range, escape_char, escape_char);
      next_char(p);
    }break;

    case 'r':{
      int escape_char = '\r'; 
      ret = _gen_node(p, op_range, escape_char, escape_char);
      next_char(p);
    }break;

    case 'n':{ 
      int escape_char = '\n'; 
      ret = _gen_node(p, op_range, escape_char, escape_char);
      next_char(p);
    }break;

    case 't':{
      int escape_char = '\t'; 
      ret = _gen_node(p, op_range, escape_char, escape_char);
      next_char(p);
    }break;

    case '(':
    case ')':
    case '[':
    case ']':
    case '-':
    case '.':
    case '$':
    case '+':{ 
      ret = _gen_node(p, op_range, cur_char, cur_char);
      next_char(p);
    }break;

    case 'd':{
      ret = _gen_node(p, op_range, '0', '9');
      next_char(p);
    }break;

    default:  unexpect_char(p);
  }

  return ret;
}

// for test
static char* op_map[] = {
  "op_nil",
  "op_and",
  "op_or",

  "op_rps",
  "op_rpp",
  "op_rpq",
  
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


#ifndef _REG_PARSE_H_
#define _REG_PARSE_H_

#include <stdlib.h>

struct reg_env;
struct reg_parse;

enum reg_op {
  op_nil,    // nil op

  op_and,   // and op
  op_or,    // or op

  op_rps,    // match 0 or more times
  op_rpp,    // match 1 or more times
  op_rpq,   // match 1 or 0 more time

  op_range, // range op

  op_count
};

struct reg_range {
  int begin;
  int end;
};

struct reg_ast_node {
  enum reg_op op;
  union {
    struct reg_range range;
  }value;

  struct reg_ast_node* childs[2];
};



struct reg_parse* parse_new(struct reg_env* env);
void parse_clear(struct reg_parse* p);
void parse_free(struct reg_parse* p);
struct reg_ast_node* parse_exec(struct reg_parse* p, const char* rule, size_t size);
int parse_is_match_tail(struct reg_parse* p);

// for test
void parse_dump(struct reg_ast_node* root);
#endif


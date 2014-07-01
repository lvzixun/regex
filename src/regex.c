#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_state.h"

#include <string.h>
#include "regex.h"

struct reg_env {
  struct reg_parse* parse_p;
  struct reg_state* state_p;
};


REG_API struct reg_env* reg_open_env(){
  struct reg_env* env = (struct reg_env*)malloc(sizeof(struct reg_env));
  env->parse_p = parse_new();
  env->state_p = state_new();
  return env;
}

REG_API void reg_close_env(struct reg_env* env){
  parse_free(env->parse_p);
  state_free(env->state_p);
  free(env);
}

REG_API struct reg_filter* reg_enw_filter(struct reg_env* env, const char* rule){
  if(rule == NULL || env == NULL) return NULL;

  parse_clear(env->parse_p);
  // todo :set exception handling
  
  struct reg_ast_node* root = parse_exec(env->parse_p, rule, strlen(rule));
  struct reg_filter* filter = state_new_filter(env->state_p, root);
  return filter;
}

REG_API void reg_free_filter(struct reg_filter* filter){
  state_free_filter(filter);
}


REG_API int reg_get_capture(struct reg_filter* filter, const unsigned char* source, int len, struct reg_capture* out_capture){
  return 0;
}
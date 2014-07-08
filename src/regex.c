#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_state.h"
#include "state_filter.h"

#include <string.h>
#include "reg_error.h"
#include "regex.h"



struct reg_env {
  struct reg_longjump* exception_chain;

  struct reg_parse* parse_p;
  struct reg_state* state_p;
};


inline struct reg_longjump** reg_get_exception(struct reg_env* env){
  return &(env->exception_chain);
}


REG_API struct reg_env* reg_open_env(){
  struct reg_env* env = (struct reg_env*)malloc(sizeof(struct reg_env));
  env->exception_chain = NULL;
  env->parse_p = parse_new(env);
  env->state_p = state_new(env);
  return env;
}

REG_API void reg_close_env(struct reg_env* env){
  parse_free(env->parse_p);
  state_free(env->state_p);
  free(env);
}

struct _filter_arg{
  struct reg_env* env;
  const char* rule;
  struct reg_filter* filter;
};

static void _pgen_filter(struct _filter_arg* argv){
  struct reg_ast_node* root = parse_exec(argv->env->parse_p, argv->rule, strlen(argv->rule));
  argv->filter = state_new_filter(argv->env->state_p, root);
}

REG_API struct reg_filter* reg_new_filter(struct reg_env* env, const char* rule){
  if(rule == NULL || env == NULL) return NULL;

  parse_clear(env->parse_p);
  
  // set exception handling
  struct _filter_arg argv = {
    .env = env,
    .rule = rule,
    .filter = NULL,
  };

  if(reg_cpcall(env, (pfunc)_pgen_filter, &argv)){
    return NULL;
  }
  
  return argv.filter;
}

REG_API void reg_free_filter(struct reg_filter* filter){
  state_free_filter(filter);
}


REG_API int reg_match(struct reg_filter* filter, const char* source, int len){
  return state_match(filter, source, len);
}



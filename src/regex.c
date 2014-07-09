#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_state.h"
#include "state_pattern.h"

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

struct _pattern_arg{
  struct reg_env* env;
  const char* rule;
  struct reg_pattern* pattern;
};

static void _pgen_pattern(struct _pattern_arg* argv){
  struct reg_ast_node* root = parse_exec(argv->env->parse_p, argv->rule, strlen(argv->rule));
  argv->pattern = state_new_pattern(argv->env->state_p, root);
}

REG_API struct reg_pattern* reg_new_pattern(struct reg_env* env, const char* rule){
  if(rule == NULL || env == NULL) return NULL;

  parse_clear(env->parse_p);
  
  // set exception handling
  struct _pattern_arg argv = {
    .env = env,
    .rule = rule,
    .pattern = NULL,
  };

  if(reg_cpcall(env, (pfunc)_pgen_pattern, &argv)){
    return NULL;
  }
  
  return argv.pattern;
}

REG_API void reg_free_pattern(struct reg_pattern* pattern){
  state_free_pattern(pattern);
}


REG_API int reg_match(struct reg_pattern* pattern, const char* source, int len){
  return state_match(pattern, source, len);
}



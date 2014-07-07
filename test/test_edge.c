#include "../src/reg_parse.h"
#include "../src/reg_state.h"
#include "../src/reg_malloc.h"
#include "../src/regex.h"

#include <string.h>
#include <stdio.h>

int main(int argc, char const *argv[]){
  assert(argc>=3);
  const char* rule = argv[1];
  const char* source = argv[2];

  // rule = "(a|a)*";
  // source = "a";

  printf("rule: %s\n", rule);
  printf("source: %s\n", source);

  struct reg_env* env = reg_open_env();

  struct reg_parse* p = parse_new(env);
  struct reg_state* s = state_new(env);

  struct reg_ast_node* root = parse_exec(p, rule, strlen((const char*)rule));
  parse_dump(root);

  struct reg_filter* filter = state_new_filter(s, root);
  dump_frame(s);
  dump_filter(filter);

  struct reg_capture cap = {0};
  printf("------------ capture --------------\n");
  int success = reg_get_capture(filter, source, strlen(source), &cap);
  printf("----------- reg_capture -------------\n");
  printf("success: %d\n", success);

  state_free_filter(filter);
  parse_free(p);
  state_free(s);

  reg_close_env(env);

  reg_dump(); // print memory leakly
  return 0;
}
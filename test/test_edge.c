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

  struct reg_pattern* pattern = state_new_pattern(s, root, parse_is_match_tail(p));
  dump_frame(s);
  dump_pattern(pattern);

  int success = reg_match(pattern, source, strlen(source));
  printf("----------- reg_match -------------\n");
  printf("success: %d\n", success);

  state_free_pattern(pattern);
  parse_free(p);
  state_free(s);

  reg_close_env(env);

  reg_dump(); // print memory leakly
  return 0;
}
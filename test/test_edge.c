#include "../src/reg_parse.h"
#include "../src/reg_state.h"
#include "../src/reg_malloc.h"

#include <string.h>
#include <stdio.h>

int main(int argc, char const *argv[]){
  assert(argc>=2);
  const char* rule = argv[1];
  printf("rule: %s\n", rule);

  struct reg_parse* p = parse_new();
  struct reg_state* s = state_new();

  struct reg_ast_node* root = parse_exec(p, rule, strlen((const char*)rule));
  parse_dump(root);

  struct reg_filter* filter = state_new_filter(s, root);
  dump_frame(s);
  dump_filter(filter);
  
  state_free_filter(filter);
  parse_free(p);
  state_free(s);

  reg_dump(); // print memory leakly
  return 0;
}
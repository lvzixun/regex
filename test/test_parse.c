#include <stdio.h>

#include <string.h>
#include "../src/reg_parse.h"
#include "../src/reg_malloc.h"


int main(int argc, char const *argv[]){
  assert(argc>=2);
  const char* rule = argv[1];

  printf("rule: %s\n", rule);

  struct reg_parse* p = parse_new();
  struct reg_ast_node* root = parse_exec(p, rule, strlen((const char*)rule));
  parse_dump(root);
  parse_free(p);

  reg_dump(); // print memory leakly
  return 0;
}
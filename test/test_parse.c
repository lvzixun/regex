#include <stdio.h>

#include <string.h>
#include "../reg_parse.h"
#include "../reg_malloc.h"


int main(int argc, char const *argv[]){
  assert(argc>=2);

  struct reg_parse* p = parse_new();
  struct reg_ast_node* root = parse_exec(p, (const unsigned char*)argv[1], strlen(argv[1]));
  parse_dump(root);
  parse_free(p);

  reg_dump(); // print memory leakly
  return 0;
}
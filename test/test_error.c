#include "../src/reg_malloc.h"
#include "../src/regex.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char const *argv[]){
  const char* rule = "ab[a-";

  printf("rule: %s\n", rule);
  struct reg_env* env = reg_open_env();


  struct reg_pattern* _ef = reg_new_pattern(env, rule);
  if(_ef) reg_free_pattern(_ef);


  reg_close_env(env);
  reg_dump(); // print memory leakly
  return 0;
}
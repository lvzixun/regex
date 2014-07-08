#include "../src/regex.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(int argc, char const *argv[]){
  assert(argc>=3);
  const char* rule = argv[1];
  const char* source = argv[2];

  printf("rule: %s\n", rule);
  printf("source: %s\n", source);

  struct reg_env* env = reg_open_env();
  
  struct reg_filter* filter = reg_new_filter(env, rule);
  int success = reg_match(filter, source, strlen(source));
  printf("-------------- reslut -----------\n success: %d\n", success);
  reg_close_env(env);
  return 0;
}
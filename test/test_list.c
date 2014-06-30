#include <stdio.h>
#include "../src/reg_malloc.h"
#include "../src/reg_list.h"

int main(int argc, char const *argv[]){
  struct reg_list* list = list_new(sizeof(int), 1);

  int a[] = {1, 2, 3, 4};

  list_add(list, a);
  list_add(list, a+1);
  list_add(list, a+3);
  list_add(list, a+4);

 // list_free(list);
  reg_dump(); // print memory leakly
  return 0;
}
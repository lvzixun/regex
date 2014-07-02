#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>
#include <assert.h>

#include "regex.h"
#include "reg_error.h"

struct reg_longjump {
  jmp_buf buf;
  int status;
  struct reg_longjump* next;
};

inline struct reg_longjump** reg_get_exception(struct reg_env* env);

void reg_painc(const char* str){
  printf("<painc>: %s\n", str);
  exit(0);
}

void reg_error(struct reg_env* env, const char* format, ...){
  char buf[0xff] = {0};
  va_list args;

  va_start(args, format);
  vsnprintf(buf, sizeof(buf)-1, format, args);
  va_end(args);

  struct reg_longjump* chain = *reg_get_exception(env);
  
  if(!chain){ // do painc
    reg_painc(buf);
  }else{
    printf("%s", buf);
    chain->status = 1; // throw exception
    longjmp(chain->buf, 1);
  }
}

int reg_cpcall(struct reg_env* env, pfunc func, void* argv) {
  assert(func);
  struct reg_longjump exception;
  struct reg_longjump** chain = reg_get_exception(env);
  exception.next = *chain;
  exception.status = 0;
  *chain = &exception;

  if(setjmp(exception.buf) == 0){
    func(argv);
  }

  *chain = exception.next;
  return exception.status;
}



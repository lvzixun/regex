#ifndef _REG_ERROR_H_
#define _REG_ERROR_H_


struct reg_longjump;
struct reg_env;

void reg_painc(const char* str);
void reg_error(struct reg_env* env, const char* format, ...);

typedef void(*pfunc)(void* argv);
int reg_cpcall(struct reg_env* env, pfunc func, void* argv);
#endif
#ifndef _REGEX_H_
#define _REGEX_H_
#include <stddef.h>

struct reg_env;
struct reg_filter;

struct reg_env* reg_open_env();
void reg_close_env(struct reg_env* env);

struct reg_filter* reg_new_filter(struct reg_env* env, const char* rule);
void reg_free_filter(struct reg_filter* filter);

int reg_match(struct reg_filter* filter, const char* source, int len);

#endif
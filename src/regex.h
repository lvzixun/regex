#ifndef _REGEX_H_
#define _REGEX_H_
#include <stddef.h>

struct reg_env;
struct reg_pattern;

struct reg_env* reg_open_env();
void reg_close_env(struct reg_env* env);

struct reg_pattern* reg_new_pattern(struct reg_env* env, const char* rule);
void reg_free_pattern(struct reg_pattern* pattern);

int reg_match(struct reg_pattern* pattern, const char* source, int len);

#endif
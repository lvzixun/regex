regex
=====

a pure C implementation of simple regular express engine.

Just for fun. ;)

## pattern
| pattern | describe |
|:-------:|:--------:|
|  `ab`   | matches `a` followed by `b` |
| <img src="http://i.imgur.com/j2GTlK2.png" height="30" width="60" />   | matches `a` or `b` (ordered) |
|  `a*`   | matches at least 0 repetitions of `a` |
|  `[a-b]`| matches any characters between `a` and `b` (range) |

## api
~~~~.c
// create/free a regex environment.
struct reg_env* reg_open_env();
void reg_close_env(struct reg_env* env);

// create/free a regex pattern(filter).
struct reg_filter* reg_new_filter(struct reg_env* env, const char* rule);
void reg_free_filter(struct reg_filter* filter);

// match pattern, return a boolean value.
int reg_match(struct reg_filter* filter, const char* source, int len);
~~~~
see `src/regex.h` headfile for detail.


## example
~~~~.c
  // create env
  struct reg_env* env = reg_open_env();
  
  // create pattern
  struct reg_filter* filter = reg_new_filter(env, rule);

  // match pattern
  int success = reg_match(filter, source, strlen(source));

  // free env
  reg_close_env(env);
~~~~
see `test/test_api.c` testcase for detail.


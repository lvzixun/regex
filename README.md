regex
=====

a pure C implementation of simple regular express engine, using DFA engine.
the former is just for yes-or-no matching.


that's all begin from [正则表达式实现](http://airtrack.me/posts/2013/07/05/%E6%AD%A3%E5%88%99%E8%A1%A8%E8%BE%BE%E5%BC%8F%E5%AE%9E%E7%8E%B0%EF%BC%88%E4%B8%80%EF%BC%89)

Just for fun. ;)

## pattern
| pattern | describe |
|:-------:|:--------:|
|   `$`   | matches the end of the string | 
|  `ab`   | matches `a` followed by `b` |
| <img src="http://i.imgur.com/j2GTlK2.png" height="30" width="60" />   | matches `a` or `b` (ordered) |
|  `a*`   | matches at least 0 repetitions of `a` |
|  `a?`   | matches 0 or 1 repetitions of `a` |
|  `a+`   | matches 1 or more repetitions of `a` |
|  `(...)`  | matches whatever regex is inside the parentheses, as a single element|
|  `[a-b]`| matches any characters between `a` and `b` (range) |
|  `.`    | matches any characters, equivalent to the set `[0-0xff]` |
|   `\d`  | matches any decimal digit, equivalent to the set `[0-9]`|
|  `\`    | escapes special characters|

the following escaping sequences are supported:

| escapes | describe |
|:-------:|:--------:|
|   `\t`  |  tab |
|   `\n`  | newline |
|   `\r`  | return |
|   `\s`  |  space |
|   `\\`  |  `\`   |
|   `\(`  |  `(`   |
|   `\)`  |  `)`   |
| `\[`    |   `[`  |
| `\]`    |   `]`  |
|  `\-`   |   `-`  |
|  `\.`   |   `.`  |
|   `\+`  |   `+`  |
|   `\$`  |   `$`  |

`^` (matches the start of the string) is enabled by default.
 
## todo list

1. subset DFA
2. backreference
3. capture

## api
~~~~.c
// create/free a regex environment.
struct reg_env* reg_open_env();
void reg_close_env(struct reg_env* env);

// create/free a regex pattern.
struct reg_pattern* reg_new_pattern(struct reg_env* env, const char* rule);
void reg_free_pattern(struct reg_pattern* pattern);

// match pattern, return a boolean value.
int reg_match(struct reg_pattern* pattern, const char* source, int len);
~~~~
see `src/regex.h` headfile for detail.


## example
~~~~.c
  // create env
  struct reg_env* env = reg_open_env();
  
  // create pattern
  struct reg_pattern* pattern = reg_new_pattern(env, rule);

  // match pattern
  int success = reg_match(pattern, source, strlen(source));

  // free env
  reg_close_env(env);
~~~~
see `test/test_api.c` testcase for detail.


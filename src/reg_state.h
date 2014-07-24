#ifndef _REG_STATE_H_
#define _REG_STATE_H_

struct reg_state;
struct reg_pattern;

struct reg_state* state_new(struct reg_env* env);
void state_free(struct reg_state* p);
void state_clear(struct reg_state* p);

struct reg_pattern* state_new_pattern(struct reg_state* p, struct reg_ast_node* ast, int is_match_tail);
void state_free_pattern(struct reg_pattern* pattern);

// for test
void dump_pattern(struct reg_pattern* p);
void dump_frame(struct reg_state* p);

#endif
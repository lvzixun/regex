#ifndef _REG_STATE_H_
#define _REG_STATE_H_

struct reg_state;
struct reg_filter;

struct reg_state* state_new();
void state_free(struct reg_state* p);
void state_clear(struct reg_state* p);

struct reg_filter* state_new_filter(struct reg_state* state, struct reg_ast_node* ast);
void state_free_filter(struct reg_filter* filter);

// for test
void dump_filter(struct reg_filter* p);
void dump_frame(struct reg_state* p);

#endif
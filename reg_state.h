#ifndef _REG_STATE_H_
#define _REG_STATE_H_

struct reg_state;

struct reg_state* state_new();
void state_free(struct reg_state* p);
void state_clear(struct reg_state* p);
void state_gen(struct reg_state* p, struct reg_ast_node* ast);


// for test
void dump_edge(struct reg_state* p);
void dump_frame(struct reg_state* p);

#endif
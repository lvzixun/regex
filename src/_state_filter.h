#ifndef _REG_STATE_FILTER_
#define _REG_STATE_FILTER_

#include "reg_parse.h"
/*
  the interior private headfile
*/

#define DEF_EDGE 4

#define DEF_EDGES  128
#define DEF_NODES  DEF_EDGES*DEF_EDGE
#define DEF_FRAMES DEF_EDGES*2

struct reg_edge {
  struct reg_range range;
};

//  the posation is index add 1
struct _reg_path {
  size_t edge_pos;      // the posation of edges_list
  size_t next_node_pos; // the posation of state_list
};

//  state node
struct reg_node {
  size_t node_pos;           // the posation of state_list  

  struct reg_list* edges; // the struct reg_path object list
};


struct reg_filter{
  struct reg_state* state;

  size_t start_state_pos;
  size_t end_state_pos;

  struct reg_list* state_list; // the struct reg_node object list
  struct reg_list* edges_list; // the struct reg_edge object list
};

void state_gen(struct reg_filter* filter, struct reg_ast_node* ast);
#endif

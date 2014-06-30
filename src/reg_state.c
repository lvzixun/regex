#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_list.h"
#include <string.h>
#include <stdio.h>

#define DEF_EDGE 4

#define DEF_EDGES  128
#define DEF_NODES  DEF_EDGES*DEF_EDGE
#define DEF_FRAMES DEF_EDGES*2

#define frame_idx(p, idx)       ((struct _range_frame*)list_idx(p->frame_list, idx))


struct reg_edge {
  struct reg_range range;
};

struct _range_frame {
  enum {
    e_begin,
    e_end
  }type;

  int value[2];  // save begin value and end value
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


struct reg_state{

  struct reg_list* frame_list; // the struct _reg_frame object list
};

struct reg_filter{
  struct reg_state* state;

  struct reg_list* state_list; // the struct reg_node object list
  struct reg_list* edges_list; // the struct reg_edge object list
};

static void _gen_frame(struct reg_state* p, struct reg_ast_node* root);
static void _gen_edge(struct reg_filter* filter);

static size_t _gen_nfa(struct reg_filter* filter, struct reg_ast_node* root);
static size_t _gen_dfa(struct reg_filter* filter, size_t start_state_pos);

static inline int _node_insert(struct reg_filter* filter, size_t node_pos, struct reg_range* range, size_t dest_pos);
static size_t _gen_op(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_and(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_or(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_rp(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_range(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);

struct reg_state* state_new(){
  struct reg_state* ret = malloc(sizeof(struct reg_state));

  // init frame
  ret->frame_list = list_new(sizeof(struct _range_frame), DEF_FRAMES);
  return ret;
}

void state_free(struct reg_state* p){
  // free frame
  list_free(p->frame_list);

  free(p);
}

void state_clear(struct reg_state* p){

}

static inline struct reg_filter* _new_filter(struct reg_state* state){
  struct reg_filter* ret = malloc(sizeof(struct reg_filter));
  ret->state = state;
  ret->edges_list = list_new(sizeof(struct reg_edge), DEF_EDGES);

  ret->state_list = list_new(sizeof(struct reg_node), DEF_NODES);

  return ret;
}


static inline int _campar(const struct _range_frame* a, const struct _range_frame* b){
  return a->value[0] - b->value[0];
}

struct reg_filter* state_new_filter(struct reg_state* p, struct reg_ast_node* ast){
  struct reg_filter* filter = _new_filter(p);

  // prepare frame list
  list_clear(p->frame_list);
  _gen_frame(p, ast);
  list_sort(p->frame_list, (campar)_campar);

  // prepare edge list
  list_clear(filter->edges_list);
  _gen_edge(filter);

  // prepare state map
  list_clear(filter->state_list);
  size_t start_state_pos =  _gen_nfa(filter, ast);
  _gen_dfa(filter, start_state_pos);
  return filter;
}

void state_free_filter(struct reg_filter* filter){
  struct reg_node* node = NULL;
  // free state
  for(int i=0; (node = list_idx(filter->state_list, i)) != NULL; i++){
    list_free(node->edges);
  }
  list_free(filter->state_list);

  // free edges
  list_free(filter->edges_list);
  free(filter);
}


// ------------------------------ generate edge  ------------------------------

static void _gen_frame(struct reg_state* p, struct reg_ast_node* root){
  if(!root) return;

  if(root->op == op_range){
    assert(root->childs[0] == NULL);
    assert(root->childs[1] == NULL);
    int begin = root->value.range.begin;
    int end   = root->value.range.end;

    struct _range_frame frame1 = {
      .type = e_begin,
      .value = {begin, end},
    };

    struct _range_frame frame2 = {
      .type = e_end,
      .value = {end, begin},
    };

    list_add(p->frame_list, &frame1);
    list_add(p->frame_list, &frame2);
    return;
  }

  _gen_frame(p, root->childs[0]);
  _gen_frame(p, root->childs[1]);
}

static inline void _insert_edge(struct reg_filter* filter, int begin, int end){
  struct reg_edge value = {
    .range.begin = begin,
    .range.end = end,
  };

  list_add(filter->edges_list, &value);
}

static inline int _need_insert(struct reg_state* p, size_t idx){
  int value = frame_idx(p, idx)->value[0];
  int is_begin = 0, is_end = 0;

  struct _range_frame* v = NULL;
  for(;v = frame_idx(p, idx), v && value == v->value[0]; idx++){
    if(v->type == e_begin)
      is_begin = 1;
    if(v->type == e_end)
      is_end = 1;

    if(v->value[0] == v->value[1] || (is_begin && is_end))
      return 1;
  }
  return 0;
}

static inline int _read_frame(struct reg_filter* filter, size_t idx, 
  int* out_range_right, int* out_next_begin){
  
  int count = 0;
  assert(out_range_right);
  int value = frame_idx(filter->state, idx)->value[0];
  
  int is_insert = 0;
  int is_begin = 0, is_end = 0;

  *out_next_begin = value;

  struct _range_frame* v = NULL;
  for(;  
      v = frame_idx(filter->state, idx), v && value == v->value[0]; 
      idx++, count++){  

    // record end node 
    if(v->type == e_begin){
      is_begin = 1;
      int end = v->value[1];
      if(*out_range_right < end)
        *out_range_right = end;
    }

    if(v->type == e_end){
      is_end = 1;
    }

    // single range
    if((v->value[0] == v->value[1] || (is_begin && is_end)) && !is_insert){
      (*out_next_begin)++;
      _insert_edge(filter, v->value[0], v->value[0]);
      is_insert = 1;
    }
  }

  return count;
}

static void _gen_edge(struct reg_filter* filter){
  int range_right = 0;
  int head = 0;
  size_t i = _read_frame(filter, 0, &range_right, &head);

  struct _range_frame* frame = NULL;
  for(; (frame = list_idx(filter->state->frame_list, i)); ){
    int tail = frame->value[0];
    if(_need_insert(filter->state, i) || frame->type == e_begin){
      tail -= 1;
    }
    if(head <= tail && tail <= range_right){
      _insert_edge(filter, head, tail);
    }

    int count = _read_frame(filter, i, &range_right, &head);
    if(head == tail) head++;
    i += count;
  }
}


// ------------------------------ generate node ------------------------------
static inline size_t _node_new(struct reg_filter* filter){
  size_t len = list_len(filter->state_list);
  struct reg_node v = {
    .node_pos = len+1,
    .edges = list_new(sizeof(struct _reg_path), DEF_EDGE),
  };
  size_t idx = list_add(filter->state_list, &v);
  return idx + 1;
}

static inline struct reg_node* _node_pos(struct reg_filter* filter, size_t pos){
  struct reg_node* ret = list_idx(filter->state_list, pos -1);
  return ret;
}

static inline void __node_insert(struct reg_filter* filter, size_t node_pos, struct _reg_path* path){
  assert(path);
  struct reg_node* node = _node_pos(filter, node_pos);
  list_add(node->edges, path);
}

static inline int _node_insert(struct reg_filter* filter, size_t node_pos, struct reg_range* range, size_t dest_pos){
  int count = 0;
  struct _reg_path path = {0};

  // insert epsilon edge
  if(range ==NULL){
    path.edge_pos = 0;
    path.next_node_pos = dest_pos;
    __node_insert(filter, node_pos, &path);
    return 0;
  }

  struct reg_edge* edge = NULL;
  for(size_t i = 0; (edge = list_idx(filter->edges_list, i)); i++){
    if(edge->range.begin >= range->begin && edge->range.end <= range->end){
      path.edge_pos = i + 1;
      path.next_node_pos = dest_pos;
      __node_insert(filter, node_pos, &path);
      count++;
    }
  }

  return count;
}

static size_t _gen_nfa(struct reg_filter* filter, struct reg_ast_node* root){
  size_t head = _node_new(filter);
  return _gen_op(filter, root, head);
}

static size_t _gen_dfa(struct reg_filter* filter, size_t start_state_pos){
  return 0;
}

static size_t _gen_op(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){
  switch(root->op){
    case op_and:
      return _gen_op_and(filter, root, start_state_pos);
    case op_or:
      return _gen_op_or(filter, root, start_state_pos);
    case op_rp:
      return _gen_op_rp(filter, root, start_state_pos);
    case op_range:
      return _gen_op_range(filter, root, start_state_pos);
    default:
      assert(0);
  }
  assert(0);
  return 0;
}

/*
  nfa state map:
  rule: ab
  --ε-->
  <start> --a--> <s1> --b--> <end> 
*/
static size_t _gen_op_and(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){
  assert(root->op == op_and);

  size_t left_end_pos = _gen_op(filter, root->childs[0], start_state_pos);
  size_t right_end_pos = _gen_op(filter, root->childs[1], left_end_pos);
  return right_end_pos;
}


/*
  nfa state map:
  rule: a|b
     |----a----> <s1> ---ε-------|
     |                           ∨
  <start> --b--> <s2> ---ε---> <end>
*/

static size_t _gen_op_or(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){ 
  assert(root->op == op_or);

  size_t s1_pos = _gen_op(filter, root->childs[0], start_state_pos);
  size_t s2_pos = _gen_op(filter, root->childs[1], start_state_pos);

  size_t end_pos = _node_new(filter);
  _node_insert(filter, s1_pos, NULL, end_pos);
  _node_insert(filter, s2_pos, NULL, end_pos);
  return end_pos;
}

/*
  nfa state map:
  rule a*
    
    |------ε-------|
    ∨              |
  <start> --a--> <end> 
*/
static size_t _gen_op_rp(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){

  size_t end_pos = _gen_op(filter, root->childs[0], start_state_pos);
  _node_insert(filter, end_pos, NULL, start_state_pos);
  return end_pos;
}


/*
  nfa state map:
  rule: [a-b]
  <start> --[a-b]--> <end>
*/
static size_t _gen_op_range(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){
  assert(root->op == op_range);
  struct reg_range* range = &(root->value.range);

  size_t end_pos = _node_new(filter);

  int is_insert  = 0;
  is_insert = _node_insert(filter, start_state_pos, range, end_pos);

  assert(is_insert);
  return end_pos;
}






// ------------------------------  for test  ------------------------------
void dump_filter(struct reg_filter* p){
  struct reg_edge* v = NULL;
  printf("------ dump_filter_edge --------\n");
  for(size_t i=0; (v = list_idx(p->edges_list, i)); i++){
    printf("[%c - %c] ", v->range.begin, v->range.end);
  }
  printf("\n");  
}

void dump_frame(struct reg_state* p){
 struct _range_frame* v = NULL;
 printf("-------- dump_frame ----------\n");
 for(size_t i=0; (v = list_idx(p->frame_list, i)); i++){
  printf("value: %c type: %d  ", v->value[0], v->type);
 }
 printf("\n");
}




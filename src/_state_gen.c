/*
  construction nfa and convert nfa to dfa
*/

#include <stdio.h>
#include <string.h>

#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_list.h"
#include "_state_filter.h"
#include "reg_stream.h"

#include "regex.h"

static size_t _gen_nfa(struct reg_filter* filter, struct reg_ast_node* root);
static size_t _gen_dfa(struct reg_filter* filter, size_t start_state_pos);

static inline int _node_insert(struct reg_filter* filter, size_t node_pos, struct reg_range* range, size_t dest_pos);
static inline struct reg_node* _node_pos(struct reg_filter* filter, size_t pos);
static inline size_t _node_new(struct reg_filter* filter);
static size_t _gen_op(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_and(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_or(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_rp(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_range(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos);

static int _pass_state(struct reg_filter* filter, size_t node_pos, struct reg_stream* source, struct reg_capture* cap);

void state_gen(struct reg_filter* filter, struct reg_ast_node* ast){
  list_clear(filter->state_list);
  filter->end_state_pos = _gen_nfa(filter, ast);
  _gen_dfa(filter, filter->start_state_pos);
}

int state_capture(struct reg_filter* filter, const char* s, int len, struct reg_capture* cap){
  struct reg_stream* source = stream_new((const unsigned char*)s, len);
  int success = _pass_state(filter, filter->start_state_pos, source, cap);
  stream_free(source);
  return success;
}

static size_t _gen_nfa(struct reg_filter* filter, struct reg_ast_node* root){
  size_t head = _node_new(filter);
  filter->start_state_pos = head;
  return _gen_op(filter, root, head);
}

static size_t _gen_dfa(struct reg_filter* filter, size_t start_state_pos){
  return 0;
}

//------------------------------ check char at edge ------------------------------
static inline struct reg_edge* _edge_pos(struct reg_filter* filter, size_t pos){
  if(pos == 0) return NULL; // is edsilone edege  
  struct reg_edge* edge = list_idx(filter->edges_list, pos-1);
  return edge;
}


static int _pass_state(struct reg_filter* filter, size_t node_pos, struct reg_stream* source, struct reg_capture* cap){
  #ifdef _DEBUG_
    printf("_pass_state: %zd\n", node_pos);
  #endif

  // pass ned state
  if(stream_end(source) && filter->end_state_pos == node_pos){ 
    size_t idx = stream_pos(source);
    cap->offset = idx - cap->head;
    return 1;
  }

  // dump edge
  struct reg_list* edges = _node_pos(filter, node_pos)->edges;
  struct _reg_path* path = NULL;
  unsigned char c = stream_char(source);

  for(size_t i=0; (path = list_idx(edges, i)); i++){
    struct reg_range* range = &(_edge_pos(filter, path->edge_pos)->range);
    size_t next_node_pos = path->next_node_pos;

    int success = 0;
    if(range == NULL){  //edsilone
      success = _pass_state(filter, next_node_pos, source, cap);
    }else if(c >= range->begin && c<=range->end){ // range
      stream_next(source);
      success = _pass_state(filter, next_node_pos, source, cap);
      stream_back(source);
    }

    if(success) return 1; 
  }

  return 0;
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
    if(count>0 && range->begin > edge->range.end) break;

    if(edge->range.begin >= range->begin && edge->range.end <= range->end){
      path.edge_pos = i + 1;
      path.next_node_pos = dest_pos;
      __node_insert(filter, node_pos, &path);
      count++;
    }
  }

  return count;
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


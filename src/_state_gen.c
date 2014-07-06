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

#define foreach_edge(v, node, filter) struct _reg_path* v = NULL; \
                                      for(size_t i=0; (v = list_idx(node->edges, i)); i++)

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

static int __move(struct reg_filter* filter, struct reg_list* subset, size_t edge_pos, struct reg_list* out_subset);

static void _dump_subset(struct reg_filter* filter);


void state_gen(struct reg_filter* filter, struct reg_ast_node* ast){
  list_clear(filter->state_list);
  filter->start_state_pos = _gen_nfa(filter, ast);
  filter->dfa_start_state_pos = _gen_dfa(filter, filter->start_state_pos);
}

int state_capture(struct reg_filter* filter, const char* s, int len, struct reg_capture* cap){
  struct reg_stream* source = stream_new((const unsigned char*)s, len);
  int success = _pass_state(filter, filter->dfa_start_state_pos, source, cap);
  stream_free(source);
  return success;
}

static size_t _gen_nfa(struct reg_filter* filter, struct reg_ast_node* root){
  size_t head = _node_new(filter);
  size_t end = _gen_op(filter, root, head);
  struct reg_node* node = _node_pos(filter, end);
  node->is_end = 1;
  return head;
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
  if(stream_end(source) && _node_pos(filter, node_pos)->is_end){ 
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


// ------------------------------ generate nfa node ------------------------------
static inline size_t _node_new(struct reg_filter* filter){
  size_t len = list_len(filter->state_list);
  struct reg_node v = {
    .node_pos = len+1,
    .is_end = 0,
    .subset_tag = 0,
    .subset = NULL,
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
  <start> --a--> <s1> --ε--> <s2> --b--> <end> 
*/
static size_t _gen_op_and(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){
  assert(root->op == op_and);

  size_t s1_pos = _gen_op(filter, root->childs[0], start_state_pos);
  
  size_t s2_pos = _node_new(filter);
  _node_insert(filter, s1_pos, NULL, s2_pos);

  size_t end = _gen_op(filter, root->childs[1], s2_pos);
  return end;
}


/*
  nfa state map:
  rule: a|b
     |----ε----> <s1> --a--> <s2> ---ε-------|
     |                                       ∨
  <start> --ε--> <s3> --b--> <s4> ---ε---> <end>
*/

static size_t _gen_op_or(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){ 
  assert(root->op == op_or);

  size_t s1_pos = _node_new(filter);
  _node_insert(filter, start_state_pos, NULL, s1_pos);

  size_t s2_pos = _gen_op(filter, root->childs[0], s1_pos);

  size_t s3_pos = _node_new(filter);
  _node_insert(filter, start_state_pos, NULL, s3_pos);

  size_t s4_pos = _gen_op(filter, root->childs[1], s3_pos);

  size_t end_pos = _node_new(filter);
  _node_insert(filter, s2_pos, NULL, end_pos);
  _node_insert(filter, s4_pos, NULL, end_pos);

  return end_pos;
}

/*
  nfa state map:
  rule a*
    
    |------------------ε-------------------|
    |                                      ∨
  <start> --ε--> <s1> --a--> <s2> --ε--> <end> 
                  ^            |
                  |-----ε------|
*/
static size_t _gen_op_rp(struct reg_filter* filter, struct reg_ast_node* root, size_t start_state_pos){
  size_t s1_pos = _node_new(filter);
  _node_insert(filter, start_state_pos, NULL, s1_pos);

  size_t s2_pos = _gen_op(filter, root->childs[0], s1_pos);
  _node_insert(filter, s2_pos, NULL, s1_pos);

  size_t end_pos = _node_new(filter);
  _node_insert(filter, start_state_pos, NULL, end_pos);
  _node_insert(filter, s2_pos, NULL, end_pos);
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

// ------------------------------ nfa subset construction to dfa  ------------------------------

static int _campar(size_t* a, size_t*b){
  return *a - *b;
}

static void _sort_subset(struct reg_list* subset){
  list_sort(subset, (campar)_campar);
}


/*
  get edsilone - closure (subset)
  subset is return value.
  the tunction return is boolean. have end state in subset.
*/
static int _closure(struct reg_filter* filter, struct reg_list* subset){
  int have_end_state = __move(filter, subset, 0, subset);
  // printf("--- closure!!!\n");
  return have_end_state;
}

/*
  get the subset by edsilone or edge.
  the out_subset is return value.
*/
static int __move(struct reg_filter* filter, struct reg_list* subset, size_t edge_pos, struct reg_list* out_subset){
  (filter->closure_tag)++; 
  int have_end_state = 0;

  size_t* pos = NULL;
  for(size_t i=0; (pos = list_idx(subset, i)); i++){
    assert(*pos);
    struct reg_node* node = _node_pos(filter, *pos);
    node->subset_tag = filter->closure_tag;

    if(node->is_end)
      have_end_state = 1;

    foreach_edge(v, node, filter){
      size_t next_node_pos = v->next_node_pos;
      assert(v->next_node_pos);
      // is edsilone and not in the out_subset
      if(v->edge_pos == edge_pos && 
        _node_pos(filter, next_node_pos)->subset_tag != filter->closure_tag){ 
        list_add(out_subset, &(v->next_node_pos));
      }
    }
  }

  _sort_subset(out_subset);
  #ifdef _DEBUG_
    _dump_subset(filter);
  #endif
  return have_end_state;
}

/*
  move state_pos edge_pos, 
  the return value is filter->eval_subset
*/
static int _move(struct reg_filter* filter, size_t state_pos, size_t edge_pos){
  struct reg_list* subset = _node_pos(filter, state_pos)->subset;

  assert(state_pos);
  assert(edge_pos);
  assert(list_len(subset));

  list_clear(filter->eval_subset);

  __move(filter, subset, edge_pos, filter->eval_subset);
  int success = list_len(filter->eval_subset)>0;
  // printf("---move state pos: %zd edge: %zd success: %d\n", state_pos, edge_pos, success);
  return success;
}

static size_t _nfa_node_new(struct reg_filter* filter){
  size_t pos = _node_new(filter);
  struct reg_node* node = _node_pos(filter, pos);
  assert(node->subset == NULL);
  node->subset = list_copy(filter->eval_subset);
  return pos;
}

static void _nfa_node_insert(struct reg_filter* filter, size_t node_pos, size_t edge_pos, size_t next_node_pos){
  struct _reg_path path = {
    .edge_pos = edge_pos,
    .next_node_pos = next_node_pos,
  };
  __node_insert(filter, node_pos, &path);
}

static int _subset2node(struct reg_filter* filter, size_t begin_pos){
  struct reg_node* v = NULL;
  size_t eval_subset_len = list_len(filter->eval_subset);
  assert(eval_subset_len);
  assert(begin_pos);

  for(size_t pos = begin_pos; (v = _node_pos(filter, pos)); pos++){
    assert(v->subset);
    if(eval_subset_len == list_len(v->subset)){
      size_t i;
      for(i=0; i<eval_subset_len; i++){
        size_t* pos1 = list_idx(filter->eval_subset, i);
        size_t* pos2 = list_idx(v->subset, i);
        if(*pos1 != *pos2) break;
      }
      if(i == eval_subset_len) return pos; // the subset is exist
    }
  }
  return _nfa_node_new(filter); // return the new node
}

static size_t _gen_dfa(struct reg_filter* filter, size_t start_state_pos){
  size_t dfa_begin_idx = list_len(filter->state_list);
  size_t edges_count = list_len(filter->edges_list);
  int have_end_state = 0;

  // closure(start)
  list_clear(filter->eval_subset);
  list_add(filter->eval_subset, &start_state_pos);
  have_end_state = _closure(filter, filter->eval_subset);
  assert(list_len(filter->eval_subset));

  size_t pos = _nfa_node_new(filter);
  if(have_end_state){ 
    _node_pos(filter, pos)->is_end = 1;
  }

  size_t begin_pos = dfa_begin_idx + 1;
  size_t end_pos = begin_pos + 1;

  // work loop
  for(;begin_pos<end_pos;){
    for(size_t state_pos=begin_pos; state_pos<end_pos; state_pos++){
      // foreach edges
      for(size_t edge_pos=1; edge_pos<=edges_count; edge_pos++){
        // counstruction subset
        int success = _move(filter, state_pos, edge_pos);
        // the subset is not nil
        if(success){
          have_end_state = _closure(filter, filter->eval_subset);

          // insert subset
          size_t next_node_pos = _subset2node(filter, begin_pos);
          _nfa_node_insert(filter, state_pos, edge_pos, next_node_pos);

          if(have_end_state){
            _node_pos(filter, next_node_pos)->is_end = 1;
          }
        }
      }
    }

    // next
    begin_pos = end_pos;
    end_pos = list_len(filter->state_list)+1;
  }

  return dfa_begin_idx+1;
}

// for test
static void _dump_subset(struct reg_filter* filter){
  size_t* pos = NULL;
  printf("------- dump subset len=%zd -------\n", list_len(filter->eval_subset));
  for(size_t i=0; (pos = list_idx(filter->eval_subset, i)); i++){
    printf("%zd ", *pos);
  }
  printf("\n");
}



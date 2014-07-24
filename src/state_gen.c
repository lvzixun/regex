/*
  construction nfa and convert nfa to dfa
*/

#include <stdio.h>
#include <string.h>

#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_list.h"
#include "state_pattern.h"
#include "reg_stream.h"

#include "regex.h"

struct min_node{
  size_t state_pos;
  int subset;
};


#define foreach_edge(v, node) struct _reg_path* v = NULL; \
                                      for(size_t i=0; (v = list_idx(node->edges, i)); i++)

static size_t _gen_nfa(struct reg_pattern* pattern, struct reg_ast_node* root);
static size_t _gen_dfa(struct reg_pattern* pattern, size_t start_state_pos);
static size_t _gen_min_dfa(struct reg_pattern* pattern);

static inline size_t _node_pass(struct reg_pattern* pattern, size_t node_pos, size_t edge_pos);
static inline void _node_tag(struct reg_pattern* pattern, size_t node_pos, int tag);
static inline int _node_insert(struct reg_pattern* pattern, size_t node_pos, struct reg_range* range, size_t dest_pos);
static inline size_t _node_new(struct reg_pattern* pattern);
static int __move(struct reg_pattern* pattern, struct reg_list* subset, size_t edge_pos, struct reg_list* out_subset);

/*dump op*/
static size_t _gen_op(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_and(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_or(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);

static size_t _gen_op_rps(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_rpp(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);
static size_t _gen_op_rpq(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);

static size_t _gen_op_range(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos);


#ifdef _DEBUG_
static void _dump_subset(struct reg_pattern* pattern);
static void _dump_minsubset(struct reg_list* minsubset);
#endif

void state_gen(struct reg_pattern* pattern, struct reg_ast_node* ast){
  list_clear(pattern->state_list);
  pattern->start_state_pos = _gen_nfa(pattern, ast);
  pattern->dfa_start_state_pos = _gen_dfa(pattern, pattern->start_state_pos);
  pattern->min_dfa_start_state_pos = _gen_min_dfa(pattern);
}

static size_t _gen_nfa(struct reg_pattern* pattern, struct reg_ast_node* root){
  size_t head = _node_new(pattern);
  size_t end = _gen_op(pattern, root, head);
  struct reg_node* node = state_node_pos(pattern, end);
  node->is_end = 1;
  return head;
}

//------------------------------ check char at edge ------------------------------
inline struct reg_edge* state_edge_pos(struct reg_pattern* pattern, size_t pos){
  if(pos == 0) return NULL; // is edsilone edege  
  struct reg_edge* edge = list_idx(pattern->edges_list, pos-1);
  return edge;
}

// ------------------------------ generate nfa node ------------------------------
static inline size_t _node_new(struct reg_pattern* pattern){
  size_t len = list_len(pattern->state_list);
  struct reg_node v = {
    .node_pos = len+1,
    .merge_pos = 0,
    .is_end = 0,
    .subset_tag = 0,
    .subset = NULL,
    .edges = list_new(sizeof(struct _reg_path), DEF_EDGE),
  };
  size_t idx = list_add(pattern->state_list, &v);
  return idx + 1;
}

static inline void _node_tag(struct reg_pattern* pattern, size_t node_pos, int tag){
  state_node_pos(pattern, node_pos)->subset_tag = tag;
}

static inline size_t _node_pass(struct reg_pattern* pattern, size_t node_pos, size_t edge_pos){
  struct reg_node* node = state_node_pos(pattern, node_pos);
  foreach_edge(v, node){
    if(v->edge_pos == edge_pos)
      return v->next_node_pos;
  }
  return 0;
}

inline struct reg_node* state_node_pos(struct reg_pattern* pattern, size_t pos){
  struct reg_node* ret = list_idx(pattern->state_list, pos -1);
  return ret;
}

static inline void __node_insert(struct reg_pattern* pattern, size_t node_pos, struct _reg_path* path){
  assert(path);
  struct reg_node* node = state_node_pos(pattern, node_pos);
  list_add(node->edges, path);
}

static inline int _node_insert(struct reg_pattern* pattern, size_t node_pos, struct reg_range* range, size_t dest_pos){
  int count = 0;
  struct _reg_path path = {0};

  // insert epsilon edge
  if(range ==NULL){
    path.edge_pos = 0;
    path.next_node_pos = dest_pos;
    __node_insert(pattern, node_pos, &path);
    return 0;
  }

  struct reg_edge* edge = NULL;
  for(size_t i = 0; (edge = list_idx(pattern->edges_list, i)); i++){
    if(count>0 && range->begin > edge->range.end) break;

    if(edge->range.begin >= range->begin && edge->range.end <= range->end){
      path.edge_pos = i + 1;
      path.next_node_pos = dest_pos;
      __node_insert(pattern, node_pos, &path);
      count++;
    }
  }

  return count;
}


static size_t _gen_op(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){
  switch(root->op){
    case op_and:
      return _gen_op_and(pattern, root, start_state_pos);
    case op_or:
      return _gen_op_or(pattern, root, start_state_pos);
    case op_rps:
      return _gen_op_rps(pattern, root, start_state_pos);
    case op_rpp:
      return _gen_op_rpp(pattern, root, start_state_pos);
    case op_rpq:
      return _gen_op_rpq(pattern, root, start_state_pos);
    case op_range:
      return _gen_op_range(pattern, root, start_state_pos);
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
static size_t _gen_op_and(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){
  assert(root->op == op_and);

  size_t s1_pos = _gen_op(pattern, root->childs[0], start_state_pos);
  
  size_t s2_pos = _node_new(pattern);
  _node_insert(pattern, s1_pos, NULL, s2_pos);

  size_t end = _gen_op(pattern, root->childs[1], s2_pos);
  return end;
}


/*
  nfa state map:
  rule: a|b
     |----ε----> <s1> --a--> <s2> ---ε-------|
     |                                       ∨
  <start> --ε--> <s3> --b--> <s4> ---ε---> <end>
*/

static size_t _gen_op_or(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){ 
  assert(root->op == op_or);

  size_t s1_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, s1_pos);

  size_t s2_pos = _gen_op(pattern, root->childs[0], s1_pos);

  size_t s3_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, s3_pos);

  size_t s4_pos = _gen_op(pattern, root->childs[1], s3_pos);

  size_t end_pos = _node_new(pattern);
  _node_insert(pattern, s2_pos, NULL, end_pos);
  _node_insert(pattern, s4_pos, NULL, end_pos);

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
static size_t _gen_op_rps(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){
  size_t s1_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, s1_pos);

  size_t s2_pos = _gen_op(pattern, root->childs[0], s1_pos);
  _node_insert(pattern, s2_pos, NULL, s1_pos);

  size_t end_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, end_pos);
  _node_insert(pattern, s2_pos, NULL, end_pos);
  return end_pos;
}


/*
  nfa state map:
  rule a+
    
  <start> --ε--> <s1> --a--> <s2> --ε--> <end> 
                  ^            |
                  |-----ε------|
*/
static size_t _gen_op_rpp(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){
  size_t s1_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, s1_pos);

  size_t s2_pos = _gen_op(pattern, root->childs[0], s1_pos);
  _node_insert(pattern, s2_pos, NULL, s1_pos);

  size_t end_pos = _node_new(pattern);
  _node_insert(pattern, s2_pos, NULL, end_pos);
  return end_pos;
}

/*
  nfa state map:
  rule a?
    
    |------------------ε-------------------|
    |                                      ∨
  <start> --ε--> <s1> --a--> <s2> --ε--> <end> 
*/
static size_t _gen_op_rpq(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){
  size_t s1_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, s1_pos);

  size_t s2_pos = _gen_op(pattern, root->childs[0], s1_pos);

  size_t end_pos = _node_new(pattern);
  _node_insert(pattern, start_state_pos, NULL, end_pos);
  _node_insert(pattern, s2_pos, NULL, end_pos);
  return end_pos;
}



/*
  nfa state map:
  rule: [a-b]
  <start> --[a-b]--> <end>
*/
static size_t _gen_op_range(struct reg_pattern* pattern, struct reg_ast_node* root, size_t start_state_pos){
  assert(root->op == op_range);
  struct reg_range* range = &(root->value.range);

  size_t end_pos = _node_new(pattern);

  int is_insert  = 0;
  is_insert = _node_insert(pattern, start_state_pos, range, end_pos);

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
static int _closure(struct reg_pattern* pattern, struct reg_list* subset){
  int have_end_state = __move(pattern, subset, 0, subset);
  return have_end_state;
}

/*
  get the subset by edsilone or edge.
  the out_subset is return value.
*/
static int __move(struct reg_pattern* pattern, struct reg_list* subset, size_t edge_pos, struct reg_list* out_subset){
  (pattern->closure_tag)++; 
  int have_end_state = 0;

  size_t* pos = NULL;
  for(size_t i=0; (pos = list_idx(subset, i)); i++){
    assert(*pos);
    struct reg_node* node = state_node_pos(pattern, *pos);
    node->subset_tag = pattern->closure_tag;

    if(node->is_end)
      have_end_state = 1;

    foreach_edge(v, node){
      size_t next_node_pos = v->next_node_pos;
      struct reg_node* next_node = state_node_pos(pattern, next_node_pos);
      assert(v->next_node_pos);
      // is edsilone and not in the out_subset
      if(v->edge_pos == edge_pos && 
        next_node->subset_tag != pattern->closure_tag){ 
        next_node->subset_tag = pattern->closure_tag;
        list_add(out_subset, &next_node_pos);
      }
    }
  }

  _sort_subset(out_subset);
  #ifdef _DEBUG_
    _dump_subset(pattern);
  #endif
  return have_end_state;
}

/*
  move state_pos edge_pos, 
  the return value is pattern->eval_subset
*/
static int _move(struct reg_pattern* pattern, size_t state_pos, size_t edge_pos){
  struct reg_list* subset = state_node_pos(pattern, state_pos)->subset;

  assert(state_pos);
  assert(edge_pos);
  assert(list_len(subset));

  list_clear(pattern->eval_subset);

  __move(pattern, subset, edge_pos, pattern->eval_subset);
  int success = list_len(pattern->eval_subset)>0;
  #ifdef _DEBUG_
    printf("move state pos: %zd edge: %zd success: %d\n", state_pos, edge_pos, success);
  #endif
  return success;
}

static size_t _nfa_node_new(struct reg_pattern* pattern){
  size_t pos = _node_new(pattern);
  struct reg_node* node = state_node_pos(pattern, pos);
  assert(node->subset == NULL);
  node->subset = list_copy(pattern->eval_subset);
  return pos;
}

/*
  connect node_pos and next_node_pos
  node_pos --- edge_pos --> next_node_pos
*/
static void _dfa_node_insert(struct reg_pattern* pattern, size_t node_pos, size_t edge_pos, size_t next_node_pos){
  struct _reg_path path = {
    .edge_pos = edge_pos,
    .next_node_pos = next_node_pos,
  };

  struct reg_node* node = state_node_pos(pattern, node_pos);
  foreach_edge(v, node){
    if(v->edge_pos == path.edge_pos){
      assert(v->next_node_pos == path.next_node_pos);
      return;
    }
  }

  __node_insert(pattern, node_pos, &path);
}

static int _subset2node(struct reg_pattern* pattern, size_t begin_pos){
  struct reg_node* v = NULL;
  size_t eval_subset_len = list_len(pattern->eval_subset);
  assert(eval_subset_len);
  assert(begin_pos);

  for(size_t pos = begin_pos; (v = state_node_pos(pattern, pos)); pos++){
    assert(v->subset);
    if(eval_subset_len == list_len(v->subset)){
      size_t i;
      for(i=0; i<eval_subset_len; i++){
        size_t* pos1 = list_idx(pattern->eval_subset, i);
        size_t* pos2 = list_idx(v->subset, i);
        if(*pos1 != *pos2) break;
      }
      if(i == eval_subset_len) return pos; // the subset is exist
    }
  }
  return _nfa_node_new(pattern); // return the new node
}


/*
  counstruction to dfa
  start_state_pos is nfa  start state postion
*/
static size_t _gen_dfa(struct reg_pattern* pattern, size_t start_state_pos){
  size_t dfa_begin_idx = list_len(pattern->state_list);
  size_t edges_count = list_len(pattern->edges_list);
  int have_end_state = 0;

  // closure(start)
  list_clear(pattern->eval_subset);
  list_add(pattern->eval_subset, &start_state_pos);
  have_end_state = _closure(pattern, pattern->eval_subset);
  assert(list_len(pattern->eval_subset));

  // set end state
  size_t pos = _nfa_node_new(pattern);
  if(have_end_state){ 
    state_node_pos(pattern, pos)->is_end = 1;
  }

  size_t begin_pos = dfa_begin_idx + 1;
  size_t end_pos = begin_pos + 1;

  // work loop
  for(;begin_pos<end_pos;){
    for(size_t state_pos=begin_pos; state_pos<end_pos; state_pos++){
      // foreach edges
      for(size_t edge_pos=1; edge_pos<=edges_count; edge_pos++){
        // counstruction subset
        int success = _move(pattern, state_pos, edge_pos);
        // the subset is not nil
        if(success){
          have_end_state = _closure(pattern, pattern->eval_subset);

          // insert subset
          size_t next_node_pos = _subset2node(pattern, dfa_begin_idx + 1);
          _dfa_node_insert(pattern, state_pos, edge_pos, next_node_pos);

          // set edn state
          if(have_end_state){
            state_node_pos(pattern, next_node_pos)->is_end = 1;
          }
        }
      }
    }

    // next
    begin_pos = end_pos;
    end_pos = list_len(pattern->state_list)+1;
  }

  return dfa_begin_idx+1;
}



//------------------------------ minimization dfa ------------------------------
static int _min_sort(struct min_node* a, struct min_node* b){
  return a->subset - b->subset;
}

static void _sort_minsubset(struct reg_list* minsubset, size_t begin_idx, size_t len){
  list_sort_subset(minsubset, begin_idx, len, (campar)_min_sort);
}


static struct reg_list* _new_minsubset(struct reg_pattern* pattern){
  size_t len = list_len(pattern->state_list) - pattern->dfa_start_state_pos + 1;
  struct reg_list* minsubset = list_new(sizeof(struct min_node), len);

  // foreach dfa node
  struct reg_node* v = NULL;
  for(size_t pos = pattern->dfa_start_state_pos; (v = state_node_pos(pattern, pos)); pos++){
    struct min_node node = {
      .state_pos = v->node_pos, 
      .subset = 0,
    };
    if(v->is_end){ // is not accept subset
      node.subset = 2;
    }else{ // is accept subset
      node.subset = 1; 
    }

    _node_tag(pattern, node.state_pos, node.subset);
    list_add(minsubset, &node);
  }

  _sort_minsubset(minsubset, 0, list_len(minsubset));
  return minsubset;
}


static void _free_minsubset(struct reg_pattern* pattern, struct reg_list* minsubset){
  list_free(minsubset);
}


static inline int _split(struct reg_pattern* pattern, struct reg_list* minsubset, size_t begin_idx, size_t len){
  struct min_node* v = NULL;
  size_t edge_len = list_len(pattern->edges_list);
  int is_split = 0;
  int split_edge_pos = 0;

  #ifdef _DEBUG_
    printf("_split: begin_idx: %zd len: %zd\n", begin_idx, len);
  #endif

  // foreach all edge
  for(size_t edge_pos = 1; edge_pos<= edge_len ;edge_pos++){
    int cur_subset = 0;
    int split_count = 0;

    // foreach subset
    for(size_t i = begin_idx; i<begin_idx+len; i++){
      v = list_idx(minsubset, i);
      size_t node_pos = v->state_pos;
      struct reg_node* cur_node = state_node_pos(pattern, node_pos);
      assert(cur_node->subset_tag == v->subset);

      size_t next_node_pos = _node_pass(pattern, node_pos, edge_pos);
      struct reg_node* next_node = state_node_pos(pattern, next_node_pos);

      if(cur_subset==0){
        cur_subset = v->subset;
      }

      if((!next_node) || (next_node && len >1 && cur_subset != next_node->subset_tag)) {
        split_count++;
        v->subset = pattern->minsubset_max;
        cur_node->subset_tag = v->subset;
      }
    }

    if(split_count > 0 && split_count <len){
      is_split = 1;
      split_edge_pos = edge_pos;
      break;
    }
    else if(split_count == len)
      pattern->minsubset_max++;
  }


  #ifdef _DEBUG_
    printf("split  edge:%d: success: %d  begin: %zd old_len:%zd\n", split_edge_pos, is_split, begin_idx, len);
    _dump_minsubset(minsubset);
    printf("\n");
  #endif

  if(is_split){
    pattern->minsubset_max++;
    _sort_minsubset(minsubset, begin_idx, len);
  }


  return is_split;
}


static void _min_dfa(struct reg_pattern* pattern, struct reg_list* minsubset){
  int split_count = 0;
  size_t min_len = list_len(minsubset);
  assert(min_len);

  do{
    split_count = 0;
    struct min_node* v = NULL;
    size_t cur_subset = ((struct min_node*)list_idx(minsubset, 0))->subset;
    size_t subset_len = 0;

    for(size_t i=0; (v = list_idx(minsubset, i)); i++){
      if(v->subset != cur_subset){
        split_count += _split(pattern, minsubset, i - subset_len, subset_len);
        cur_subset = v->subset;
        subset_len = 1;
      }else{
        subset_len++;
      }
    }

    split_count += _split(pattern, minsubset, min_len - subset_len, subset_len);
  }while(split_count);
}

/*
  mark the dfa state
*/
static void _mark(struct reg_pattern* pattern, struct reg_list* minsubset){
  struct min_node* v = NULL;
  size_t i=0;
  size_t j=0;

  for(i=0; (v = list_idx(minsubset, i)); i = j){
    struct min_node* nv = NULL;
    size_t merge_pos = 0;
    for(j=i+1; (nv = list_idx(minsubset, j))&&(v->subset == nv->subset); j++){
      if(merge_pos == 0)
        merge_pos = _node_new(pattern);

      state_node_pos(pattern, v->state_pos)->merge_pos = merge_pos;

      struct reg_node* node = state_node_pos(pattern, nv->state_pos);
      node->merge_pos = merge_pos;

      if(node->is_end)
        state_node_pos(pattern, merge_pos)->is_end = node->is_end;
    }
  }
}

/*
  merge minsubset at dfa
*/
static void _merge(struct reg_pattern* pattern, struct reg_list* minsubset){
  struct reg_node* node = NULL;
  for(size_t pos=pattern->dfa_start_state_pos; (node= state_node_pos(pattern, pos)); pos++){
    size_t merge_pos = node->merge_pos;
    foreach_edge(v, node){
      size_t edge_pos = v->edge_pos;
      size_t next_node_pos = v->next_node_pos;
      struct reg_node* next_node = state_node_pos(pattern, next_node_pos);
      next_node_pos = (next_node->merge_pos)?(next_node->merge_pos):(next_node_pos);
      if(merge_pos)
        _dfa_node_insert(pattern, merge_pos, edge_pos, next_node_pos); 
      else
        v->next_node_pos = next_node_pos;
    }
  }
}


static size_t _gen_min_dfa(struct reg_pattern* pattern){
  struct reg_list* minsubset = _new_minsubset(pattern);
  pattern->minsubset_max = 3;

  _min_dfa(pattern, minsubset);
  _mark(pattern, minsubset);
  _merge(pattern, minsubset);

  struct reg_node* node = state_node_pos(pattern, pattern->dfa_start_state_pos);
  assert(node->node_pos == pattern->dfa_start_state_pos);
  size_t start_pos = (node->merge_pos)?(node->merge_pos):(node->node_pos);

  _free_minsubset(pattern, minsubset);
  return start_pos;
}





#ifdef _DEBUG_
// for test
static void _dump_minsubset(struct reg_list* minsubset){
  printf("------- minsubset --------\n");
  struct min_node* v = NULL;
  for(size_t i =0; (v = list_idx(minsubset, i)); i++){
    printf("state:%zd[%d]   ", v->state_pos, v->subset);
  }
  printf("\n");
}

static void _dump_subset(struct reg_pattern* pattern){
  size_t* pos = NULL;
  printf("------- dump subset len=%zd -------\n", list_len(pattern->eval_subset));
  for(size_t i=0; (pos = list_idx(pattern->eval_subset, i)); i++){
    size_t* nv = list_idx(pattern->eval_subset, i+1);
    if(nv){
      assert(*nv != *pos);
    }
    printf("%zd ", *pos);
  }
  printf("\n");
}

#endif

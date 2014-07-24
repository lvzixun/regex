#include <string.h>
#include <stdio.h>

#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_list.h"
#include "state_pattern.h"


#define frame_idx(p, idx)       ((struct _range_frame*)list_idx(p->frame_list, idx))

struct _range_frame {
  enum {
    e_begin,
    e_end
  }type;

  int value[2];  // save begin value and end value
};

struct reg_state{
  struct reg_env* env;
  struct reg_list* frame_list; // the struct _reg_frame object list
};

static void _gen_frame(struct reg_state* p, struct reg_ast_node* root);
static void _gen_edge(struct reg_pattern* pattern);

struct reg_state* state_new(struct reg_env* env){
  struct reg_state* ret = malloc(sizeof(struct reg_state));

  ret->env = env;

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
  list_clear(p->frame_list);
}

static inline struct reg_pattern* _new_pattern(struct reg_state* state){
  struct reg_pattern* ret = malloc(sizeof(struct reg_pattern));
  ret->state = state;
  ret->edges_list = list_new(sizeof(struct reg_edge), DEF_EDGES);

  ret->state_list = list_new(sizeof(struct reg_node), DEF_NODES);

  ret->eval_subset = list_new(sizeof(size_t), DEF_SUBSET_COUNT);

  ret->start_state_pos = 0;
  ret->dfa_start_state_pos = 0;
  ret->min_dfa_start_state_pos = 0;
  ret->minsubset_max = 0;

  ret->closure_tag = 0;
  return ret;
}


static inline int _campar(const struct _range_frame* a, const struct _range_frame* b){
  return a->value[0] - b->value[0];
}

struct reg_pattern* state_new_pattern(struct reg_state* p, struct reg_ast_node* ast, int is_match_tail){
  struct reg_pattern* pattern = _new_pattern(p);

  // set is match the end of string
  pattern->is_match_tail = is_match_tail;

  // prepare frame list
  list_clear(p->frame_list);
  _gen_frame(p, ast);
  list_sort(p->frame_list, (campar)_campar);

  // prepare edge list
  list_clear(pattern->edges_list);
  _gen_edge(pattern);

  // prepare state map
  state_gen(pattern, ast);
  return pattern;
}

void state_free_pattern(struct reg_pattern* pattern){
  struct reg_node* node = NULL;
  // free state
  for(int i=0; (node = list_idx(pattern->state_list, i)) != NULL; i++){
    list_free(node->edges);
    if(node->subset)
      list_free(node->subset);
  }
  list_free(pattern->state_list);

  // free subset
  list_free(pattern->eval_subset);

  // free edges
  list_free(pattern->edges_list);
  free(pattern);
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

static inline void _insert_edge(struct reg_pattern* pattern, int begin, int end){
  struct reg_edge value = {
    .range.begin = begin,
    .range.end = end,
  };

  list_add(pattern->edges_list, &value);
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

static inline int _read_frame(struct reg_pattern* pattern, size_t idx, 
  int* out_range_right, int* out_next_begin){
  
  int count = 0;
  assert(out_range_right);
  
  struct _range_frame* f = frame_idx(pattern->state, idx);
  if(!f) return 0;

  int value = f->value[0];
  int is_insert = 0;
  int is_begin = 0, is_end = 0;

  *out_next_begin = value;

  struct _range_frame* v = NULL;
  for(;  
      v = frame_idx(pattern->state, idx), v && value == v->value[0]; 
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
      _insert_edge(pattern, v->value[0], v->value[0]);
      is_insert = 1;
    }
  }

  return count;
}

static void _gen_edge(struct reg_pattern* pattern){
  int range_right = 0;
  int head = 0;
  size_t i = _read_frame(pattern, 0, &range_right, &head);

  struct _range_frame* frame = NULL;
  for(; (frame = list_idx(pattern->state->frame_list, i)); ){
    int tail = frame->value[0];
    if(_need_insert(pattern->state, i) || frame->type == e_begin){
      tail -= 1;
    }
    if(head <= tail && tail <= range_right){
      _insert_edge(pattern, head, tail);
    }

    int count = _read_frame(pattern, i, &range_right, &head);
    if(head == tail) head++;
    i += count;
  }
}

// ------------------------------  for test  ------------------------------

static inline void _dump_edge(struct reg_list* edges_list){
  struct reg_edge* v = NULL;
  printf("------ dump_pattern_edge --------\n");
  for(size_t i=0; (v = list_idx(edges_list, i)); i++){
    printf("[%c - %c] ", v->range.begin, v->range.end);
  }
  printf("\n");  
}

static inline void _dump_node(struct reg_pattern* pattern, struct reg_node* node){
  printf("state[%zd] merge:%zd end: %d  edges: %zd", 
    node->node_pos, node->merge_pos, 
    node->is_end, list_len(node->edges));

  struct _reg_path* path = NULL;
  for(size_t i=0; (path = list_idx(node->edges, i)); i++){
    if(path->edge_pos > 0){
      struct reg_edge* edge = list_idx(pattern->edges_list, path->edge_pos-1);
      printf("[%c-%c]->next_state<%zd>  ", edge->range.begin, edge->range.end, path->next_node_pos);
    }else{
      printf("[ε]->next_state<%zd>  ", path->next_node_pos);
    }
  }
  printf("\n");
}

static void __dump_state(struct reg_pattern* pattern){
  printf("\n--------------dump state -----------------\n");
  printf("nfa start pos: %zd  nfa end pos: %zd\ndfa start pos: %zd dfa end pos:%zd\nmin dfa start pos: %zd\n", 
    pattern->start_state_pos, pattern->dfa_start_state_pos-1,
    pattern->dfa_start_state_pos, list_len(pattern->state_list),
    pattern->min_dfa_start_state_pos);
  struct reg_node* node = NULL;
  for(size_t i=0; (node = list_idx(pattern->state_list, i)); i++){
    _dump_node(pattern, node);
  }
}

void dump_pattern(struct reg_pattern* p){
  _dump_edge(p->edges_list);
  __dump_state(p);
}

void dump_frame(struct reg_state* p){
 struct _range_frame* v = NULL;
 printf("-------- dump_frame ----------\n");
 for(size_t i=0; (v = list_idx(p->frame_list, i)); i++){
  printf("value: %c type: %d  ", v->value[0], v->type);
 }
 printf("\n");
}




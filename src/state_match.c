/*
  match regex rule from minimization dfa
*/

#include <stdio.h>
#include "state_pattern.h"
#include "reg_stream.h"
#include "reg_list.h"

#include "regex.h"

static int _match_dfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source);
static int _match_nfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source);

int state_match(struct reg_pattern* pattern, const char* s, int len){
  assert(_match_nfa_state); // filter warning

  struct reg_stream* source = stream_new((const unsigned char*)s, len);
  int success = _match_dfa_state(pattern, pattern->min_dfa_start_state_pos, source);
  stream_free(source);
  return success;
}


static int _match_dfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source){
  for(;!stream_end(source);){
    // dump edge
    struct reg_list* edges = state_node_pos(pattern, node_pos)->edges;
    struct _reg_path* path = NULL;
    unsigned char c = stream_char(source);    

    for(size_t i=0; (path = list_idx(edges, i)); i++){
      struct reg_range* range = &(state_edge_pos(pattern, path->edge_pos)->range);
      size_t next_node_pos = path->next_node_pos;

      assert(range);
      if(c >= range->begin && c<=range->end){ // range
        node_pos = next_node_pos;
        stream_next(source);
        break;
      }
    }
  }

  // match is end of source
  return (stream_end(source) && state_node_pos(pattern, node_pos)->is_end);
}


static int _match_nfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source){
  // pass end state
  if(stream_end(source) && state_node_pos(pattern, node_pos)->is_end){ 
    return 1;
  }

  // dump edge
  struct reg_list* edges = state_node_pos(pattern, node_pos)->edges;
  struct _reg_path* path = NULL;
  unsigned char c = stream_char(source);


  int success = 0;
  for(size_t i=0; (path = list_idx(edges, i)); i++){
    struct reg_range* range = &(state_edge_pos(pattern, path->edge_pos)->range);
    size_t next_node_pos = path->next_node_pos;

    if(range == NULL){  //edsilone
      success = _match_nfa_state(pattern, next_node_pos, source);
    }else if(c >= range->begin && c<=range->end){ // range
      if(stream_next(source)){
        assert(success==0);
        success = _match_nfa_state(pattern, next_node_pos, source);
        stream_back(source);
      }
    }

    if(success) return 1; 
  }

  return 0;
}



#include <stdint.h>
#include <stdlib.h>
#include "cow_trie.c"

typedef struct node_or_edge_t node_or_edge_t, *node_or_edge_p;

struct node_or_edge_t {
    int tag;
    union {
        struct {
            uint32_t label;
            size_t pred_ct, succ_ct;
            int *preds, *succs;
        } node;
        struct {
            uint32_t label;
            int pred, succ;
        } edge;
    } _;
};

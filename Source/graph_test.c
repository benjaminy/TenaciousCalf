#include <stdint.h>
#include <stdlib.h>

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


node_or_edge_p node_or_edge_alloc(int tag, int num_preds, int num_succs) {
    node_or_edge_p n = (node_or_edge_p)malloc(sizeof(n[0]));
    if (tag == 0) {
        n->node = malloc(sizeof(n->node));
        int *p_array;
        int *s_array;
        p_array = (int *)malloc(sizeof(int)*num_preds);
        s_array = (int *)malloc(sizeof(int)*num_succs);
        n.node.preds = p_array;
        n.node.succs = s_array;      
    }
}

int main() {
    node_or_edge_p n = node_or_edge_alloc(0, 0 ,0);
}

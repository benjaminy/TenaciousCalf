#include <stdint.h>
#include <stdlib.h>

typedef struct node_t node_t, *node_p;
struct node_t {
    uint32_t val;
};
typedef struct edge_t edge_t, *edge_p;
struct edge_t {
    node_p pred;
    node_p ant;
};

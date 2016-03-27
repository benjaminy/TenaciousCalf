#ifndef C_T_2_H_
#define C_T_2_H_

typedef struct val_t val_t, *val_p;

typedef struct g_node_t g_node_t, *g_node_p;

typedef struct edge_list_t edge_list_t, *edge_list_p;

/* Using the variable-sized struct trick. 
 * Stores an array of int_type.
 */

//node fanout
typedef uint32_t int_type;


struct edge_list_t {
    uint32_t ref_ct;
    char _[0];
};

struct g_node_t {
    uint32_t succ_ct;
    edge_list_p succs;
#ifdef COW_INCLUDE_PREDS
    uint32_t pred_ct;
    edge_list_p preds;
#endif
};

struct g_edge_t {
    uint32_t pred, succ;
};

typedef struct g_edge_t g_edge_t, *g_edge_p;

struct val_t {
    int tag;
    union {
        g_node_t node;
        g_edge_t edge;
    } _;
};

typedef int_type vkey_t;

/* Using the variable-sized struct trick.  There are actually two
 * arrays:
 * - cow_trie_next_p children[ child_n ]
 * - val_vkey_t      values[ value_n ]
 */
typedef struct cow_trie_t cow_trie_t, *cow_trie_p;
    struct cow_trie_t
{
    int             ref_count;
    int_type        child_bitmap, value_bitmap;
    //size of this node
    int_type        size;
    //size of this node and everything underneath
    size_t          tot_size;
    char            _[0];
};

void cow_close_edge_list(edge_list_p e);

void cow_trie_close_node(cow_trie_p map);

int cow_trie_lookup(cow_trie_p map, vkey_t key, val_t *v);

int_type cow_trie_insert(cow_trie_p map, val_t val, cow_trie_p *res);

int cow_trie_delete(cow_trie_p map, vkey_t key, cow_trie_p *res);

cow_trie_p *cow_trie_children( cow_trie_p map);

val_t cow_trie_create_val(int tag,
                          #if COW_INCLUDE_PREDS
                          size_t pred_ct, 
                          #endif
                          size_t succ_ct,
                          #if COW_INCLUDE_PREDS
                          uint32_t *preds,
                          #endif
                          uint32_t *succs,
                          uint32_t pred,
                          uint32_t succ);

cow_trie_p cow_trie_alloc(int children, int values);

int count_one_bits( uint32_t bits );

edge_list_p edge_list_alloc(int ct);

uint32_t *edge_list_values(edge_list_p e);

#endif


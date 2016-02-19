#ifndef COW_TRIE_H_
#define COW_TRIE_H_

typedef struct node_or_edge_t node_or_edge_t, *node_or_edge_p;

typedef struct g_node_t g_node_t, *g_node_p;

struct g_node_t {
    uint32_t label;
    size_t pred_ct, succ_ct;
    uint32_t *preds, *succs;
};

struct g_edge_t {
    uint32_t label;
    uint32_t pred, succ;
};

typedef struct g_edge_t g_edge_t, *g_edge_p;

struct node_or_edge_t {
    int tag;
    union {
        g_node_t node;
        g_edge_t edge;
    } _;
};

typedef node_or_edge_t val_t;
typedef uint32_t vkey_t;

typedef struct val_key_t val_key_t, *val_key_p;

struct val_key_t
{
    uint32_t key_frag;
    val_t val;
};

/* Using the variable-sized struct trick.  There are actually two
 * arrays:
 * - cow_trie_next_p children[ child_n ]
 * - val_key_t      values[ value_n ]
 */
typedef struct cow_trie_t cow_trie_t, *cow_trie_p;
    struct cow_trie_t
{
    int             ref_count;
    uint32_t        child_bitmap, value_bitmap;
    uint32_t        size;
    char            _[0];
};


int cow_trie_lookup(cow_trie_p map, vkey_t key, val_t *v);

int cow_trie_insert(cow_trie_p map, vkey_t key, val_t val, cow_trie_p *res);

int cow_trie_delete(cow_trie_p map, vkey_t key, cow_trie_p *res);

val_t cow_trie_create_val(int tag,
                                 uint32_t label, 
                                 size_t pred_ct, 
                                 size_t succ_ct, 
                                 uint32_t *preds,
                                 uint32_t *succs,
                                 uint32_t pred,
                                 uint32_t succ);

cow_trie_p cow_trie_alloc(int children, int values);

#endif

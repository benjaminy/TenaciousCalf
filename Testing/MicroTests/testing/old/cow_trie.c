#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cow_trie.h"

int count_one_bits( uint32_t bits )
{
#if __has_builtin(__builtin_popcount)
    return __builtin_popcount( bits );
#else
    static const uint32_t SK5 =0x55555555, SK3 =0x33333333, SK0F=0x0F0F0F0F;
    bits -= ( ( bits >> 1 ) & SK5 );
    bits = ( bits & SK3 )  + ( ( bits >> 2 ) & SK3 );
    bits = ( bits & SK0F ) + ( ( bits >> 4 ) & SK0F );
    bits += bits >> 8;
    return ( bits + ( bits >> 16 ) ) & 0x3F;
#endif
}

int count_trailing_zeroes(uint32_t bits) {
#if __has_builtin(__builtin_ctz)
    return __builtin_ctz(bits);
#else
    static const int MultiplyDeBruijnBitPosition[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return MultiplyDeBruijnBitPosition[((uint32_t)((bits & -bits) * 0x077CB531U)) >> 27];
#endif
}

cow_trie_p *cow_trie_children( cow_trie_p m )
{
    return (cow_trie_p *)&m->_[0];
}

node_or_edge_p node_or_edge_alloc(int tag, int num_preds, int num_succs) {
    node_or_edge_p n = (node_or_edge_p)malloc(sizeof(n[0]));
    if (tag == 0) {
        n->_.node.preds = edge_list_alloc(num_preds);
        n->_.node.succs = edge_list_alloc(num_succs);
    }
    return n;
}

edge_list_p edge_list_alloc(int ct) {
    edge_list_p n = (edge_list_p)malloc(sizeof(n[0]) + ct * sizeof(uint32_t));
    return n;
}

uint32_t *edge_list_values(edge_list_p e) {
    return (uint32_t *)&e->_[0];
}


val_t cow_trie_create_val(int tag, 
                          size_t pred_ct, 
                          size_t succ_ct, 
                          uint32_t *preds,
                          uint32_t *succs,
                          uint32_t pred,
                          uint32_t succ) {
    node_or_edge_p thing = node_or_edge_alloc(tag, pred_ct, succ_ct);
    if (tag == 0) {
        thing->tag = 0;
        uint32_t *pred_array = edge_list_values(thing->_.node.preds);
        uint32_t *succ_array = edge_list_values(thing->_.node.succs);
        /*for (int i=0;i<pred_ct;i++) {
            pred_array[i] = preds[i];
        }
        for (int i=0;i<succ_ct;i++) {
            succ_array[i] = succs[i];
        }*/
        thing->_.node.succs->ref_ct = 1;
        thing->_.node.preds->ref_ct = 1;
        thing->_.node.pred_ct = pred_ct;
        thing->_.node.succ_ct = succ_ct;
    }
    else if (tag == 1) {
        thing->tag = 1;
        thing->_.edge.pred = pred;
        thing->_.edge.succ = succ;
    }
    return *thing;
}

static val_key_p cow_trie_values( cow_trie_p m )
{
    int child_count = count_one_bits( m->child_bitmap );
    return (val_key_p)&( ( cow_trie_children( m ) )[ child_count ] );
}

const int      CHILD_ARRAY_BUFFER_SIZE = 0;
const int      VALUE_ARRAY_BUFFER_SIZE = 0;
const uint32_t           LOW_BITS_MASK = 0x1f;
const int                 BITS_PER_LVL = 5;
const int                 LVL_CAPACITY = 1 << ( BITS_PER_LVL );

cow_trie_p cow_trie_alloc( int children, int values ) {
    uint32_t size = sizeof( cow_trie_t ) + (children + CHILD_ARRAY_BUFFER_SIZE) * sizeof( cow_trie_p )
                    + (values + VALUE_ARRAY_BUFFER_SIZE) * sizeof(val_key_t);
    cow_trie_p n = (cow_trie_p)malloc(size);
    n->size = size;
    return n;
}

cow_trie_p cow_trie_clone_node( cow_trie_p map, int children, int values )
{
    cow_trie_p m = (cow_trie_p)malloc(
               sizeof( m[0] ) + children * sizeof( cow_trie_p )
        + values * sizeof( val_key_t ) );
    if( m )
    {
        *m = *map;
    }
    return m;
}

cow_trie_p cow_trie_copy_node(cow_trie_p map) {
    int num_children = count_one_bits(map->child_bitmap);
    int num_values = count_one_bits(map->value_bitmap);
    cow_trie_p copy = cow_trie_alloc(num_children, num_values);
    copy->child_bitmap = map->child_bitmap;
    copy->value_bitmap = map->value_bitmap;
    copy->ref_count    = 1;
    cow_trie_p * orig_children = cow_trie_children(map);
    cow_trie_p * copy_children = cow_trie_children(copy);
    val_key_p orig_values = cow_trie_values(map);
    val_key_p copy_values = cow_trie_values(copy);
    for (int i=0;i<num_children;i++) {
        ++orig_children[i]->ref_count;
        copy_children[i] = orig_children[i];
    }
    for (int i=0;i<num_values;i++) {
        if(orig_values[i].val.tag == 0) {
            ++orig_values[i].val._.node.preds->ref_ct;
            ++orig_values[i].val._.node.succs->ref_ct;
        }
        copy_values[i] = orig_values[i];
    }
    return copy;
}

void cow_trie_close_node(cow_trie_p node) {
    --node->ref_count;
    if (node->ref_count == 0) {
        int num_values = count_one_bits(node->value_bitmap);
        val_key_p vals = cow_trie_values(node);
        for (int i=0;i<num_values;i++) {
            if (vals[i].val.tag == 0) {
                --vals[i].val._.node.preds->ref_ct;
                --vals[i].val._.node.succs->ref_ct;
                if (vals[i].val._.node.preds->ref_ct == 0) {
                    free(vals[i].val._.node.preds);
                }
                if (vals[i].val._.node.succs->ref_ct == 0) {
                    free(vals[i].val._.node.succs);
                }
            }
        }
        int num_kids = count_one_bits(node->child_bitmap);
        for (int i=0;i<num_kids;i++) {
            cow_trie_p *children = cow_trie_children(node);
            cow_trie_close_node(children[i]);
        }
        free(node);
    }
}
static val_key_t *cow_val_ref( cow_trie_p map, int ccount, int vidx )
{
    return (val_key_t *)&map->_[ ccount * sizeof( cow_trie_p )
                                 + vidx * sizeof( val_key_t ) ];
}

static val_t cow_deref_val( cow_trie_p map, int ccount, int vidx )
{
    return cow_val_ref( map, ccount, vidx )->val;
}


int cow_trie_lookup(
    cow_trie_p map, vkey_t key, val_t *v )
{
    int        virtual_idx = key & LOW_BITS_MASK;
    uint32_t   bitmask_loc = 1 << virtual_idx;
    uint32_t   bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0;
    }
    if( bitmask_loc & map->value_bitmap )
    {
        int child_count  = count_one_bits( map->child_bitmap );
        int physical_idx = count_one_bits( map->value_bitmap & bitmask_lower );
        *v = cow_deref_val( map, child_count, physical_idx );
        return 0;
    }
    else if( bitmask_loc & map->child_bitmap ) {
        int physical_idx = count_one_bits( map->child_bitmap & bitmask_lower );
        return cow_trie_lookup(
            cow_trie_children(map)[physical_idx], key >> BITS_PER_LVL, v );
    }
    else {
        return 1;
    }
}

int move_stuff_around(cow_trie_p map, int child_count, int value_count, uint32_t bitmask_loc,
                     cow_trie_p child, int v_physical_idx, int c_physical_idx) {
    uint32_t val_size = sizeof(val_key_t);
    uint32_t ptr_size = sizeof(cow_trie_p);
    int vals_greater = val_size > ptr_size;
    //9 cases
    //beginning of values array, end of child array
    if (v_physical_idx == 0 && c_physical_idx == child_count) {
        if (vals_greater) {
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
            memmove(cow_trie_values(map),
                    (char *)cow_trie_values(map) + val_size - ptr_size, 
                    value_count * val_size);
        }
        else {
            memmove((char *)cow_trie_values(map) + ptr_size, 
                    (char *)cow_trie_values(map) + val_size, 
                    value_count * val_size);
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
        }
    }
    //middle of values array, end of child array
    //could these be faster?
    else if (v_physical_idx > 0 && v_physical_idx < value_count && 
             c_physical_idx == child_count) {
        if (vals_greater) {
            memmove((char *)cow_trie_values(map) + ptr_size,
                    cow_trie_values(map),
                    val_size * v_physical_idx);
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
            memmove(&cow_trie_values(map)[v_physical_idx], 
                    (char *)&cow_trie_values(map)[v_physical_idx] + val_size - ptr_size, 
                    (value_count - v_physical_idx) * val_size);
        }
        else {
            memmove(&cow_trie_values(map)[1],
                    &cow_trie_values(map)[0],
                    val_size * v_physical_idx);
            map->child_bitmap = map->child_bitmap | bitmask_loc;
            memmove(cow_trie_values(map),
                    (char *)cow_trie_values(map) - (ptr_size - val_size), 
                    val_size * value_count);
            cow_trie_children(map)[c_physical_idx] = child;
        }
    }
    //end of values array, end of child array
    else if (v_physical_idx == value_count && c_physical_idx == child_count) {
        memmove((char *)cow_trie_values(map) + ptr_size,
                (char *)cow_trie_values(map),
                val_size * value_count);
        cow_trie_children(map)[c_physical_idx] = child;
        map->child_bitmap = map->child_bitmap | bitmask_loc;
    }
    //beginning of values array, beginning or middle of child array
    else if (v_physical_idx == 0 && c_physical_idx >= 0 && c_physical_idx < child_count) {
        if (vals_greater) {
            memmove(&cow_trie_children(map)[c_physical_idx + 1],
                    &cow_trie_children(map)[c_physical_idx],
                    (child_count - c_physical_idx) * ptr_size);
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
            memmove(cow_trie_values(map),
                    (char *)cow_trie_values(map) + (val_size - ptr_size),
                    value_count * val_size);
        }
        else {
            memmove((char *)cow_trie_values(map) + ptr_size, &cow_trie_values(map)[1], value_count * val_size);
            memmove(&cow_trie_children(map)[c_physical_idx + 1], &cow_trie_children(map)[c_physical_idx],
                    (child_count - c_physical_idx) * ptr_size);
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
        }
    }
    //middle of values array, beginning or middle of child array (worst and probably most frequent case)
    //could these be faster?
    else if (v_physical_idx > 0 && v_physical_idx < value_count && 
        c_physical_idx >= 0 && c_physical_idx < child_count) {
        if (vals_greater) {
            memmove((char *)cow_trie_values(map) + ptr_size,
                    cow_trie_values(map),
                    val_size * v_physical_idx);
            memmove(&cow_trie_children(map)[c_physical_idx + 1],
                    &cow_trie_children(map)[c_physical_idx],
                    (child_count - c_physical_idx) * ptr_size);
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
            memmove(&cow_trie_values(map)[v_physical_idx],
                    (char *)&cow_trie_values(map)[v_physical_idx] + (val_size - ptr_size),
                    (value_count - v_physical_idx) * val_size);
        }
        else {
            memmove((char *)&cow_trie_values(map)[v_physical_idx+1] + (ptr_size - val_size),
                    &cow_trie_values(map)[v_physical_idx+1],
                    (value_count - v_physical_idx) * val_size);
            memmove((char *)cow_trie_values(map) + (ptr_size - val_size),
                    cow_trie_values(map), val_size * v_physical_idx);
            memmove(&cow_trie_children(map)[c_physical_idx + 1],
                    &cow_trie_children(map)[c_physical_idx],
                    (child_count - c_physical_idx) * ptr_size);
            cow_trie_children(map)[c_physical_idx] = child;
            map->child_bitmap = map->child_bitmap | bitmask_loc;
        }
    }
    //end of values array, beginning or middle of child array
    else if (v_physical_idx == value_count  && 
        c_physical_idx >= 0 && c_physical_idx < child_count) {
        memmove((char *)cow_trie_values(map) + ptr_size, cow_trie_values(map), value_count * val_size);
        memmove(&cow_trie_children(map)[c_physical_idx + 1], &cow_trie_children(map)[c_physical_idx],
                (child_count - c_physical_idx) * ptr_size);
        cow_trie_children(map)[c_physical_idx] = child;
        map->child_bitmap = map->child_bitmap | bitmask_loc;
    }
    return 1;
}


int cow_trie_insert(cow_trie_p map, vkey_t key, val_t val, cow_trie_p *res) {
    int rc = 0;
    int virtual_idx = key & LOW_BITS_MASK;
    uint32_t bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0;
    }
    int child_count = count_one_bits(map->child_bitmap);
    int value_count = count_one_bits(map->value_bitmap);
    
    if (bitmask_loc & map->child_bitmap) {
        int physical_idx = count_one_bits(map->child_bitmap & bitmask_lower);
        cow_trie_p *n = &cow_trie_children(map)[physical_idx];
        if ((*n)->ref_count > 1) {
            cow_trie_p child_copy = cow_trie_copy_node(*n);
            --(*n)->ref_count;
            cow_trie_children(map)[physical_idx] = child_copy;
            return cow_trie_insert(child_copy, key >> BITS_PER_LVL, val, &child_copy);
        }
        else {
            return cow_trie_insert(*n, key >> BITS_PER_LVL, val, n);
        }
    }
    else if (bitmask_loc & map->value_bitmap) {
        int physical_idx        = count_one_bits(map->value_bitmap & bitmask_lower);
        cow_trie_p child        = cow_trie_alloc(0, 1);
        child->ref_count        = 1;
        child->child_bitmap     = 0;
        val_key_p vks           = cow_trie_values(map);
        val_key_p child_vks     = cow_trie_values(child);
        child_vks[0].val        = vks[physical_idx].val;
        child_vks[0].key_frag   = vks[physical_idx].key_frag >> BITS_PER_LVL;
        child->value_bitmap     = 1 << child_vks[0].key_frag;
        int c_physical_idx      = count_one_bits(map->child_bitmap & bitmask_lower);
        //if there's no room for a child, minus a value
        if (map->size < sizeof(cow_trie_t) + 
            (child_count + 1) * sizeof(cow_trie_p) + 
            (value_count-1) * sizeof(val_key_t)) {
            cow_trie_p n            = cow_trie_alloc(child_count+1, value_count);
            n->ref_count            = map->ref_count;
            n->value_bitmap         = map->value_bitmap ^ bitmask_loc;
            n->child_bitmap         = map->child_bitmap | bitmask_loc;
            cow_trie_p *new_children = cow_trie_children(n);
            cow_trie_p *old_children = cow_trie_children(map);
            memcpy(cow_trie_values(n), cow_trie_values(map), value_count * sizeof(val_key_t));
            memcpy(new_children, old_children, c_physical_idx * sizeof(cow_trie_p));
            memcpy(&new_children[c_physical_idx + 1],
                   &old_children[c_physical_idx],
                   (child_count - c_physical_idx) * sizeof(cow_trie_p));
            new_children[c_physical_idx] = child;
            rc = cow_trie_insert(child, key >> BITS_PER_LVL, val, &child);
            free(map);
            *res = n;
        }
        else {
            rc = cow_trie_insert(child, key >> BITS_PER_LVL, val, &child);
            move_stuff_around(map, child_count, value_count-1, bitmask_loc, child, physical_idx, c_physical_idx);
            map->value_bitmap = map->value_bitmap ^ bitmask_loc;
        }
        return 1;
    }
    else {
        int physical_idx = count_one_bits(map->value_bitmap & bitmask_lower);
        //if there's no room for a value
        if (map->size < sizeof(cow_trie_t) + 
            ((child_count) * sizeof(cow_trie_p)) + 
            ((value_count + 1) * sizeof(val_key_t))) {
            cow_trie_p n = cow_trie_alloc(child_count, value_count+1);
            n->ref_count    = map->ref_count;
            n->value_bitmap = map->value_bitmap | bitmask_loc;
            n->child_bitmap = map->child_bitmap;
            val_key_p new_vals = cow_trie_values(n);
            val_key_p old_vals = cow_trie_values(map);
            memcpy(cow_trie_children(n), cow_trie_children(map), child_count * sizeof(cow_trie_p));
            memcpy(new_vals, old_vals, physical_idx * sizeof(val_key_t));
            memcpy(&new_vals[physical_idx + 1],
                   &old_vals[physical_idx],
                   (value_count - physical_idx) * sizeof(val_key_t));
            new_vals[physical_idx].key_frag = key;
            new_vals[physical_idx].val = val;
            free(map);
            *res = n;
        }
        else {
            map->value_bitmap = map->value_bitmap | bitmask_loc;
            val_key_p vals = cow_trie_values(map);
            memmove(&vals[physical_idx+1], &vals[physical_idx], (value_count - physical_idx) * sizeof(val_key_t));
            vals[physical_idx].key_frag = key;
            vals[physical_idx].val = val;
        }
        return 1;
        
    }
    return 0;
}

int cow_trie_delete(cow_trie_p map, vkey_t key, cow_trie_p *res) {
    int rc = 0;
    int virtual_idx = key & LOW_BITS_MASK;
    uint32_t bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0;
    }
    int child_count = count_one_bits(map->child_bitmap);
    int value_count = count_one_bits(map->value_bitmap);
    
    if (bitmask_loc & map->child_bitmap) {
        int physical_idx = count_one_bits(map->child_bitmap & bitmask_lower);
        cow_trie_p *c = cow_trie_children(map);
        cow_trie_p n = c[physical_idx];
        cow_trie_p child;
        if ((n)->ref_count > 1) {
            child = cow_trie_copy_node(n);
            --(n)->ref_count;
            c[physical_idx] = child;
        }
        else {
            child = n;
        }
        rc = cow_trie_delete(child, key >> BITS_PER_LVL, &child);
        if (rc == 2) {
            free(child);
            map->child_bitmap = map->child_bitmap ^ bitmask_loc;
            memmove(&c[physical_idx], &c[physical_idx+1], (child_count - physical_idx) * sizeof(cow_trie_p) +
                                                          (value_count) * sizeof(val_key_t));
        }
        rc = 1;
    }
    else if (bitmask_loc & map->value_bitmap) {
        int physical_idx        = count_one_bits(map->value_bitmap & bitmask_lower);
        val_key_p vals = cow_trie_values(map);
        if (vals[physical_idx].key_frag == key) {
            map->value_bitmap = map->value_bitmap ^ bitmask_loc;
            memmove(&vals[physical_idx], &vals[physical_idx+1], (value_count - physical_idx) * sizeof(val_key_t));
            if (value_count == 1 && child_count == 0) {
                rc = 2;
            }
            else {
                rc = 1;
            }
        }
        else {
            rc = 0;
        }
    }
    else {
        rc = 0;        
    }
    return rc;
}

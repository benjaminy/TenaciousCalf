#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "c_t_2.h"

int count_one_bits( int_type bits )
{
#ifdef USE_UINT64_T
    #if __has_builtin(__builtin_popcountl)
        return __builtin_popcountl(bits);
    #else
        //TODO: put in some kinda popcountl function
    #endif
#else
    #if __has_builtin(__builtin_popcount)
        return __builtin_popcount( bits );
    #else
        static const int_type SK5 =0x55555555, SK3 =0x33333333, SK0F=0x0F0F0F0F;
        bits -= ( ( bits >> 1 ) & SK5 );
        bits = ( bits & SK3 )  + ( ( bits >> 2 ) & SK3 );
        bits = ( bits & SK0F ) + ( ( bits >> 4 ) & SK0F );
        bits += bits >> 8;
        return ( bits + ( bits >> 16 ) ) & 0x3F;
    #endif
#endif
}

int count_trailing_zeroes(int_type bits) {
#ifdef USE_UINT64_T
    #if __has_builtin(__builtin_ctzl)
        return __builtin_ctzl(bits);
    #else
        //TODO: put in some kind of ctzl function
    #endif
#else
    #if __has_builtin(__builtin_ctz)
        return __builtin_ctz(bits);
    #else
        static const int MultiplyDeBruijnBitPosition[32] = {
            0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
            31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
        };
        return MultiplyDeBruijnBitPosition[((int_type)((bits & -bits) * 0x077CB531U)) >> 27];
    #endif
#endif
}

cow_trie_p *cow_trie_children( cow_trie_p m )
{
    return (cow_trie_p *)&m->_[0];
}

val_p node_or_edge_alloc(int tag,
                         #ifdef COW_INCLUDE_PREDS
                         int num_preds,
                         #endif
                         int num_succs) {
    val_p n = (val_p)malloc(sizeof(n[0]));
    if (tag == 0) {
        #ifdef COW_INCLUDE_PREDS
        n->_.node.preds = edge_list_alloc(num_preds);
        #endif
        n->_.node.succs = edge_list_alloc(num_succs);
    }
    return n;
}

edge_list_p edge_list_alloc(int ct) {
    edge_list_p n = (edge_list_p)malloc(sizeof(n[0]) + ct * sizeof(int_type));
    return n;
}

int_type *edge_list_values(edge_list_p e) {
    return (int_type *)&e->_[0];
}

void cow_close_edge_list(edge_list_p e) {
    --e->ref_ct;
    if (e->ref_ct == 0) {
        free(e);
    }
}

val_t cow_trie_create_val(int tag,
                          #ifdef COW_INCLUDE_PREDS
                          size_t pred_ct,
                          #endif
                          size_t succ_ct,
                          #ifdef COW_INCLUDE_PREDS 
                          int_type *preds,
                          #endif
                          int_type *succs,
                          int_type pred,
                          int_type succ) {
    val_p thing = node_or_edge_alloc(tag, succ_ct);
    if (tag == 0) {
        thing->tag = 0;
        #ifdef COW_INCLUDE_PREDS
        thing->_.node.preds->ref_ct = 1;
        thing->_.node.pred_ct = pred_ct;
        #endif
        thing->_.node.succs->ref_ct = 1;
        thing->_.node.succ_ct = succ_ct;
    }
    else if (tag == 1) {
        thing->tag = 1;
        thing->_.edge.pred = pred;
        thing->_.edge.succ = succ;
    }
    return *thing;
}

static val_p cow_trie_values( cow_trie_p m )
{
    int child_count = count_one_bits( m->child_bitmap );
    return (val_p)&( ( cow_trie_children( m ) )[ child_count ] );
}

const int      CHILD_ARRAY_BUFFER_SIZE = 0;
const int      VALUE_ARRAY_BUFFER_SIZE = 0;
#ifdef USE_UINT64_T
const int_type           LOW_BITS_MASK = 0x3f;
const int_type            BITS_PER_LVL = 6;
const int_type            LVL_CAPACITY = 1UL << ( BITS_PER_LVL );
#else
const int_type           LOW_BITS_MASK = 0x1f;
const int_type            BITS_PER_LVL = 5;
const int_type            LVL_CAPACITY = 1 << ( BITS_PER_LVL );
#endif

cow_trie_p cow_trie_alloc( int children, int values ) {
    int_type size = sizeof( cow_trie_t ) + (children + CHILD_ARRAY_BUFFER_SIZE) * sizeof( cow_trie_p )
                    + (values + VALUE_ARRAY_BUFFER_SIZE) * sizeof(val_t);
    cow_trie_p n = (cow_trie_p)malloc(size);
    n->size = size;
    n->tot_size = size;
    return n;
}

cow_trie_p cow_trie_clone_node( cow_trie_p map, int children, int values )
{
    cow_trie_p m = (cow_trie_p)malloc(
               sizeof( m[0] ) + children * sizeof( cow_trie_p )
        + values * sizeof( val_t ) );
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
    val_p orig_values = cow_trie_values(map);
    val_p copy_values = cow_trie_values(copy);
    for (int i=0;i<num_children;i++) {
        ++orig_children[i]->ref_count;
        copy_children[i] = orig_children[i];
    }
    for (int i=0;i<num_values;i++) {
        if(orig_values[i].tag == 0) {
            #ifdef COW_INCLUDE_PREDS
            ++orig_values[i]._.node.preds->ref_ct;
            #endif
            ++orig_values[i]._.node.succs->ref_ct;
        }
        copy_values[i] = orig_values[i];
    }
    return copy;
}

void cow_trie_close_node(cow_trie_p node) {
    --node->ref_count;
    if (node->ref_count == 0) {
        int num_values = count_one_bits(node->value_bitmap);
        val_p vals = cow_trie_values(node);
        for (int i=0;i<num_values;i++) {
            if (vals[i].tag == 0) {
                #ifdef COW_INCLUDE_PREDS
                cow_close_edge_list(vals[i]._.node.preds);
                #endif
                cow_close_edge_list(vals[i]._.node.succs);
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
static val_t *cow_val_ref( cow_trie_p map, int ccount, int vidx )
{
    return (val_t *)&map->_[ ccount * sizeof( cow_trie_p )
                                 + vidx * sizeof( val_t ) ];
}

static val_t cow_deref_val( cow_trie_p map, int ccount, int vidx )
{
    return *cow_val_ref( map, ccount, vidx );
}


int cow_trie_lookup(
    cow_trie_p map, vkey_t key, val_t *v ) 
{
    int_type   virtual_idx = key & LOW_BITS_MASK;
    #ifdef USE_UINT64_T
    int_type   bitmask_loc = 1UL << virtual_idx;
    #else
    int_typ    bitmask_loc = 1 << virtual_idx;
    #endif
    int_type   bitmask_lower;
    int_type   shift_amt = LVL_CAPACITY - virtual_idx;
    #ifdef USE_UINT64_T
    if (shift_amt != LVL_CAPACITY) {
        bitmask_lower = ( ~0UL ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0UL;
    }
    #else
    if (shift_amt != LVL_CAPACITY) {
        bitmask_lower = ( ~0U ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0;
    }
    #endif
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

int change_value_to_child(cow_trie_p map, int child_count, int value_count, int_type bitmask_loc,
                     cow_trie_p child, int v_physical_idx, int c_physical_idx) {
    int_type val_size = sizeof(val_t);
    int_type ptr_size = sizeof(cow_trie_p);
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

int_type cow_trie_insert(cow_trie_p map, val_t val, cow_trie_p *res) {
    int_type key_or_virtual_idx = 0;
    int child_count = count_one_bits(map->child_bitmap);
    int value_count = count_one_bits(map->value_bitmap);
    int_type free_val_bitmap = ~map->value_bitmap & ~map->child_bitmap;
    if (free_val_bitmap) {
        key_or_virtual_idx = count_trailing_zeroes(free_val_bitmap);
        int_type shift_amt = LVL_CAPACITY - key_or_virtual_idx;
        int_type bitmask_lower = 0;
        #ifdef USE_UINT64_T
        if (key_or_virtual_idx != 0UL) {
            bitmask_lower = ( ~0UL ) >> ( shift_amt );
        }
        #else
        if (key_or_virtual_idx != 0) {
            bitmask_lower = ( ~0U ) >> ( shift_amt );
        }
        #endif
        int physical_idx = count_one_bits(map->value_bitmap & bitmask_lower);
        #ifdef USE_UINT64_T
        int_type bitmask_loc = 1UL << key_or_virtual_idx;
        #else
        int_type bitmask_loc = 1 << key_or_virtual_idx;
        #endif
        //if there's no room for a value
        if (map->size < sizeof(cow_trie_t) + 
            ((child_count) * sizeof(cow_trie_p)) + 
            ((value_count + 1) * sizeof(val_t))) {
            cow_trie_p n = cow_trie_alloc(child_count, value_count+1);
            n->ref_count    = map->ref_count;
            n->value_bitmap = map->value_bitmap | bitmask_loc;
            n->child_bitmap = map->child_bitmap;
            n->tot_size = map->tot_size + sizeof(val_t);
            val_p new_vals = cow_trie_values(n);
            val_p old_vals = cow_trie_values(map);
            memcpy(cow_trie_children(n), cow_trie_children(map), child_count * sizeof(cow_trie_p));
            memcpy(new_vals, old_vals, physical_idx * sizeof(val_t));
            memcpy(&new_vals[physical_idx + 1],
                   &old_vals[physical_idx],
                   (value_count - physical_idx) * sizeof(val_t));
            new_vals[physical_idx] = val;
            free(map);
            *res = n;
        }
        else {
            map->value_bitmap = map->value_bitmap | bitmask_loc;
            val_p vals = cow_trie_values(map);
            memmove(&vals[physical_idx+1], &vals[physical_idx], (value_count - physical_idx) * sizeof(val_t));
            vals[physical_idx] = val;
            map->tot_size += sizeof(val_t);
        }
    }
    else {
        if (~map->child_bitmap) {
            int physical_idx        = count_trailing_zeroes(~map->child_bitmap);
            key_or_virtual_idx      = physical_idx;
            #ifdef USE_UINT64_T
            int_type bitmask_loc    = 1UL << physical_idx;
            #else
            int_type bitmask_loc    = 1 << physical_idx;
            #endif
            cow_trie_p child        = cow_trie_alloc(0, 1);
            child->ref_count        = 1;
            child->child_bitmap     = 0;
            val_p vks               = cow_trie_values(map);
            val_p child_vks         = cow_trie_values(child);
            child_vks[0]            = vks[0];
            child->value_bitmap     = 1;

            key_or_virtual_idx      = cow_trie_insert(child, val, &child) << BITS_PER_LVL | key_or_virtual_idx;
            #ifdef USE_UINT64_T
            change_value_to_child(map, child_count, value_count-1UL, bitmask_loc, child, 0UL, physical_idx);
            #else
            change_value_to_child(map, child_count, value_count-1, bitmask_loc, child, 0, physical_idx);
            #endif
            map->value_bitmap       = map->value_bitmap ^ bitmask_loc;
            map->child_bitmap       = map->child_bitmap | bitmask_loc;

            map->tot_size = map->tot_size - sizeof(val_t) + sizeof(cow_trie_p);
        }
        else {
            //find least populated child
            #ifdef USE_UINT64_T
            int_type lp_size = UINT64_MAX-1;
            #else
            int_type lp_size = UINT32_MAX-1;
            #endif
            int_type lp_index = 0;
            int_type current_size = 0;
            cow_trie_p *children = cow_trie_children(map);
            int_type child_bitmap = map->child_bitmap;
            for (int i=0;i<child_count;i++) {
                current_size = children[i]->tot_size;
                if (current_size < lp_size) {
                    lp_size = current_size;
                    lp_index = i;
                    key_or_virtual_idx = count_trailing_zeroes(child_bitmap);
                }
                child_bitmap = child_bitmap & (child_bitmap - 1);
            }
            cow_trie_p *lp_child = &children[lp_index];
            //insert into the least populated child
            if ((*lp_child)->ref_count > 1) {
                cow_trie_p lp_child_copy = cow_trie_copy_node(*lp_child);
                --(*lp_child)->ref_count;
                cow_trie_children(map)[lp_index] = lp_child_copy;
                key_or_virtual_idx = cow_trie_insert(lp_child_copy, val, &lp_child_copy) << BITS_PER_LVL |
                                                     key_or_virtual_idx;
            }
            else {
                key_or_virtual_idx = cow_trie_insert(*lp_child, val, lp_child) << BITS_PER_LVL |
                                                     key_or_virtual_idx;
            }
            //update the total size of everything under this node, if we need to
            current_size = children[lp_index]->tot_size;
            if (current_size > lp_size) {
                map->tot_size += (current_size - lp_size);
            }
        }
    }
    //return the key
    return key_or_virtual_idx;
}

int cow_trie_delete(cow_trie_p map, vkey_t key, cow_trie_p *res) {
    int rc = 0;
    int_type virtual_idx = key & LOW_BITS_MASK;
    #ifdef USE_UINT64_T
    int_type bitmask_loc = 1UL << virtual_idx;
    #else
    int_type bitmask_loc = 1 << virtual_idx;
    #endif
    int_type bitmask_lower;
    int_type shift_amt = LVL_CAPACITY - virtual_idx;
    #ifdef USE_UINT64_T
    if (virtual_idx != 0UL) {
        bitmask_lower = ( ~0UL ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0UL;
    }
    #else
    if (virtual_idx != 0) {
        bitmask_lower = ( ~0UL ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0;
    }
    #endif
    int child_count = count_one_bits(map->child_bitmap);
    int value_count = count_one_bits(map->value_bitmap);
    
    if (bitmask_loc & map->child_bitmap) {
        int physical_idx = count_one_bits(map->child_bitmap & bitmask_lower);
        cow_trie_p *c = cow_trie_children(map);
        cow_trie_p n = c[physical_idx];
        cow_trie_p child;
        if (n->ref_count > 1) {
            cow_trie_p child_copy = cow_trie_copy_node(n);
            --n->ref_count;
            c[physical_idx] = child_copy;
            child = child_copy;
        }
        else {
            child = n;
        }
        rc = cow_trie_delete(child, key >> BITS_PER_LVL, &child);
        if (rc == 2) {
            free(child);
            map->child_bitmap = map->child_bitmap ^ bitmask_loc;
            memmove(&c[physical_idx], &c[physical_idx+1], (child_count - physical_idx) * sizeof(cow_trie_p) +
                                                          (value_count) * sizeof(val_t));
            map->tot_size -= sizeof(cow_trie_p);
        }
        else if (rc == 1) {
            map->tot_size -= sizeof(val_t);
        }
        rc = 1;
    }
    else if (bitmask_loc & map->value_bitmap) {
        int physical_idx = count_one_bits(map->value_bitmap & bitmask_lower);
        val_p vals = cow_trie_values(map);
        map->value_bitmap = map->value_bitmap ^ bitmask_loc;
        val_p val = &vals[physical_idx];
        if (val->tag == 0) {
            #ifdef COW_INCLUDE_PREDS
            cow_close_edge_list(val->_.node.preds);
            #endif
            cow_close_edge_list(val->_.node.succs);
        }
        memmove(&vals[physical_idx], &vals[physical_idx+1], ((value_count - 1) - physical_idx) * sizeof(val_t));
        map->tot_size -= sizeof(val_t);
        if (value_count == 1 && child_count == 0) {
            rc = 2;
        }
        else {
            rc = 1;
        }
    }
    else {
        rc = 3;        
    }
    return rc;
}

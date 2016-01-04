#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int count_one_bits( uint32_t bits )
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

typedef uint32_t val_t;
typedef uint32_t key_t;

typedef struct val_key_t val_key_t, *val_key_p;
struct val_key_t
{
    uint32_t key_frag;
    val_t val;
};

/* TODO: Think about the trade-off between putting the meta-information
 * with the node itself (as it is now) versus pulling it up to the
 * parent.  Storing the meta-information with the parent might be better
 * for locality.  However, in a persistent world there can be multiple
 * references to a single node.  It's not clear where the
 * meta-information should be in that case (multiple copies?  Seems
 * ugly.)
 */

/* Using the variable-sized struct trick.  There are actually two
 * arrays:
 * - cow_trie_next_p children[ child_n ]
 * - val_key_t      values[ value_n ] */
typedef struct cow_trie_t cow_trie_t, *cow_trie_p;
    struct cow_trie_t
{
    int             ref_count;
    uint32_t        child_bitmap, value_bitmap;
    char            _[0];
};

static cow_trie_p *cow_trie_children( cow_trie_p m )
{
    return (cow_trie_p *)&m->_[0];
}

static val_key_p cow_trie_values( cow_trie_p m )
{
    int child_count = count_one_bits( m->child_bitmap );
    return (val_key_p)&( ( cow_trie_children( m ) )[ child_count ] );
}

const uint32_t LOW_BITS_MASK = 0x1f;
const int      BITS_PER_LVL = 5;
const int      LVL_CAPACITY = 1 << ( BITS_PER_LVL );

cow_trie_p cow_trie_alloc( int children, int values )
{
    return (cow_trie_p)malloc(
        sizeof( cow_trie_t ) + children * sizeof( cow_trie_p )
        + values * sizeof(val_key_t));
}

cow_trie_p cow_trie_clone_node( cow_trie_p map, int children, int values )
{
    cow_trie_p m = malloc(
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
    cow_trie_p * orig_children = cow_trie_children(map);
    cow_trie_p * copy_children = cow_trie_children(copy);
    for (int i=0;i<num_children;i++) {
        copy_children[i] = orig_children[i];
    }
    return copy;
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
    cow_trie_p map, key_t key, val_t *v )
{
    int        virtual_idx = key & LOW_BITS_MASK;
    uint32_t   bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( LVL_CAPACITY - virtual_idx );
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
    else if( bitmask_loc & map->child_bitmap )
    {
        int physical_idx = count_one_bits( map->child_bitmap & bitmask_lower );
        return cow_trie_lookup(
            cow_trie_children(map)[physical_idx], key >> BITS_PER_LVL, v );
    }
    else
    {
        return 1;
    }
}


int cow_trie_insert(cow_trie_p map, key_t key, val_t val, cow_trie_p *res) {
    int rc = 0;
    int virtual_idx = key & LOW_BITS_MASK;
    uint32_t bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( LVL_CAPACITY - virtual_idx );
    }
    else {
        bitmask_lower = 0;
    }
    int child_count = count_one_bits(map->child_bitmap);
    int value_count = count_one_bits(map->value_bitmap);
    
    if (bitmask_loc & map->child_bitmap) {
        int physical_idx = count_one_bits(map->child_bitmap & bitmask_lower);
        cow_trie_p n = cow_trie_children(map)[physical_idx];
        if (n->ref_count > 1) {
            cow_trie_p child_copy = cow_trie_copy_node(n);
            --n->ref_count;
            return cow_trie_insert(child_copy, key >> BITS_PER_LVL, val, &child_copy);
        }
        else {
            return cow_trie_insert(n, key >> BITS_PER_LVL, val, &n);
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
        cow_trie_p n            = cow_trie_alloc(child_count+1, value_count);
        n->value_bitmap         = map->value_bitmap ^ bitmask_loc;
        n->child_bitmap         = map->child_bitmap | bitmask_loc;
        int c_physical_idx      = count_one_bits(map->child_bitmap & bitmask_lower);
        cow_trie_p* new_children  = cow_trie_children(n);
        memcpy(cow_trie_values(n), cow_trie_values(map), value_count * sizeof(val_key_t));
        memcpy(new_children, cow_trie_children(map), c_physical_idx * sizeof(cow_trie_p));
        memcpy(new_children + (c_physical_idx + 1) * sizeof(cow_trie_p),
               cow_trie_children(map) + c_physical_idx * sizeof(cow_trie_p),
               (child_count - c_physical_idx) * sizeof(cow_trie_p));
        rc = cow_trie_insert(child, key >> BITS_PER_LVL, val, &child);
        new_children[c_physical_idx] = child;
        *res = n;
        return 1;
    }
    else {
        int physical_idx = count_one_bits(map->value_bitmap & bitmask_lower);
        cow_trie_p n = cow_trie_alloc(child_count, value_count+1);
        n->value_bitmap = map->value_bitmap | bitmask_loc;
        n->child_bitmap = map->child_bitmap;
        val_key_p new_vals = cow_trie_values(n);
        memcpy(cow_trie_children(n), cow_trie_children(map), child_count * sizeof(cow_trie_p));
        memcpy(new_vals, cow_trie_values(map), physical_idx * sizeof(val_key_t));
        memcpy(new_vals + (physical_idx + 1) * sizeof(val_key_t),
               cow_trie_values(map) + physical_idx * sizeof(val_key_t),
               (value_count - physical_idx) * sizeof(val_key_t));
        new_vals[physical_idx].key_frag = key >> BITS_PER_LVL;
        new_vals[physical_idx].val = val;
        *res = n;
        return 1;
        
    }
    return 0;
}


int main( int argc, char **argv )
{
    cow_trie_p f = cow_trie_alloc(0, 0);
    f->child_bitmap = 0;
    f->value_bitmap = 0;
    cow_trie_insert(f, 5, 10, &f);
    uint32_t q = 0;
    cow_trie_insert(f, 37, 14, &f);
    cow_trie_p abc = cow_trie_copy_node(f);
    cow_trie_insert(abc, 501472, 123, &abc);
    cow_trie_lookup(abc, 501472, &q);
    printf("value is %d", q);
    fflush(stdout);
}

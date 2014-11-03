#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


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
    uint32_t hash_frag;
    key_t key;
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
 * - lgmt_hash_map_next_p children[ child_n ]
 * - val_key_t      values[ value_n ] */
typedef struct lgmt_hash_map_t lgmt_hash_map_t, *lgmt_hash_map_p;
struct lgmt_hash_map_t
{
    int             ref_count;
    uint32_t        child_bitmap, value_bitmap;
    char            _[0];
};

static lgmt_hash_map_p *lgmt_hash_map_children( lgmt_hash_map_p m )
{
    return (lgmt_hash_map_p *)&m->tail[0];
}

static val_key_p lgmt_hash_map_values( lgmt_hash_map_p m )
{
    int child_count = count_one_bits( m->child_bitmap );
    return (val_key_p)&( ( lgmt_hash_map_children( m ) )[ child_count ] );
}

const uint32_t LOW_BITS_MASK = 0x1f;
const int      BITS_PER_LVL = 5;
const int      LVL_CAPACITY = 1 << ( BITS_PER_LVL );

lgmt_hash_map_p lgmt_hash_map_alloc( int children, int values )
{
    return (lgmt_hash_map_p)malloc(
        sizeof( lgmt_hash_map_t ) + children * sizeof( lgmt_hash_map_p )
        + values * sizeof( val_key_t ) );
}

lgmt_hash_map_p lgmt_hash_map_clone_node( lgmt_hash_map_p map, int children, int values )
{
    lgmt_hash_map_p m = malloc(
        sizeof( m[0] ) + children * sizeof( lgmt_hash_map_p )
        + values * sizeof( val_key_t ) );
    if( m )
    {
        *m = *map;
    }
    return m;
}

static val_key_t *lgmt_val_ref( lgmt_hash_map_p map, int ccount, int vidx )
{
    return (val_key_t *)&map->_[ ccount * sizeof( lgmt_hash_map_p )
                                 + vidx * sizeof( val_key_t ) ];
}

static val_t lgmt_deref_val( lgmt_hash_map_p map, int ccount, int vidx )
{
    return lgmt_val_ref( map, ccount, vidx )->val;
}

int lgmt_hash_map_lookup(
    lgmt_hash_map_p map, key_t key, uint32_t hash, int level, val_t *v )
{
    int        virtual_idx = hash & LOW_BITS_MASK;
    uint32_t   bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower = ( ~0U ) >> ( LVL_CAPACITY - virtual_idx );

    if( bitmask_loc & map->value_bitmap )
    {
        int child_count  = count_one_bits( map->child_bitmap );
        int physical_idx = count_one_bits( map->value_bitmap & bitmask_lower );
        *v = lgmt_deref_val( map, child_count, physical_idx );
        return 0;
    }
    else if( bitmask_loc & map->child_bitmap )
    {
        int physical_idx = count_one_bits( map->child_bitmap & bitmask_lower );
        return lgmt_hash_map_lookup(
            lgmt_hash_map_p map, key_t key, uint32_t hash, int level, val_t *v )
    }
    else
    {
        
    }
}

int lgmt_hash_map_insert(
    lgmt_hash_map_p map, key_t key, val_t val,
    uint32_t hash, int level, lgmt_hash_map_p *res )
{
    int                 rc = 0;
    int        virtual_idx = hash & LOW_BITS_MASK;
    uint32_t   bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower = ( ~0U ) >> ( LVL_CAPACITY - virtual_idx );
    int        child_count = count_one_bits( map->child_bitmap );
    int        value_count = count_one_bits( map->value_bitmap );
    if( BITS_PER_LVL * level > LVL_CAPACITY )
    {
        /* XXX */
        assert( 0 );
    }
    assert( !( ( bitmask_loc & map->child_bitmap )
            && ( bitmask_loc & map->value_bitmap ) ) );
    if( bitmask_loc & map->child_bitmap )
    {
        int physical_idx = count_one_bits( map->child_bitmap & bitmask_lower );
        /* goto child */
        lgmt_hash_map_p n;
        return lgmt_hash_map_insert(
            lgmt_hash_map_children( map )[ physical_idx ], key, val,
            hash >> BITS_PER_LVL, level + 1, &n );
    }
    /* else */ if( bitmask_loc & map->value_bitmap )
    {
        int         physical_idx = count_one_bits( map->value_bitmap & bitmask_lower );
        lgmt_hash_map_p    child = lgmt_hash_map_alloc( 0, 1 );
        child->ref_count         = 1;
        child->child_bitmap      = 0;
        val_key_p            vks = lgmt_hash_map_values( map );
        val_key_p      child_vks = lgmt_hash_map_values( child );
        child_vks[ 0 ].val       = vks[ physical_idx ].val;
        child_vks[ 0 ].hash_frag = vks[ physical_idx ].hash_frag >> BITS_PER_LVL;
        child->value_bitmap      = 1 << child_vks[ 0 ].hash_frag;
        lgmt_hash_map_p n;
        rc = lgmt_hash_map_insert( child, key, val, hash >> BITS_PER_LVL, level + 1, &n );

        *res = lgmt_hash_map_clone_node( map, child_count, value_count );

        //return lgmt_hash_map_insert(
        //    &map->children[ physical_idx ], hash >> SHIFT_AMT, val );
    }
    else
    {
        int physical_idx = count_one_bits( map->value_bitmap & bitmask_lower );
        *res = lgmt_hash_map_clone_node( map, child_count, value_count + 1 );
        memcpy( lgmt_hash_map_children( (*res) ), lgmt_hash_map_children( map ),
                child_count * sizeof( lgmt_hash_map_p ) );
        memcpy( lgmt_hash_map_values( (*res) ), lgmt_hash_map_values( map ),
                physical_idx * sizeof( val_key_t ) );
        memcpy( lgmt_hash_map_values( (*res) ) + physical_idx + 1,
                lgmt_hash_map_values( map ) + physical_idx,
                ( child_count - physical_idx ) * sizeof( val_key_t ) );
        /* add value */
    }
    return rc;
}

int main( int argc, char **argv )
{
    return 0;
}

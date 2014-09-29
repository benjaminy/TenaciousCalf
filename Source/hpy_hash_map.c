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

typedef struct hpy_hash_map_t hpy_hash_map_t, *hpy_hash_map_p;
struct hpy_hash_map_t
{
    int      ref_count;
    uint32_t child_bitmap, value_bitmap;
    /* Using the variable-sized struct trick.  After the fields above,
     * there are actually two arrays:
     * - hpy_hash_map_p children[ child_n ]
     * - val_key_t      values[ value_n ] */
    char tail[1]; /* XXX Might be wasting a few bytes here. */
};

static hpy_hash_map_p *hpy_hash_map_children( hpy_hash_map_p m )
{
    return (hpy_hash_map_p *)&m->tail[0];
}

static val_key_p hpy_hash_map_values( hpy_hash_map_p m )
{
    int child_count = count_one_bits( m->child_bitmap );
    return (val_key_p)&( ( hpy_hash_map_children( m ) )[ child_count ] );
}

const uint32_t LOW_BITS_MASK = 0x1f;
const int      BITS_PER_LVL = 5;
const int      LVL_CAPACITY = 1 << ( BITS_PER_LVL );

hpy_hash_map_p hpy_hash_map_alloc( int children, int values )
{
    return (hpy_hash_map_p)malloc(
        sizeof( hpy_hash_map_t ) + children * sizeof( hpy_hash_map_p )
        + values * sizeof( val_key_t ) );
}

hpy_hash_map_p hpy_hash_map_clone_node( hpy_hash_map_p map, int children, int values )
{
    hpy_hash_map_p m = malloc(
        sizeof( m[0] ) + children * sizeof( hpy_hash_map_p )
        + values * sizeof( val_key_t ) );
    if( m )
    {
        *m = *map;
    }
    return m;
}

int hpy_hash_map_insert(
    hpy_hash_map_p map, key_t key, val_t val,
    uint32_t hash, int level, hpy_hash_map_p *res )
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
        hpy_hash_map_p n;
        rc = hpy_hash_map_insert(
            hpy_hash_map_children( map )[ physical_idx ], key, val,
            hash >> BITS_PER_LVL, level + 1, &n );
        //*res;
    }
    else if( bitmask_loc & map->value_bitmap )
    {
        int         physical_idx = count_one_bits( map->value_bitmap & bitmask_lower );
        hpy_hash_map_p     child = hpy_hash_map_alloc( 0, 1 );
        child->ref_count         = 1;
        child->child_bitmap      = 0;
        val_key_p            vks = hpy_hash_map_values( map );
        val_key_p      child_vks = hpy_hash_map_values( child );
        child_vks[ 0 ].val       = vks[ physical_idx ].val;
        child_vks[ 0 ].hash_frag = vks[ physical_idx ].hash_frag >> BITS_PER_LVL;
        child->value_bitmap      = 1 << child_vks[ 0 ].hash_frag;
        hpy_hash_map_p n;
        rc = hpy_hash_map_insert( child, key, val, hash >> BITS_PER_LVL, level + 1, &n );

        *res = hpy_hash_map_clone_node( map, child_count, value_count );

        //return hpy_hash_map_insert(
        //    &map->children[ physical_idx ], hash >> SHIFT_AMT, val );
    }
    else
    {
        int physical_idx = count_one_bits( map->value_bitmap & bitmask_lower );
        *res = hpy_hash_map_clone_node( map, child_count, value_count + 1 );
        memcpy( hpy_hash_map_children( (*res) ), hpy_hash_map_children( map ),
                child_count * sizeof( hpy_hash_map_p ) );
        memcpy( hpy_hash_map_values( (*res) ), hpy_hash_map_values( map ),
                physical_idx * sizeof( val_key_t ) );
        memcpy( hpy_hash_map_values( (*res) ) + physical_idx + 1,
                hpy_hash_map_values( map ) + physical_idx,
                ( child_count - physical_idx ) * sizeof( val_key_t ) );
        /* add value */
    }
    return rc;
}

int main( int argc, char **argv )
{
    return 0;
}

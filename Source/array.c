#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#define TEST_ARRAY
#define TEST_TREE

#define N ( 32 * 1024 * 1024 )
#define M ( 10 * 1000 * 1000 )

#define F 32

typedef struct leaf_t leaf_t, *leaf_p;

struct leaf_t
{
    leaf_p left;
    unsigned data[F];
    leaf_p right;
};

typedef union seq_tree_t seq_tree_t, *seq_tree_p;
union seq_tree_t
{
    struct
    {
        unsigned range, child_n, child_max, tag;
        seq_tree_p _;
    } nodes[F];
    struct
    {
        unsigned range;
        leaf_p _;
    } leaves[F];
};

typedef struct iterator_t iterator_t, *iterator_p;

struct iterator_t
{
    leaf_p l;
    unsigned i;
};

int iterator_first( seq_tree_p t, unsigned tag, iterator_p i )
{
    if( tag )
    {
        i->l = t->leaves[0]._;
        i->i = 0;
        return 0;
    }
    else
    {
        return iterator_first( t->nodes[ 0 ]._, t->nodes[ 0 ].tag, i );
    }
}

int has_next( iterator_p i )
{
    return ( !!i->l->right ) || ( i->i < F );
}

int advance( iterator_p i )
{
    ++i->i;
    if( i->i < F )
    {
        return 0;
    }
    if( i->l->right )
    {
        i->l = i->l->right;
        i->i = 0;
        __builtin_prefetch( i->l->right );
        return 0;
    }
    return 0;
}

int get( iterator_p i, unsigned *res )
{
    if( i && i->l && i->i < F && res )
    {
        *res = i->l->data[ i->i ];
        return 0;
    }
    return 1;
}

seq_tree_p seq_alloc(
    unsigned level, unsigned *n, unsigned *max_range, unsigned *tag, leaf_p *prev );
int seq_set( seq_tree_p t, unsigned idx, unsigned val,
             unsigned max, unsigned n, unsigned tag );
static int seq_get( seq_tree_p t, unsigned idx, unsigned level, unsigned *p,
             unsigned max, unsigned n, unsigned tag );
int seq_set_mv( seq_tree_p t, unsigned idx, unsigned val,
                unsigned max, unsigned n, unsigned tag, seq_tree_p *res );



struct timeval time_diff( struct timeval start, struct timeval end );
int print_time( FILE *f, struct timeval tv );

int main( int argc, char** argv )
{
    struct timeval start, end, duration;
    srand( 42 );
    unsigned x;
#ifdef TEST_ARRAY
    printf( "Array alloc\n" );
    assert( !gettimeofday( &start, NULL ) );
    unsigned *a = (unsigned *)malloc( N * sizeof( a[0] ) );
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    printf( "Array init\n" );
    assert( !gettimeofday( &start, NULL ) );
    for( unsigned i = 0; i < N; ++i )
    {
        a[ i ] = rand();
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    x = 0;
    printf( "Array reads %u\n", x );
    assert( !gettimeofday( &start, NULL ) );

    // for( unsigned i = 0; i < M; ++i )
    for( unsigned i = 0; i < M; ++i )
    {
        unsigned idx = rand() % N;
        // unsigned idx = i % N;
        // printf( "   %u %u", idx, a[ idx ] );
        x += a[ idx ];
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );
    printf( "Magic number: %u\n", x );
    free( a );
#endif
    srand( 42 );

    assert( ( F * F * F * F * F ) == N );
#ifdef TEST_TREE
    unsigned levels = 5;
    printf( "Tree alloc\n" );
    assert( !gettimeofday( &start, NULL ) );
    unsigned n, max_range, tag;
    leaf_p prev = NULL;
    seq_tree_p t = seq_alloc( levels, &n, &max_range, &tag, &prev );
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    printf( "??? %p\n", t->leaves[ 1 ]._->data );

    printf( "Tree init %u  mr=%u\n", t->nodes[ F /* XXX */ - 1 ].range, max_range );
    assert( !gettimeofday( &start, NULL ) );
    for( unsigned i = 0; i < N; ++i )
    {
        unsigned val = rand();
        seq_set( t, i, val, max_range, n, tag );
        // seq_tree_p temp;
        // seq_set_mv( t, i, val, max_range, n, tag, &temp );
        // t = temp;
        // printf( "  set[%u] (4) %u", i, t->leaves[0]._[4] );
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    x = 0;
    printf( "Tree reads %u\n", x );
    assert( !gettimeofday( &start, NULL ) );
#if 1
    for( unsigned i = 0; i < M; ++i )
    {
        unsigned v;
        unsigned idx = rand() % N;
        // unsigned idx = i % N;
        seq_get( t, idx, levels - 1, &v, max_range, n, tag );
        // printf( "   %u %u", idx, v );
        x += v;
    }
#else
    iterator_t iter;
    for( assert( !iterator_first( t, tag, &iter ) );
         has_next( &iter );
         assert( !advance( &iter ) ) )
    {
        unsigned val;
        assert( !get( &iter, &val ) );
        x += val;
    }
#endif
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );
    printf( "Magic number: %u\n", x );
#endif
    return 0;
}

#define SUBR(t, tag, idx) ( tag ? t->leaves[ idx ].range : t->nodes[ idx ].range )

seq_tree_p seq_alloc(
    unsigned level, unsigned *n, unsigned *max_range, unsigned *tag, leaf_p *prev )
{
    seq_tree_p t = (seq_tree_p)malloc( sizeof(t[0]) );
    if( level < 2 )
    {
        /* XXX deprecated */
        assert( 0 );
        leaf_p leaf = (leaf_p)malloc( sizeof(leaf[0]) );
        *n = 1;
        *max_range = F;
        t->leaves[0].range = F;
        t->leaves[0]._ = leaf;
    }
    else
    {
        *n = F;
        unsigned range = 0;
        for( unsigned i = 0; i < F; ++i )
        {
            if( level > 2 )
            {
                seq_tree_p nd = seq_alloc(
                    level - 1, &t->nodes[i].child_n,
                    &t->nodes[i].child_max, &t->nodes[i].tag, prev );
                range += SUBR( nd, level == 3, t->nodes[i].child_n - 1);
                // printf( " r%u", range );
                t->nodes[i]._ = nd;
                t->nodes[i].range = range;
            }
            else
            {
                range += F;
                t->leaves[i]._ = (leaf_p)malloc( sizeof( t->leaves[i]._[0] ) );
                t->leaves[i].range = range;
                t->leaves[i]._->left = *prev;
                t->leaves[i]._->right = NULL;
                if( *prev )
                {
                    (*prev)->right = t->leaves[i]._;
                }
                *prev = t->leaves[i]._;
                // printf( " i=%u L%p\n", i, t->leaves[i]._ );
            }
        }
        // printf( "  r=%u", range );
        *max_range = range;
        *tag = ( 2 == level );
    }
    return t;
}

int seq_set( seq_tree_p t, unsigned idx, unsigned val,
             unsigned max, unsigned n, unsigned tag )
{
    // printf( "seq_set idx=%u t=%p n=%u m=%u\n", idx, t, n, max );
    unsigned guess = idx / ( max / n );
    while( 1 )
    {
        if( idx >= SUBR( t, tag, guess ) )
        {
            printf( "\n\nTOO HIGH %u\n\n", guess );
            --guess;
        }
        else if( guess > 0 && idx < SUBR( t, tag, guess - 1 ) )
        {
            printf( "\n\nTOO LOW %u\n\n", guess );
            ++guess;
        }
        else
        {
            break;
        }
    }
    unsigned prev_range = guess < 1 ? 0 : SUBR( t, tag, guess - 1 );
    unsigned next_idx = idx - prev_range;
    //printf( "  g=%u  prv=%u  r=%u  nx=%u  v=%u\n", guess, prev_range,
    //        SUBR( t, tag, guess ), next_idx, val );
    if( tag )
    {
        // printf( "  set t=%p L=%p\n", t, t->leaves[ guess ]._ );
        t->leaves[ guess ]._->data[ next_idx ] = val;
        // (t->leaves[ guess ]._)[ next_idx ] = val;
        return 0;
    }
    else
    {
        return seq_set( t->nodes[ guess ]._, next_idx, val,
                        t->nodes[ guess ].child_max, t->nodes[ guess ].child_n,
                        t->nodes[ guess ].tag );
    }
}

int seq_set_mv( seq_tree_p t, unsigned idx, unsigned val,
                unsigned max, unsigned n, unsigned tag, seq_tree_p *res )
{
    // printf( "seq_set idx=%u t=%p n=%u m=%u\n", idx, t, n, max );
    unsigned guess = idx / ( max / n );
    while( 1 )
    {
        if( idx >= SUBR( t, tag, guess ) )
        {
            printf( "\n\nTOO HIGH %u\n\n", guess );
            --guess;
        }
        else if( guess > 0 && idx < SUBR( t, tag, guess - 1 ) )
        {
            printf( "\n\nTOO LOW %u\n\n", guess );
            ++guess;
        }
        else
        {
            break;
        }
    }
    unsigned prev_range = guess < 1 ? 0 : SUBR( t, tag, guess - 1 );
    unsigned next_idx = idx - prev_range;
    //printf( "  g=%u  prv=%u  r=%u  nx=%u  v=%u\n", guess, prev_range,
    //        SUBR( t, tag, guess ), next_idx, val );
    if( tag )
    {
        // printf( "  set t=%p L=%p\n", t, t->leaves[ guess ]._ );
        // (t->leaves[ guess ]._)[ next_idx ] = val;
        seq_tree_p t_new = (seq_tree_p)malloc( sizeof( t_new[0] ) );
        memcpy( t_new, t, sizeof( t[0] ) );
        t_new->leaves[ guess ]._ = (leaf_p)malloc( sizeof( t_new->leaves[guess]._[0] ) );
        memcpy( t_new->leaves[ guess ]._, t->leaves[ guess ]._,
                F * sizeof( t_new->leaves[guess]._[0] ) );
        t_new->leaves[ guess ]._->data[ next_idx ] = val;
        free( t->leaves[ guess ]._ );
        free( t );
        *res = t_new;
        return 0;
    }
    else
    {
        seq_tree_p child_new;
        int rc = seq_set_mv(
            t->nodes[ guess ]._, next_idx, val, t->nodes[ guess ].child_max,
            t->nodes[ guess ].child_n, t->nodes[ guess ].tag, &child_new );
        seq_tree_p t_new = (seq_tree_p)malloc( sizeof( t_new[0] ) );
        memcpy( t_new, t, sizeof( t[0] ) );
        t_new->nodes[ guess ]._ = child_new;
        free( t );
        *res = t_new;
        return rc;
    }
}

static int seq_get( seq_tree_p t, unsigned idx, unsigned level, unsigned *p,
             unsigned max, unsigned n, unsigned tag )
{
#if 1
    // printf( "seq_get t=%p n=%u r0=%u\n", t, t->n, t->ranges[0] );
    // unsigned guess = idx / ( max / n );
    unsigned guess = n * idx / max;
    // unsigned guess = idx >> ( level * 5 );
    //printf( "   %u %u   %u %u   ",
    //        max / n, 1 << ( level * 5 ), guess, idx >> ( level * 5 ) );
    // unsigned prev_range;
    while( 1 )
    {
        if( idx >= SUBR( t, tag, guess ) )
        {
            printf( "\n\nIDX TOO HIGH %u\n\n", guess );
            ++guess;
        }
        else
        {
            if( guess > 0 && idx < SUBR( t, tag, guess - 1 ) )
            {
                printf( "\n\nIDX TOO LOW %u\n\n", guess );
                --guess;
            }
            else
            {
                break;
            }
        }
    }
    unsigned prev_range = guess < 1 ? 0 : SUBR( t, tag, guess - 1 );
    unsigned next_idx = idx - prev_range;
#else
    unsigned guess = 0;
    unsigned next_idx = 0;
#endif
    if( tag )
    {
        *p = t->leaves[ guess ]._->data[ next_idx ];
        return 0;
    }
    else
    {
        return seq_get( t->nodes[ guess ]._, next_idx, level - 1, p,
                        t->nodes[ guess ].child_max, t->nodes[ guess ].child_n,
                        t->nodes[ guess ].tag );
    }
}

struct timeval time_diff( struct timeval start, struct timeval end )
{
    struct timeval result;
    result.tv_sec = end.tv_sec - start.tv_sec;
    if( start.tv_usec > end.tv_usec )
    {
        --result.tv_sec;
        end.tv_usec += 1000000;
    }
    result.tv_usec = end.tv_usec - start.tv_usec;
    return result;
}

int print_time( FILE *f, struct timeval tv )
{
    return fprintf( f, "%02ldm %02lds %03dms %03dus\n",
        tv.tv_sec / 60, tv.tv_sec % 60, tv.tv_usec / 1000, tv.tv_usec % 1000 );
}

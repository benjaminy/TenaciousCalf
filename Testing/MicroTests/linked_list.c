#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

#define DOUBLY

struct timeval time_diff( struct timeval start, struct timeval end );
int print_time( FILE *f, struct timeval tv );

typedef uint32_t item_t;

typedef struct ll_t ll_t, *ll_p;
struct ll_t
{
    item_t item;
    ll_p next;
#ifdef DOUBLY
    ll_p prev;
#endif
};

#define N ( 1000 * 1000 * 1000 )
#define AGAIN 5

static int ll_cons( item_t item, ll_p list, ll_p *res );

#define TIMED_STMT(name, S) \
do { \
    struct timeval start, end, duration; \
    /* printf( "%s START\n", name ); */ \
    assert( !gettimeofday( &start, NULL ) ); \
    S; \
    assert( !gettimeofday( &end, NULL ) ); \
    duration = time_diff( start, end ); \
    printf( "%28s ", name ); \
    assert( 0 < print_time( stdout, duration ) ); \
} while( 0 )

void __attribute__ ((noinline)) dense( void )
{
    item_t x = 43, y = 0;
    item_t *a;

    TIMED_STMT( "Dense array alloc", a = (item_t *)malloc( N * sizeof( a[0] ) ) );
    TIMED_STMT( "Dense array init", 
        for( int i = N - 1; i >= 0; --i )
        {
            a[ i ] = x;
            x = ( x * 17 ) + 59;
        } );

    TIMED_STMT( "Dense array reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( unsigned i = 0; i < N; ++i )
            {
                y += a[ i ];
            }
        } );
    printf( "Magic number: %u\n", y );
    free( a );
}

void __attribute__ ((noinline)) padded( void )
{
    int x = 43, y = 0;
    item_t *pa2;
    TIMED_STMT( "Padded array alloc", pa2 = (item_t *)malloc( 2 * N * sizeof( pa2[0] ) ) );
    TIMED_STMT( "Padded array init",
        for( int i = N - 1; i >= 0; --i )
        {
            pa2[ 2 * i ] = x;
            x = ( x * 17 ) + 59;
        } );
    TIMED_STMT( "Padded array reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( unsigned i = 0; i < N; ++i )
            {
                y += pa2[ 2 * i ];
            }
        } );
    printf( "Magic number: %u\n", y );
    free( pa2 );
}

void __attribute__ ((noinline)) padded3( void )
{
    int x = 43, y = 0;
    item_t *pa3;
    TIMED_STMT( "Padded array 3 alloc", pa3 = (item_t *)malloc( 3 * N * sizeof( pa3[0] ) ) );
    TIMED_STMT( "Padded array 3 init",
        for( int i = N - 1; i >= 0; --i )
        {
            pa3[ 3 * i ] = x;
            x = ( x * 17 ) + 59;
        } );
    TIMED_STMT( "Padded array reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( unsigned i = 0; i < N; ++i )
            {
                y += pa3[ 3 * i ];
            }
        } );
    printf( "Magic number: %u\n", y );
    free( pa3 );
}

void __attribute__ ((noinline)) pad_struct( void )
{
    int x = 43, y = 0;
    ll_p sa;
    TIMED_STMT( "Struct array alloc", sa = (ll_p)malloc( N * sizeof( sa[0] ) ) );
    TIMED_STMT( "Struct array init",
        for( int i = N - 1; i >= 0; --i )
        {
            sa[ i ].item = x;
            x = ( x * 17 ) + 59;
        } );
    TIMED_STMT( "Struct array reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( unsigned i = 0; i < N; ++i )
            {
                y += sa[ i ].item;
            }
        } );
    printf( "Magic number: %u\n", y );
    free( sa );
}

void __attribute__ ((noinline)) self_ptr( void )
{
    int x = 43, y = 0;
    ll_p spa;
    TIMED_STMT( "Self ptr array alloc", spa = (ll_p)malloc( N * sizeof( spa[0] ) ) );
    TIMED_STMT( "Self ptr array init",
       for( int i = N - 1; i >= 0; --i )
       {
           spa[ i ].next = (ll_p)&spa[ i ].item;
           spa[ i ].item = x;
           x = ( x * 17 ) + 59;
       } );
    TIMED_STMT( "Self ptr array reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( unsigned i = 0; i < N; ++i )
            {
                y += ((item_t *)spa[ i ].next)[ 0 ];
            }
        } );
    printf( "Magic number: %u\n", y );
    free( spa );
}

void __attribute__ ((noinline)) malloc_ptr( void )
{
    int x = 43, y = 0;
    item_t **mpa;
    TIMED_STMT( "Malloc ptr array alloc", mpa = (item_t **)malloc( N * sizeof( mpa[0] ) ) );
    TIMED_STMT( "Malloc Ptr array init",
        for( int i = N - 1; i >= 0; --i )
        {
            mpa[ i ] = (item_t *)malloc( sizeof( mpa[0][0] ) );
            mpa[ i ][ 0 ] = x;
            x = ( x * 17 ) + 59;
        } );
    TIMED_STMT( "Ptr array reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( unsigned i = 0; i < N; ++i )
            {
                y += mpa[ i ][0];
            }
        } );
    TIMED_STMT( "Malloc Ptr array dealloc",
        for( int i = N - 1; i >= 0; --i )
        {
            free( mpa[ i ] );
        } );
    printf( "Magic number: %u\n", y );
}

void __attribute__ ((noinline)) linked( void )
{
    int x = 43, y = 0;
    ll_p list = NULL;
    TIMED_STMT( "List alloc and init",
        for( unsigned i = N; i > 0; --i )
        {
            ll_p temp;
            ll_cons( x, list, &temp );
            list = temp;
            x = ( x * 17 ) + 59;
        } );
    TIMED_STMT( "List reads",
        for( int j = 0; j < AGAIN; ++j )
        {
            for( ll_p l = list; !!l; l = l->next )
            {
                y += l->item;
            }
        } );
    printf( "Magic number: %u\n", y );
}

int main( int argc, char **argv )
{
    sleep( 1 );
    printf( "DONE SLEEPING\n" );
    dense();
    padded();
    padded3();
    pad_struct();
    self_ptr();
    malloc_ptr();
    linked();
    return 0;
}

static int ll_cons( item_t item, ll_p list, ll_p *res )
{
    *res = (ll_p)malloc( sizeof( res[0][0] ) );
    if( *res )
    {
        (*res)->item = item;
        (*res)->next = list;
#ifdef DOUBLY
        if( list )
        {
            list->prev = *res;
        }
#endif
        return 0;
    }
    else
    {
        return ENOMEM;
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

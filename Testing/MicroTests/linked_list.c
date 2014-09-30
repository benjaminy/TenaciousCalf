#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

// #define DOUBLY

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

#define N ( 10 * 1000 * 1000 )
#define AGAIN 50

static int ll_cons( item_t item, ll_p list, ll_p *res );

int main( int argc, char **argv )
{
    struct timeval start, end, duration;
    item_t x = 43;

    printf( "Array alloc %lu\n", sizeof( ll_t ) );
    assert( !gettimeofday( &start, NULL ) );
    item_t *a = (item_t *)malloc( N * sizeof( a[0] ) );
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    printf( "Array init\n" );
    assert( !gettimeofday( &start, NULL ) );
    for( int i = N - 1; i >= 0; --i )
    {
        a[ i ] = x;
        x = ( x * 17 ) + 59;
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    item_t y = 0;
    printf( "Array reads\n" );
    assert( !gettimeofday( &start, NULL ) );
    for( int j = 0; j < AGAIN; ++j )
    {
        for( unsigned i = 0; i < N; ++i )
        {
            y += a[ i ];
        }
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );
    printf( "Magic number: %u\n", y );
    free( a );

    x = 43;
    y = 0;
    ll_p list = NULL;
    printf( "List alloc and init\n" );
    assert( !gettimeofday( &start, NULL ) );
    for( unsigned i = N; i > 0; --i )
    {
        ll_p temp;
        ll_cons( x, list, &temp );
        list = temp;
        x = ( x * 17 ) + 59;
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );

    y = 0;
    printf( "List reads\n" );
    assert( !gettimeofday( &start, NULL ) );
    for( int j = 0; j < AGAIN; ++j )
    {
        for( ll_p l = list; !!l; l = l->next )
        {
            y += l->item;
        }
    }
    assert( !gettimeofday( &end, NULL ) );
    duration = time_diff( start, end );
    assert( 0 < print_time( stdout, duration ) );
    printf( "Magic number: %u\n", y );
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

/*
 * Refinements to consider:
 * - The stuct hack can be used in several places.
 *   - pro: one fewer pointer deref to get to prefix & suffix.
 *   - cons: Makes the spine management code more complex.  Might
 *     actually hurt locality overall.
 * - Make the arrays circular buffers.
 *   - pro: Might substantially improve performance of transient
 *     updates.
 *   - cons: More complicated.  Hurts memory efficiency.
 * - Put meta information in parent nodes instead of "beside" large
 *   arrays.
 *   - pro: Might improve cache use
 *   - cons: More complicated, especially for transient updates.
 */


#define CHUNK_FACTOR 32;
typedef int value_t;

typedef struct leaf_t leaf_t, *leaf_p;
{
    int length, refcount;
    value_t *values;
}

typedef struct branch_t branch_t, *branch_p;
struct branch_t
{
    int degree, height, refcount;
    size_t size;
    union
    {
        branch_p branches;
        leaf_p leaves;
    } _;
};

typedef struct vertebra_t vertebra_t, *vertebra_p;
struct vertebra_t
{
    int height, refcount;
    size_t size;
    branch_p prefix, suffix;
};

typedef struct finger_tree_root_t finger_tree_root_t, *finger_tree_root_p;
struct finger_tree_root_t
{
    size_t size;
    int spine_length, refcount;
    leaf_t prefix, suffix;
    vertebra_p spine;
};

int lgmt_finger_tree_constr( finger_tree_root_p r )
{
    r->prefix       = NULL;
    r->suffix       = NULL;
    r->spine        = NULL;
    r->size         = 0;
    r->spine_length = 0;
    r->refcount     = 1;
    return 0;
}

int lgmt_finger_tree_prepend(
    finger_tree_root_p r_source, value_t v, finger_tree_root_p r )
{
    if( r != r_source )
        *r = *r_source;
    r->refcount = 1;
    if( r->prefix )
    {
        assert( r->prefix->length > 0 );
        assert( r->prefix->length <= CHUNK_FACTOR );
        if( r->prefix->length == CHUNK_FACTOR )
        {
        }
        else
        {
            
        }
    }
    else
    {
        r->prefix.length = 1;
        r->prefix.refcount = 1;
        r->prefix.values = (value_t *)malloc( sizeof(r->prefix.values[0]) );
        r->prefix.values[0] = v;
    }
    return 0;
}

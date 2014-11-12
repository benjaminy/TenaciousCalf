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
 * - Make a more compact representation for short sequences.
 *   - con: Might make the size of the basic structure dynamic
 */


#define CHUNK_FACTOR 32;
typedef int value_t;

typedef struct leaf_t leaf_t, *leaf_p;
{
    int refcount, length;
    value_t *values;
}

typedef struct branch_t branch_t, *branch_p;
struct branch_t
{
    size_t size;
    int refcount, degree, height;
    union
    {
        branch_p branches;
        leaf_p leaves;
    } _;
};

typedef struct vertebra_t vertebra_t, *vertebra_p;
struct vertebra_t
{
    size_t size;
    int refcount, height;
    branch_p prefix, suffix;
};

typedef struct finger_tree_root_t finger_tree_root_t, *finger_tree_root_p;
struct finger_tree_root_t
{
    size_t size;
    int refcount, spine_length;
    vertebra_p spine;
    leaf_t prefix, suffix;
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

int lgmt_prepend2()
{
}

int lgmt_finger_tree_prepend(
    finger_tree_root_p r_pre, value_t v, finger_tree_root_p r )
{
    /*  */
    if( r != r_pre )
        *r = *r_pre;
    r->refcount = 1;
    assert( r->prefix->length <= CHUNK_FACTOR );
    if( r->prefix->length == CHUNK_FACTOR )
    {
        if( r->spine_length == 0 )
        {
        }
        else
        {
            return lgmt_prepend2();
        }
    }
    else
    {
        int new_len = r->prefix.length + 1;
        r->prefix.refcount = 1; /* XXX? */
        value_t *tmp = (value_t *)malloc( new_len * sizeof(tmp[0]) );
        memcpy( &tmp[1], r->prefix.values, r->prefix.length * sizeof(tmp[0]) );
        tmp[0] = v;
        free( r->prefix.values ); /* XXX refcount? */
        r->prefix.values = tmp;
        r->prefix.length = new_len;
    }
    return 0;
}

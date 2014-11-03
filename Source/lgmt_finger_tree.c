
#define CHUNK_FACTOR 32;
typedef int key_t;

typedef key_t *leaf_p;

typedef struct branch_meta_t branch_meta_t, *branch_meta_p;
struct branch_meta_t
{
    int length;
};

typedef struct branch_t branch_t, *branch_p;
struct branch_t
{
};

typedef struct vertebra_t vertebra_t, *vertebra_p;
struct vertebra_t
{
    int prefix_length, suffix_length, prefix_cap, suffix_cap;
    branch_meta_t prefix_meta, suffix_meta;

    /* Consider (configurably) using the struct hack.
     * - pro: one fewer pointer deref to get to prefix & suffix.
     * - cons: Makes the spine management code more complex.  Might
     *   actually hurt locality overall. */
    branch_p prefix, suffix;
};

typedef struct finger_tree_root_t finger_tree_root_t, *finger_tree_root_p;
struct finger_tree_root_t
{
    int prefix_length, suffix_length, prefix_cap, suffix_cap, spine_length;

    /* Consider (configurably) using the struct hack.
     * - pro: one fewer pointer deref to get to prefix, suffix & spine.
     * - con: sizeof is no longer "right" (e.g. cannot make an array of
     *   finger trees). */
    key_t *prefix, *suffix;
    vertebra_p spine;
};

typedef struct cow_trie_t cow_trie_t, *cow_trie_p;
    struct cow_trie_t
{
    int             ref_count;
    uint32_t        child_bitmap, value_bitmap;
    uint32_t        size;
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

const int      CHILD_ARRAY_BUFFER_SIZE = 2;
const int      VALUE_ARRAY_BUFFER_SIZE = 2;
const uint32_t           LOW_BITS_MASK = 0x1f;
const int                 BITS_PER_LVL = 5;
const int                 LVL_CAPACITY = 1 << ( BITS_PER_LVL );

cow_trie_p cow_trie_alloc( int children, int values )
{
    uint32_t size = sizeof( cow_trie_t ) + (children + CHILD_ARRAY_BUFFER_SIZE) * sizeof( cow_trie_p )
                    + (values + VALUE_ARRAY_BUFFER_SIZE) * sizeof(val_key_t);
    cow_trie_p n = (cow_trie_p)malloc(size);
    n->size = size;
    return n;
}

node_or_edge_p node_or_edge_alloc(int tag, int num_preds, int num_succs) {
    node_or_edge_p n = (node_or_edge_p)malloc(sizeof(n[0]));
    return n;
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
        ++copy_children[i]->ref_count;
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
    uint32_t   bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( shift_amt );
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
    else if( bitmask_loc & map->child_bitmap ) {
        int physical_idx = count_one_bits( map->child_bitmap & bitmask_lower );
        return cow_trie_lookup(
            cow_trie_children(map)[physical_idx], key >> BITS_PER_LVL, v );
    }
    else {
        return 1;
    }
}

int move_stuff_around(cow_trie_p map, int child_count, int value_count, uint32_t bitmask_loc,
                     cow_trie_p child, int v_physical_idx, int c_physical_idx) {
    uint32_t val_size = sizeof(val_key_t);
    uint32_t ptr_size = sizeof(cow_trie_p);
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
        printf("e e");
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


int cow_trie_insert(cow_trie_p map, key_t key, val_t val, cow_trie_p *res) {
    int rc = 0;
    int virtual_idx = key & LOW_BITS_MASK;
    uint32_t bitmask_loc = 1 << virtual_idx;
    uint32_t bitmask_lower;
    int shift_amt = LVL_CAPACITY - virtual_idx;
    if (shift_amt != 32) {
        bitmask_lower = ( ~0U ) >> ( shift_amt );
    }
    else {
        bitmask_lower = 0;
    }
    int child_count = count_one_bits(map->child_bitmap);
    int value_count = count_one_bits(map->value_bitmap);
    
    if (bitmask_loc & map->child_bitmap) {
        int physical_idx = count_one_bits(map->child_bitmap & bitmask_lower);
        cow_trie_p *n = &cow_trie_children(map)[physical_idx];
        if ((*n)->ref_count > 1) {
            cow_trie_p child_copy = cow_trie_copy_node(*n);
            --(*n)->ref_count;
            return cow_trie_insert(child_copy, key >> BITS_PER_LVL, val, &child_copy);
        }
        else {
            return cow_trie_insert(*n, key >> BITS_PER_LVL, val, n);
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
        int c_physical_idx      = count_one_bits(map->child_bitmap & bitmask_lower);
        //if there's no room for a child, minus a value
        if (map->size < sizeof(cow_trie_t) + 
            (child_count + 1) * sizeof(cow_trie_p) + 
            (value_count-1) * sizeof(val_key_t)) { 
            cow_trie_p n            = cow_trie_alloc(child_count+1, value_count);
            n->value_bitmap         = map->value_bitmap ^ bitmask_loc;
            n->child_bitmap         = map->child_bitmap | bitmask_loc;
            cow_trie_p* new_children  = cow_trie_children(n);
            memcpy(cow_trie_values(n), cow_trie_values(map), value_count * sizeof(val_key_t));
            memcpy(new_children, cow_trie_children(map), c_physical_idx * sizeof(cow_trie_p));
            memcpy(&new_children[c_physical_idx + 1],
                   &cow_trie_children(map)[c_physical_idx],
                   (child_count - c_physical_idx) * sizeof(cow_trie_p));
            new_children[c_physical_idx] = child;
            rc = cow_trie_insert(child, key >> BITS_PER_LVL, val, &child);
            *res = n;
        }
        else {
            printf("poopee");
            move_stuff_around(map, child_count, value_count-1, bitmask_loc, child, physical_idx, c_physical_idx);
            map->value_bitmap = map->value_bitmap ^ bitmask_loc;
            rc = cow_trie_insert(child, key >> BITS_PER_LVL, val, &child);
            *res = map;
        }
        return 1;
    }
    else {
        int physical_idx = count_one_bits(map->value_bitmap & bitmask_lower);
        //if there's no room for a value
        if (map->size < sizeof(cow_trie_t) + 
            ((child_count) * sizeof(cow_trie_p)) + 
            ((value_count + 1) * sizeof(val_key_t))) {
            cow_trie_p n = cow_trie_alloc(child_count, value_count+1);
            n->value_bitmap = map->value_bitmap | bitmask_loc;
            n->child_bitmap = map->child_bitmap;
            val_key_p new_vals = cow_trie_values(n);
            val_key_p old_vals = cow_trie_values(map);
            memcpy(cow_trie_children(n), cow_trie_children(map), child_count * sizeof(cow_trie_p));
            memcpy(new_vals, old_vals, physical_idx * sizeof(val_key_t));
            memcpy(&new_vals[physical_idx + 1],
                   &old_vals[physical_idx],
                   (value_count - physical_idx) * sizeof(val_key_t));
            new_vals[physical_idx].key_frag = key;
            new_vals[physical_idx].val = val;
            *res = n;
        }
        else {
            map->value_bitmap = map->value_bitmap | bitmask_loc;
            val_key_p vals = cow_trie_values(map);
            memmove(&vals[physical_idx+1], &vals[physical_idx], (value_count - physical_idx) * sizeof(val_key_t));
            vals[physical_idx].key_frag = key;
            vals[physical_idx].val = val;
        }
        return 1;
        
    }
    return 0;
}

/*int main(int argc, char **argv) {
    cow_trie_p test = cow_trie_alloc(3,0);
    test->child_bitmap = 0;
    test->value_bitmap = 0;
    node_or_edge_p n1 = node_or_edge_alloc(1, 0, 0);
    n1->tag=1;
    n1->_.edge.label = 50;
    n1->_.edge.pred = 90;
    n1->_.edge.succ = 91;
    node_or_edge_p n2 = node_or_edge_alloc(1, 0, 0);
    n2->tag=1;
    n2->_.edge.label = 51;
    n2->_.edge.pred = 92;
    n2->_.edge.succ = 93;
    node_or_edge_p n3 = node_or_edge_alloc(1, 0, 0);
    n3->tag=1;
    n3->_.edge.label = 53;
    n3->_.edge.pred = 94;
    n3->_.edge.succ = 95;
    node_or_edge_p n4 = node_or_edge_alloc(1, 0, 0);
    n4->tag=1;
    n4->_.edge.label = 54;
    n4->_.edge.pred = 96;
    n4->_.edge.succ = 97;
    node_or_edge_p n5 = node_or_edge_alloc(1, 0, 0);
    n5->tag=1;
    n5->_.edge.label = 55;
    n5->_.edge.pred = 98;
    n5->_.edge.succ = 99;
    node_or_edge_p n6 = node_or_edge_alloc(1, 0, 0);
    n6->tag=1;
    n6->_.edge.label = 56;
    n6->_.edge.pred = 100;
    n6->_.edge.succ = 101;
    node_or_edge_p n7 = node_or_edge_alloc(1, 0, 0);
    n7->tag=1;
    n7->_.edge.label = 57;
    n7->_.edge.pred = 102;
    n7->_.edge.succ = 103;
    node_or_edge_p n8 = node_or_edge_alloc(1, 0, 0);
    n8->tag=1;
    n8->_.edge.label = 58;
    n8->_.edge.pred = 104;
    n8->_.edge.succ = 105;
    node_or_edge_p n9 = node_or_edge_alloc(1, 0, 0);
    n9->tag=1;
    n9->_.edge.label = 59;
    n9->_.edge.pred = 106;
    n9->_.edge.succ = 107;
    node_or_edge_p n10 = node_or_edge_alloc(1, 0, 0);
    n10->tag=1;
    n10->_.edge.label = 60;
    n10->_.edge.pred = 108;
    n10->_.edge.succ = 109;
    cow_trie_insert(test, 1, *n1, &test);
    cow_trie_insert(test, 33, *n2, &test);
    cow_trie_insert(test, 3, *n3, &test);
    cow_trie_insert(test, 5, *n4, &test);
    cow_trie_insert(test, 35, *n5, &test);
    cow_trie_insert(test, 37, *n6, &test);
    cow_trie_insert(test, 0, *n7, &test);
    cow_trie_insert(test, 2, *n8, &test);
    cow_trie_insert(test, 4, *n9, &test);
    cow_trie_insert(test, 32, *n10, &test);
    val_t q;
    cow_trie_lookup(test, 35, &q);
}*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "cow_trie.c"

uint32_t random_int(int min, uint32_t max) {
    return rand() % (max + 1 - min) + min;
}

void shuffle(uint32_t *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          uint32_t t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

int is_in(uint32_t *array, int size, uint32_t val) {
    int result = 0;
    for (int i=0;i<size;i++) {
        if (array[i] == val) {
            result = 1;
        }
    }
    return result;
}

uint32_t *random_unique_array(int num_keys, uint32_t max) {
    int M = num_keys;
    uint32_t N = max;
    uint32_t *array = (uint32_t *)malloc(sizeof(uint32_t)*num_keys);
    for (int i=0;i<num_keys;i++) {
        array[i] = 0;
    }
    for (int i=0;i<num_keys;i++) {
        uint32_t cand = rand();
        while (is_in(array, num_keys, cand)) {
            cand = rand();
        }
        array[i] = cand;
    }
    shuffle(array, num_keys);
    return array;

}


node_or_edge_p make_values(uint32_t *key_array, int num_values) {
    node_or_edge_p val_array = (node_or_edge_p)malloc(num_values * sizeof(val_array[0]));
    for (int i=0; i<num_values; i++) {
        //generate tag
        int tag = random_int(0, 1);
        val_array[i].tag = tag;
        if (tag == 0) {
            //generate a node
            val_array[i]._.node.label = random_int(0, UINT32_MAX-1);
            val_array[i]._.node.pred_ct = (size_t)random_int(0, 50);
            val_array[i]._.node.succ_ct = (size_t)random_int(0, 50);
            val_array[i]._.node.preds = random_unique_array(val_array[i]._.node.pred_ct, UINT32_MAX-1);
            val_array[i]._.node.succs = random_unique_array(val_array[i]._.node.succ_ct, UINT32_MAX-1);
        }
        else if (tag == 1) {
            //generate an edge
            val_array[i]._.edge.label = random_int(0, UINT32_MAX-1);
            val_array[i]._.edge.pred = random_int(0, UINT32_MAX-1);
            val_array[i]._.edge.succ = random_int(0, UINT32_MAX-1);
        }
    }
    return val_array;
}

int check_result(cow_trie_p map, uint32_t *keys, node_or_edge_p vals, int result, int size) {
    for (int i=0;i<size;i++) {
        val_t u;
        cow_trie_lookup(map, keys[i], &u);
        printf("key: %d\n", keys[i]);
        if (u.tag == 0) {
            printf("node label from lookup == %d\n ", u._.node.label);
            printf("node label from list == %d\n\n", vals[i]._.node.label);
        }
        else if (u.tag == 1) {
            printf("edge label from lookup == %d\n", u._.edge.label);
            printf("edge label from list == %d\n\n", vals[i]._.edge.label);
        }
        else {
            printf("got a tag that's not 0 or 1 (you made an oops)\n\n");
        }
    }
    return result;
}

int main() {
    int size = 50000;
    uint32_t *keys = random_unique_array(size, UINT32_MAX-1);
    node_or_edge_p vals = make_values(keys, size);
    cow_trie_p test = cow_trie_alloc(0,0);
    test->child_bitmap = 0;
    test->value_bitmap = 0;
    for (int i=0;i<size;i++) {
        printf("key: %d\n", keys[i]);
    }
    printf("\n");
    for (int i=0;i<size;i++) {
        if (vals[i].tag == 0) {
            printf("val: %d\n", vals[i]._.node.label);
        }
        if (vals[i].tag == 1) {
            printf("val: %d\n", vals[i]._.edge.label);
        }
    }
    printf("\n");
    for (int i=0;i<size;i++) {
        if (i == 16) {
            printf("barry white green beans");
        }
        cow_trie_insert(test, keys[i], vals[i], &test);
        printf("inserted %d\n\n\n", i);
    }
    printf("\n");
    int p = 0;
    check_result(test, keys, vals, p, size);
    fflush(stdout);
}

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "cow_trie.c"

uint32_t random_int(int min, uint32_t max) {
    srand(time(NULL));
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


val_key_p make_values(uint32_t *key_array, int num_values) {
    val_key_p val_array = (val_key_p)malloc(num_values * sizeof(val_array[0]));
    for (int i=0; i<num_values; i++) {
        srand(time(NULL));
        val_array[i].key_frag = key_array[i];
        //generate tag
        int tag = random_int(0, 1);
        val_array[i].val.tag = tag;
        if (tag == 0) {
            //generate a node
            val_array[i].val._.node.label = random_int(0, UINT32_MAX-1);
            val_array[i].val._.node.pred_ct = (size_t)random_int(0, 50);
            val_array[i].val._.node.succ_ct = (size_t)random_int(0, 50);
            val_array[i].val._.node.preds = random_unique_array(val_array[i].val._.node.pred_ct, UINT32_MAX-1);
            val_array[i].val._.node.succs = random_unique_array(val_array[i].val._.node.succ_ct, UINT32_MAX-1);
        }
        else if (tag == 1) {
            //generate an edge
            val_array[i].val._.edge.label = random_int(0, UINT32_MAX-1);
            val_array[i].val._.edge.pred = random_int(0, UINT32_MAX-1);
            val_array[i].val._.edge.succ = random_int(0, UINT32_MAX-1);
        }
    }
    return val_array;
}

int check_result(cow_trie_p map, uint32_t *keys, val_key_p vals, int result, int size) {
    for (int i=0;i<size;i++) {
        val_t u;
        cow_trie_lookup(map, keys[i], &u);
        if (u.tag == 0) {
            printf("node label == %d", u._.node.label);
            printf("node label from list == %d\n\n", vals[i].val._.node.label);
        }
        else if (u.tag == 1) {
            printf("edge label == %d", u._.edge.label);
            printf("edge label from list == %d\n\n", vals[i].val._.edge.label);
        }
    }
    return result;
}

int main() {
    uint32_t *keys = random_unique_array(2, UINT32_MAX-1);
    val_key_p vals = make_values(keys, 2);
    cow_trie_p test = cow_trie_alloc(0,0);
    test->child_bitmap = 0;
    test->value_bitmap = 0;
    for (int i=0;i<2;i++) {
        cow_trie_insert(test, keys[i], vals[i].val, &test);
    }
    int result = 0;
    check_result(test, keys, vals, result, 2);
}

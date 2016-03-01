#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "tester.h"

uint32_t random_int(int r_min, uint32_t r_max) {
    return (rand() % (r_max + 1 - r_min)) + r_min;
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

void seed() {
    srand(0);
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


/*node_or_edge_p make_values(uint32_t *key_array, int num_values) {
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
}*/

/*int check_result(cow_trie_p map, uint32_t *keys, node_or_edge_p vals, int result, int size) {
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
    printf("check done\n\n");
    return result;
}*/

/*int main() {
    int size = 200;
    int copy_freq = 5;
    uint32_t *keys = random_unique_array(size, UINT32_MAX-1);
    node_or_edge_p vals = make_values(keys, size);
    cow_trie_p test = cow_trie_alloc(0,0);
    cow_trie_p *results = (cow_trie_p *)malloc(sizeof(cow_trie_p *) * size/5);
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
    results[0] = test;
    printf("beginning insert\n");
    fflush(stdout);
    for (int i=0;i<size/copy_freq;i++) {
        for (int j=0;j<copy_freq;j++) {
            cow_trie_insert(results[i], keys[copy_freq * i + j], vals[copy_freq * i + j], &results[i]);
        }
        cow_trie_p new_node = cow_trie_copy_node(results[i]);
        results[i+1] = new_node;
        int p=0;
        printf("checking results on original root:\n\n");
        check_result(results[0], keys, vals, p, copy_freq);
        for (int k=0;k<i;k++) {
            printf("checking results on %dth/rd/st copy of root:\n\n", k+1);
            check_result(results[k+1], keys, vals, p, (k+2)*copy_freq);
        }
    }
    printf("insert done");
    fflush(stdout);
    int p = 0;
}*/

/*int main() {
    int size = 16;
    uint32_t *keys = random_unique_array(size, UINT32_MAX-1);
    node_or_edge_p vals = make_values(keys, size);
    cow_trie_p test = cow_trie_alloc(0,0);
    test->ref_count = 1;
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
        printf("inserted %d\n\n", i);
    }
    printf("\n");
    int p = 0;
    check_result(test, keys, vals, p, size);
    fflush(stdout);
    free(vals);
    free(keys);
}

int main(int argc, char **argv) {
    cow_trie_p test = cow_trie_alloc(0,0);
    test->ref_count = 1;
    test->child_bitmap = 0;
    test->value_bitmap = 0;
    node_or_edge_p n1 = node_or_edge_alloc(1, 0, 0);
    n1->tag=0;
    n1->_.node.label = 50;
    n1->_.node.pred_ct = 10;
    n1->_.node.succ_ct = 7;
    n1->_.node.preds = (uint32_t *)malloc(sizeof(uint32_t)*10);
    n1->_.node.succs = (uint32_t *)malloc(sizeof(uint32_t)*7);
    for (int i=0;i<10;i++) {
        n1->_.node.preds[i] = 749287;
    }
    for (int i=0;i<7;i++) {
        n1->_.node.succs[i] = 432789;
    }
    node_or_edge_p n2 = node_or_edge_alloc(1, 0, 0);
    n2->tag=0;
    n2->_.node.label = 51;
    n2->_.node.pred_ct = 10;
    n2->_.node.succ_ct = 7;
    n2->_.node.preds = (uint32_t *)malloc(sizeof(uint32_t)*10);
    n2->_.node.succs = (uint32_t *)malloc(sizeof(uint32_t)*7);
    for (int i=0;i<10;i++) {
        n2->_.node.preds[i] = 749287;
    }
    for (int i=0;i<7;i++) {
        n2->_.node.succs[i] = 432789;
    }
    node_or_edge_p n3 = node_or_edge_alloc(1, 0, 0);
    n3->tag=0;
    n3->_.node.label = 53;
    n3->_.node.pred_ct = 10;
    n3->_.node.succ_ct = 7;
    n3->_.node.preds = (uint32_t *)malloc(sizeof(uint32_t)*10);
    n3->_.node.succs = (uint32_t *)malloc(sizeof(uint32_t)*7);
    for (int i=0;i<10;i++) {
        n3->_.node.preds[i] = 749287;
    }
    for (int i=0;i<7;i++) {
        n3->_.node.succs[i] = 432789;
    }
    cow_trie_insert(test, 1, *n1, &test);
    free(n1);
    cow_trie_insert(test, 2, *n2, &test);
    free(n2);
    cow_trie_insert(test, 3, *n3, &test);
    free(n3);
    val_t q;
    cow_trie_lookup(test, 3, &q);
    if (q.tag == 0) {
        printf("node label from lookup == %d\n ", q._.node.label);
    }
    else if (q.tag == 1) {
        printf("edge label from lookup == %d\n", q._.edge.label);
    }
}*/

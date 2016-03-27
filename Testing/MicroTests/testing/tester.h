#ifndef TESTER_H_
#define TESTER_H_

uint32_t random_int(int min, uint32_t max);

void shuffle(uint32_t *array, size_t n);

int is_in(uint32_t *array, int size, uint32_t val);

uint32_t *random_unique_array(int num_keys, uint32_t max);

void seed();

#endif


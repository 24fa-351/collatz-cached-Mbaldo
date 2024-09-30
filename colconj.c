#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>



typedef struct {
    int number;
    int steps;
} CacheEntry;

typedef struct {
    CacheEntry *entries;
    int *access_order; // For LRU
    int size;
    int hits;
    int misses;
    int current_size;
    int policy;
} Cache;

int collatz_steps(long long int n) {
    int steps = 0;
    while (n != 1) {
        if (n % 2 == 0)
            n /= 2;
        else {
            n = 3 * n + 1;
        }
        steps++;
        if (steps > 1000000) {
            fprintf(stderr, "Error: Exceeded maximum number of steps for number %lld\n", n);
            exit(1);
        }
    }
    return steps;
}

int lookup_cache(Cache *cache, int number) {
    for (int i = 0; i < cache->current_size; i++) {
        if (cache->entries[i].number == number) {
            if (cache->policy == CACHE_LRU) {
                for (int j = 0; j < cache->current_size; j++) {
                    if (cache->access_order[j] == i) {
                        for (int k = j; k < cache->current_size - 1; k++) {
                            cache->access_order[k] = cache->access_order[k + 1];
                        }
                        cache->access_order[cache->current_size - 1] = i;
                        break;
                    }
                }
            }
            cache->hits++;
            return cache->entries[i].steps;
        }
    }
    cache->misses++;
    return -1;
}

void add_to_cache(Cache *cache, int number, int steps) {
    if (cache->current_size < cache->size) {
        cache->entries[cache->current_size].number = number;
        cache->entries[cache->current_size].steps = steps;
        if (cache->policy == CACHE_LRU) {
            cache->access_order[cache->current_size] = cache->current_size;
        }
        cache->current_size++;
    } else {
        int replace_index = 0;
        if (cache->policy == CACHE_LRU) {
            replace_index = cache->access_order[0];
            for (int i = 0; i < cache->current_size - 1; i++) {
                cache->access_order[i] = cache->access_order[i + 1];
            }
            cache->access_order[cache->current_size - 1] = replace_index;
        } else if (cache->policy == CACHE_RR) {
            replace_index = rand() % cache->size;
        }
        cache->entries[replace_index].number = number;
        cache->entries[replace_index].steps = steps;
    }
}

Cache* init_cache(int size, int policy) {
    if (size <= 0) {
        fprintf(stderr, "Cache size must be greater than 0.\n");
        exit(1);
    }

    Cache *cache = (Cache*)malloc(sizeof(Cache));
    if (cache == NULL) {
        fprintf(stderr, "Failed to allocate memory for cache.\n");
        exit(1);
    }

    cache->entries = (CacheEntry*)malloc(size * sizeof(CacheEntry));
    if (cache->entries == NULL) {
        fprintf(stderr, "Failed to allocate memory for cache entries.\n");
        exit(1);
    }

    if (policy == CACHE_LRU) {
        cache->access_order = (int*)malloc(size * sizeof(int));
        if (cache->access_order == NULL) {
            fprintf(stderr, "Failed to allocate memory for LRU access order.\n");
            exit(1);
        }
    }

    cache->size = size;
    cache->hits = 0;
    cache->misses = 0;
    cache->current_size = 0;
    cache->policy = policy;

    return cache;
}

void free_cache(Cache *cache) {
    if (cache == NULL) return;
    
    free(cache->entries);
    if (cache->policy == CACHE_LRU) {
        free(cache->access_order);
    }
    free(cache);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s <N> <MIN> <MAX> <cache_policy> <cache_size>\n", argv[0]);
        printf("cache_policy: LRU or RR\n");
        return 1;
    }

    int N = atoi(argv[1]);
    int MIN = atoi(argv[2]);
    int MAX = atoi(argv[3]);
    char *cache_policy_str = argv[4];
    int cache_size = atoi(argv[5]);

    if (N <= 0 || MIN <= 0 || MAX <= 0 || MIN >= MAX) {
        fprintf(stderr, "Invalid arguments: Ensure that N, MIN, MAX are positive and MIN < MAX.\n");
        return 1;
    }

    int cache_policy;
    if (strcmp(cache_policy_str, "LRU") == 0) {
        cache_policy = CACHE_LRU;
    } else if (strcmp(cache_policy_str, "RR") == 0) {
        cache_policy = CACHE_RR;
    } else {
        fprintf(stderr, "Invalid cache policy. Use LRU or RR.\n");
        return 1;
    }

    srand(time(NULL));

    Cache *cache = NULL;
    if (cache_policy != CACHE_NONE) {
        cache = init_cache(cache_size, cache_policy);
    }

    FILE *output = fopen("collatz_results.csv", "w");
    if (output == NULL) {
        fprintf(stderr, "Error opening file for output.\n");
        if (cache) free_cache(cache);
        return 1;
    }

    fprintf(output, "Number,Steps\n");

    printf("Running Collatz on %d numbers between %d and %d...\n", N, MIN, MAX);

    for (int i = 0; i < N; i++) {
        int number = MIN + rand() % (MAX - MIN + 1);
        int steps = -1;

        if (cache_policy != CACHE_NONE) {
            steps = lookup_cache(cache, number);
        }

        if (steps == -1) {  // Cache miss or no cache
            steps = collatz_steps(number);
            if (cache_policy != CACHE_NONE) {
                add_to_cache(cache, number, steps);
            }
        }

        fprintf(output, "%d,%d\n", number, steps);
    }

    fclose(output);

    if (cache_policy != CACHE_NONE) {
        printf("Cache Hits: %d\n", cache->hits);
        printf("Cache Misses: %d\n", cache->misses);
        printf("Cache Hit Rate: %.2f%%\n", (100.0 * cache->hits) / (cache->hits + cache->misses));
        free_cache(cache);
    }

    return 0;
}

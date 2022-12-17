#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "../src/client/client.h"

#define PAIR_LEN     5
#define NUM_THREADS  300
#define NUM_OF_PAIRS 1000

struct thread_input {
	int fd;
	char **keys;
	char **vals;
};

static void rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int keys = rand() % (int) (sizeof charset - 1);
            str[n] = charset[keys];
        }
        str[size] = '\0';
    }
}

void *test_thread(void *fd) 
{
	int t_fd = *(int *)fd;
	dict_pair *recieve;
	char **keys;
	char **vals;
	
	keys = calloc(NUM_OF_PAIRS, sizeof(char *));
	vals = calloc(NUM_OF_PAIRS, sizeof(char *));
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		keys[i] = calloc(PAIR_LEN, sizeof(char));
		vals[i] = calloc(PAIR_LEN, sizeof(char));
		rand_string(keys[i], PAIR_LEN);
		rand_string(vals[i], PAIR_LEN);
	}

	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		assert(set_pair(t_fd, keys[i], sizeof(keys[i]), CHAR, vals[i], sizeof(vals[i]), CHAR) == 0);
		recieve = get_value(t_fd, keys[i], sizeof(keys[i]), CHAR);
		assert(memcmp(recieve->value, vals[i], recieve->value_size) == 0);
		free(recieve->value);
		free(recieve);
		assert(del_pair(t_fd, keys[i], sizeof(keys[i]), CHAR) == 0);
		assert(get_value(t_fd, keys[i], sizeof(keys[i]), CHAR) == NULL);
	}
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		free(keys[i]);
		free(vals[i]);
	}
	
	free(keys);
	free(vals);
	return NULL;
}

int main()
{
	int fd;
	pthread_t thread_id[NUM_THREADS];

	fd = open(DEVICE_PATH, O_RDWR);
	
	for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&thread_id[i], NULL, test_thread, &fd);
    }
	
	for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(thread_id[i], NULL);
    }
    
	printf("Stress test (untyped) passed!\n");
	
	return 0;
}
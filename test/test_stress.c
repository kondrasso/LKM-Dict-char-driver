#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "../src/client/client.h"

#define KEY_LEN      20
#define VAL_LEN      20
#define NUM_OF_PAIRS 100000

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

static int *rand_int_arr(int *int_arr, size_t size)
{
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int_arr[n] = rand();;
        }
    }
    return int_arr;
}

/* char to char thread */
void *test_thread_0(void *fd) 
{
	int fd_0 = (int *)fd;
	dict_pair *recieve;
	
	char **keys_0;
	char **vals_0;
	
	keys_0 = malloc(NUM_OF_PAIRS * sizeof(char *));
	vals_0 = malloc(NUM_OF_PAIRS * sizeof(char *));
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		keys_0[i] = malloc(sizeof(char) * KEY_LEN);
		vals_0[i] = malloc(sizeof(char) * VAL_LEN);
		rand_string(keys_0[i], KEY_LEN);
		rand_string(vals_0[i], VAL_LEN);
	}

	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		assert(set_pair(fd_0, keys_0[i], sizeof(keys_0[i]), CHAR, vals_0[i], sizeof(vals_0[i]), CHAR) == 0);
		recieve = get_value(fd_0, keys_0[i], sizeof(keys_0[i]), CHAR);
		assert(memcmp(recieve->value, vals_0[i], recieve->value_size) == 0);
		free(recieve->value);
		free(recieve);
		assert(del_pair(fd_0, keys_0[i], sizeof(keys_0[i]), CHAR) == 0);
		assert(get_value(fd_0, keys_0[i], sizeof(keys_0[i]), CHAR) == NULL);
	}
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		free(keys_0[i]);
		free(vals_0[i]);
	}
	
	free(keys_0);
	free(vals_0);
	
}

/* int to char thread */
void *test_thread_1(void *fd) 
{
	int fd_1 = (int *)fd;
	dict_pair *recieve;
	
	int **keys_1;
	char **vals_1;
	
	keys_1 = malloc(NUM_OF_PAIRS * sizeof(int *));
	vals_1 = malloc(NUM_OF_PAIRS * sizeof(char *));
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		keys_1[i] = malloc(sizeof(int) * KEY_LEN);
		vals_1[i] = malloc(sizeof(char) * VAL_LEN);
		rand_int_arr(keys_1[i], KEY_LEN);
		rand_string(vals_1[i], VAL_LEN);
	}

	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		assert(set_pair(fd_1, keys_1[i], sizeof(keys_1[i]), INT, vals_1[i], sizeof(vals_1[i]), CHAR) == 0);
		recieve = get_value(fd_1, keys_1[i], sizeof(keys_1[i]), INT);
		assert(memcmp(recieve->value, vals_1[i], recieve->value_size) == 0);
		free(recieve->value);
		free(recieve);
		assert(del_pair(fd_1, keys_1[i], sizeof(keys_1[i]), INT) == 0);
		assert(get_value(fd_1, keys_1[i], sizeof(keys_1[i]), INT) == NULL);
	}
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		free(keys_1[i]);
		free(vals_1[i]);
	}
	
	free(keys_1);
	free(vals_1);
}

/* char to int thread */
void *test_thread_2(void *fd) 
{
	int fd_2 = (int *)fd;
	dict_pair *recieve;
	
	char **keys_2;
	int **vals_2;
	
	keys_2 = malloc(NUM_OF_PAIRS * sizeof(char *));
	vals_2 = malloc(NUM_OF_PAIRS * sizeof(int *));
	
	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		keys_2[i] = malloc(sizeof(char) * KEY_LEN);
		vals_2[i] = malloc(sizeof(int) * VAL_LEN);
		rand_string(keys_2[i], KEY_LEN);
		rand_int_arr(vals_2[i], VAL_LEN);
	}

	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		assert(set_pair(fd_2, keys_2[i], sizeof(keys_2[i]), CHAR, vals_2[i], sizeof(vals_2[i]), INT) == 0);
		recieve = get_value(fd_2, keys_2[i], sizeof(keys_2[i]), CHAR);
		assert(memcmp(recieve->value, vals_2[i], recieve->value_size) == 0);
		free(recieve->value);
		free(recieve);
		assert(del_pair(fd_2, keys_2[i], sizeof(keys_2[i]), CHAR) == 0);
		assert(get_value(fd_2, keys_2[i], sizeof(keys_2[i]), CHAR) == NULL);
	}

	for (int i = 0; i < NUM_OF_PAIRS; i++) {
		free(keys_2[i]);
		free(vals_2[i]);
	}
	
	free(keys_2);
	free(vals_2);
}



int main() {
	int fd;
	fd = open(DEVICE_PATH, O_RDWR);
	pthread_t thread_0, thread_1, thread_2;
	
	pthread_create(&thread_0, NULL, test_thread_0, fd);
	pthread_create(&thread_1, NULL, test_thread_1, fd);
	pthread_create(&thread_2, NULL, test_thread_1, fd);
	
	pthread_join(thread_0, NULL);
	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);
	
	printf("Stress test passed\n");
	return 0;
}
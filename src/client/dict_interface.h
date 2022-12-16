#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>

#define DEVICE_PATH "/dev/dict_device"

#define SET_PAIR _IOWR('a', 'a', dict_pair *)
#define DEL_PAIR _IOWR('a', 'b', dict_pair *)
#define GET_VALUE _IOWR('b', 'b', dict_pair *)
#define GET_VALUE_SIZE _IOR('b', 'c', dict_pair *)
#define GET_VALUE_TYPE _IOR('c', 'c', dict_pair *)

typedef struct dict_pair dict_pair;
typedef struct dict_value_data dict_value_data;

struct dict_pair
{
    unsigned long key_hash;

    int key_type;
    int value_type;

    size_t key_size;
    size_t value_size;    

    void *key;
    void *value;

    dict_pair *next;
};

enum my_data_types {
    MY_INT = 1,
    MY_CHAR = 2
};

int del_pair();
int set_pair();
dict_pair *get_value();
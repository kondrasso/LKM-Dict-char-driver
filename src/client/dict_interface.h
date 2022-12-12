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

#define DEVICE_PATH "/dev/pyld_device"

#define SET_PAIR _IOWR('a', 'a', pyld_pair *)
#define DEL_PAIR _IOWR('a', 'b', pyld_pair *)
#define GET_VALUE _IOWR('b', 'b', pyld_pair *)
#define GET_VALUE_SIZE _IOR('b', 'c', pyld_pair *)
#define GET_VALUE_TYPE _IOR('c', 'c', pyld_pair *)

typedef struct pyld_pair pyld_pair;
typedef struct pyld_value_data pyld_value_data;

struct pyld_pair
{
    unsigned long key_hash;

    int key_type;
    int value_type;

    size_t key_size;
    size_t value_size;    

    void *key;
    void *value;

    pyld_pair *next;
};

enum my_data_types {
    MY_INT = 1,
    MY_CHAR = 2
};

extern int *del_pair();
extern pyld_pair *set_pair();
extern pyld_pair *get_value();
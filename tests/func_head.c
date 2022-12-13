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
#include <unistd.h>
#include "dict_interface.h"


int set_pair(int fd, void *key, size_t key_size, int key_type, void* value, size_t value_size, int value_type)
{
    pyld_pair *message;

    if (key == NULL || value == NULL) {
        printf("SET_PAIR: NULL key and values not allowed\n");
        return -1;
    }

    if (key_size == 0 || value_size == 0) {
        printf("SET_PAIR: illegal size\n");
        return -1;
    }

    message = calloc(1, sizeof(pyld_pair));

    if (message == NULL) {
        printf("SET_PAIR: message calloc failed\n");
        return -1;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;
    message->value              = malloc(value_size);
    memcpy(message->value, value, value_size);
    message->value_size         = value_size;
    message->value_type         = value_type;

    if (ioctl(fd, SET_PAIR, message)) {
        printf("SET_PAIR: error setting pair\n");
        free(message->value);
        free(message);
        return -1;
    }
    free(message->value);
    free(message);

    return 0;
}


pyld_pair *get_value(int fd, void *key, size_t key_size, int key_type)
{
    int value_type;
    long value_size;
    pyld_pair *message;

    if (key == NULL) {
        printf("GET_VALUE: NULL key not allowed\n");
        return NULL;
    }

    if (key_size == 0) {
        printf("GET_VALUE: illegal key size\n");
        return NULL;
    }

    message = calloc(1, sizeof(pyld_pair));

    if (message == NULL) {
        printf("GET_VALUE: message calloc failed\n");
        return NULL;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;

    value_size = ioctl(fd, GET_VALUE_SIZE, message);
    value_type = ioctl(fd, GET_VALUE_TYPE, message);

    if (value_size < 0) {
        printf("GET_VALUE: Could not get size of value\n");
        free(message);
        return NULL;
    }

    if (value_type < 0) {
        printf("GET_VALUE: Could not get type of value\n");
        free(message);
        return NULL;
    }

    message->value_size         = (size_t)value_size;
    message->value              = malloc(message->value_size);

    if (message->value == NULL) {
        printf("GET_VALUE: value malloc failed\n");
        free(message);
        return NULL;
    }

    message->value_type         = value_type;

    if (ioctl(fd, GET_VALUE, message)) {
        printf("GET_VALUE: no such key-value pair\n");
        free(message->value);
        free(message);
        return NULL;
    }

    return message;
}


int del_pair(int fd, void *key, size_t key_size, int key_type)
{
    pyld_pair *message;
    long value_size;

    if (key == NULL) {
        printf("DEL_PAIR: NULL key not allowed\n");
        return -1;
    }

    if (key_size == 0) {
        printf("DEL_PAIR: illegal key size\n");
        return -1;
    }


    message = calloc(1, sizeof(pyld_pair));

    if (message == NULL) {
        printf("DEL_PAIR: calloc failed\n");
        return -1;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;

    if (ioctl(fd, DEL_PAIR, message)) {
        printf("DEL_PAIR: could not delete pair\n");
        free(message);
        return -1;
    }
    free(message);
    return 0;
}

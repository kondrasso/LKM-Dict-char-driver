#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "dict_interface.h"

/** @brief Send IOCTL request to copy from provided data structure and conduct set in driver
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param value  Pointer to value location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 *  @param value_size Size of value, follows sizeof() format with size_t
 *  @param key_type Number that charachterizes key for casting in userspace
 *  @param value_type Number that charachterizes key for casting in userspace
 *  @return 0 on success, -1 if IOCTL returned error or memory fault here
 */
int set_pair(int fd, void *key, size_t key_size, int key_type, void* value, size_t value_size, int value_type)
{
    dict_pair *message;

    if (key == NULL || value == NULL) {
        printf("SET_PAIR: NULL key and values not allowed\n");
        return -1;
    }

    if (key_size == 0 || value_size == 0) {
        printf("SET_PAIR: illegal size\n");
        return -1;
    }

    message = calloc(1, sizeof(dict_pair));

    if (message == NULL) {
        printf("SET_PAIR: message calloc failed\n");
        return -1;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;
    message->value              = malloc(value_size);

    if (message->value == NULL) {
        printf("SET_PAIR: malloc failed\n");
        return -1;
    }
    
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

/** @brief Sends get requests via IOCTL that will copy value to provided structure;
 *  conducts IOCTL get_value_size and get_value type for memory allocation and tracking
 *  properties
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param key_size  size of key, follows sizeof() format with size_t
 *  @return Struct containing value for matching key; NULL if pair does not exist
 */
dict_pair *get_value(int fd, void *key, size_t key_size, int key_type)
{
    int value_type;
    long value_size;
    dict_pair *message;

    if (key == NULL) {
        printf("GET_VALUE: NULL key not allowed\n");
        return NULL;
    }

    if (key_size == 0) {
        printf("GET_VALUE: illegal key size\n");
        return NULL;
    }

    message = calloc(1, sizeof(dict_pair));

    if (message == NULL) {
        printf("GET_VALUE: message calloc failed\n");
        return NULL;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;

    value_size = ioctl(fd, GET_VALUE_SIZE, message);

    if (value_size < 0) {
        printf("GET_VALUE: Could not get size of value\n");
        free(message);
        return NULL;
    }

    value_type = ioctl(fd, GET_VALUE_TYPE, message);

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

/** @brief Send deletion requests via IOCTL
 *  @param pd Pointer to a shared dictionary object
 *  @param key Pointer to key location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 *  @return 0 if ioctl worked without error; else -1
 */
int del_pair(int fd, void *key, size_t key_size, int key_type)
{
    dict_pair *message;
    long value_size;

    if (key == NULL) {
        printf("DEL_PAIR: NULL key not allowed\n");
        return -1;
    }

    if (key_size == 0) {
        printf("DEL_PAIR: illegal key size\n");
        return -1;
    }


    message = calloc(1, sizeof(dict_pair));

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

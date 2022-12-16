#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "client.h"

/** @brief Send IOCTL request to copy from provided data structure and conduct set in driver
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param value  Pointer to value location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 *  @param value_size Size of value, follows sizeof() format with size_t
 *  @param key_type Number that charachterizes key for casting in userspace
 *  @param value_type Number that charachterizes key for casting in userspace
 *  @return 0 on success
 */
int set_pair(int fd, void *key, size_t key_size, int key_type, void* value, size_t value_size, int value_type)
{
    int retval;
    dict_pair *message;
    
    message = calloc(1, sizeof(dict_pair));

    if (message == NULL) {
        printf("SET_PAIR: message calloc failed\n");
        return -ENOMEM;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;
    message->value              = value;
    message->value_size         = value_size;
    message->value_type         = value_type;

    retval = ioctl(fd, SET_PAIR, message);
    
    if (retval != 0) {
        printf("SET_PAIR: %s\n", strerror(retval));
    }
    
    free(message);
    return retval;
}

/** @brief 
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param key_size  size of key, follows sizeof() format with size_t
 *  @return Struct containing value for matching key; NULL if pair does not exist
 */
dict_pair *get_value(int fd, void *key, size_t key_size, int key_type)
{
    int retval;
    long value_type;
    dict_pair *message;

    message = calloc(1, sizeof(dict_pair));

    if (message == NULL) {
        fprintf(stderr, "GET_VALUE: message calloc failed\n");
        return NULL;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;
    message->value_size_adress  = &message->value_size;

    retval = ioctl(fd, GET_VALUE_SIZE, message);

    if (retval != 0) {
        if (retval == NO_PAIR) {
            fprintf(stderr, "GET_VALUE_SIZE: no such pair\n");
        } else {
            fprintf(stderr, "GET_VALUE_SIZE: %s\n", strerror(retval));
        }
        goto get_error;
    }
        
    value_type = ioctl(fd, GET_VALUE_TYPE, message);

    if (value_type < 0) {
        fprintf(stderr, "GET_VALUE_TYPE: %s\n", strerror(value_type));
        goto get_error;
    }
    
    message->value_type         = value_type;
    message->value              = malloc(message->value_size);

    if (message->value == NULL) {
        fprintf(stderr, "GET_VALUE: value malloc failed\n");
        goto get_error;
    }


    retval = ioctl(fd, GET_VALUE, message);
    
    if (retval != 0) {
        fprintf(stderr, "GET_VALUE: %s\n", strerror(retval));
    }
    
    return message;

get_error:
    free(message);
    return NULL;

}

/** @brief Send deletion requests via IOCTL
 *  @param pd Pointer to a shared dictionary object
 *  @param key Pointer to key location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 *  @return 0 if ioctl worked without error; else -1
 */
int del_pair(int fd, void *key, size_t key_size, int key_type)
{
    long retval;
    long value_size;
    dict_pair *message;

    message = calloc(1, sizeof(dict_pair));

    if (message == NULL) {
        fprintf(stderr, "DEL_PAIR: calloc failed\n");
        return -ENOMEM;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;

    retval = ioctl(fd, DEL_PAIR, message);
    
    if (retval != 0) {
        fprintf(stderr, "DEL_PAIR: error %s\n", strerror(retval));
    }
    
    free(message);
    return retval;
}

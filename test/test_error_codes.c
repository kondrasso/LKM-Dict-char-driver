#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "../src/client/client.h"

static char key[] = "key\0";
static char value[] = "value\0";

void test_set_wrong_input(int fd) 
{   
    assert(set_pair(-1, key, sizeof(key), CHAR, value, sizeof(value), CHAR) == -1);
    assert(set_pair(fd, NULL, sizeof(key), CHAR, value, sizeof(value), CHAR) == EINVAL);
    assert(set_pair(fd, key,  0, CHAR, value, sizeof(value), CHAR) == EINVAL);
    assert(set_pair(fd, key, sizeof(key), -1, value, sizeof(value), CHAR) == EINVAL);
    assert(set_pair(fd, key, sizeof(key), INT, NULL, sizeof(value), CHAR) == EINVAL);
    assert(set_pair(fd, key, sizeof(key), INT, value, 0, CHAR) == EINVAL);
    assert(set_pair(fd, key, sizeof(key), INT, value, sizeof(value), -1) == EINVAL);
}

void test_get_wrong_input(int fd) 
{   
    /* set pair for test to not get NO_PAIR instantly */
    assert(set_pair(fd, key, sizeof(key), CHAR, value, sizeof(value), CHAR) == 0);

    assert(get_value(-1, NULL, sizeof(key), CHAR) == NULL);
    assert(get_value(fd, NULL, sizeof(key), CHAR) == NULL);
    assert(get_value(fd, key, 0, CHAR) == NULL);
    
    /* delete pair and chek if deleted */
    assert(del_pair(fd, key, sizeof(key), CHAR) == 0);
    assert(get_value(fd, key, sizeof(key), CHAR) == NULL);
}

void test_del_wrong_input(int fd)
{   
    assert(del_pair(-1, NULL, sizeof(key), CHAR) == -1);
    assert(del_pair(fd, NULL, sizeof(key), CHAR) == EINVAL);
    assert(del_pair(fd, key, 0, CHAR) == EINVAL);
}

void test_overwrite(int fd)
{
    int new_value[] = {1, 2, 3};
    dict_pair *recieved;
    
    /* set pair and test if we can get it back */
    assert(set_pair(fd, key, sizeof(key), CHAR, value, sizeof(value), CHAR) == 0);
    recieved = get_value(fd, key, sizeof(key), CHAR);
    
    /* check if size, type and value returned correctly */
    assert(recieved != NULL);
    assert(recieved->value_size == sizeof(value));
    assert(recieved->value_type == CHAR);
    assert(memcmp(recieved->value, value, recieved->value_size) == 0);
    
    free(recieved->value);
    free(recieved);
    
    /* write new value */
    assert(set_pair(fd, key, sizeof(key), CHAR, new_value, sizeof(new_value), INT) == 0);
    recieved = get_value(fd, key, sizeof(key), CHAR);
    
    /* check if new size, type and value returned correctly */
    assert(recieved != NULL);
    assert(recieved->value_size == sizeof(new_value));
    assert(recieved->value_type == INT);
    assert(memcmp(recieved->value, new_value, recieved->value_size) == 0);
    
    /* delete pair */
    assert(del_pair(fd, key, sizeof(key), CHAR) == 0);
}


int main() {
	int fd;
	fd = open(DEVICE_PATH, O_RDWR); 
	
	test_set_wrong_input(fd);
	test_get_wrong_input(fd);
	test_del_wrong_input(fd);
	test_overwrite(fd);
	
	return 0;
}
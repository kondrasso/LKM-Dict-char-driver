#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "client.h"

static char key[] = "key\0";
static char value[] = "value\0";

void test_set_wrong_input(int fd) 
{   
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

    assert(get_value(fd, NULL, sizeof(key), CHAR) == NULL);
    assert(get_value(fd, key, 0, CHAR) == NULL);
    
    /* delete pair and chek if deleted */
    assert(del_pair(fd, key, sizeof(key), CHAR) == 0);
    assert(get_value(fd, key, sizeof(key), CHAR) == NULL);
}

void test_del_wrong_input(int fd)
{   
    assert(del_pair(fd, NULL, sizeof(key), CHAR) == EINVAL);
    assert(del_pair(fd, key, 0, CHAR) == EINVAL);
}

void test_overwrite(int fd)
{

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
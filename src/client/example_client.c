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

#include "client.h"


int main() {
	int fd;
	dict_pair *got;
	
	int  key[] = {0, 1, 2, 3};
	char value[] = "myval\0";   
	
	fd = open(DEVICE_PATH, O_RDWR); 
	
	/* set pair in dict */
	set_pair(fd, key, sizeof(key), INT, value, sizeof(value), CHAR);
	
	/* get value from dict */
	got = get_value(fd, key, sizeof(key), INT);    
	
	/* interpret returned value with type */
	if (got->value_type == CHAR && got->value != NULL) {
		printf("Got value %s\n", (char *)got->value);
	}   
	
	/* delete pair */
	del_pair(fd, key, sizeof(key), INT);   
	return 0;
}
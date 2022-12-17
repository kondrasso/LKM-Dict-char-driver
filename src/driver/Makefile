obj-m += dict_driver.o
kvs-objs :=  dict_driver.o
 
KDIR = /lib/modules/$(shell uname -r)/build

# gcc triggers on included kernel files, so if you want to see them - uncomment next line
# CFLAGS_dict_driver.o := -Wall -Wextra

all:
	make -C $(KDIR)  M=$(shell pwd) modules

clean:
	make -C $(KDIR)  M=$(shell pwd) clean

# Install and usage

In order to compile and insert your own custom LKM's, preliminary steps are required: please consult first and second chapters of the excellent [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/#headers) by Peter Jay Salzman, Michael Burian, Ori Pomerantz, Bob Mottram, Jim Huang. 

This project was compiled and tested on `Ubuntu 22.04.01 LTS` with `5.15.0-56-generic` kernel inside VM, with 2 cores and 8gb of RAM. Test generator requires `python 3.10`, no dependecies. 

## Out of the box case

1. Compile device driver via `cd src/driver && make`
2. Insert device driver via `sudo insmod my_dict_driver.ko`
   - It's better to check what dmesg says via `sudo dmesg -w` 
3. Compile any of scenarios in client, for example `cd /src/client/ && gcc small_test.c -o small_test`
4. Run the test `sudo ./small_test`
5. If asserts does not fail - it's all good, it's working!

## Generate new testing scenarios

For automatic test scenario generation use `/tests/generate_test.py` as follows:


``` 
$ python3 /tests/generate_test.py [--name --nt --np --maxlen --minlen --deleted --seed]

optional arguments:
  --name    name of testing scenario and corresponding .c file in /src/client/
  --np      number of pairs to be generated for each thread
  --maxlen  max length of the key/value
  --minlen  min length of the key/value
  --deleted use deleted scenario
  --seed    seed used for random key/values generation
```

Example command for generating big_test_deleted:


`python3 generate_test.py --name big_test_deleted --np 10000 --maxlen 40 --minlen 10 --deleted 1 --seed 44`


Example command for generating big_test:


`python3 generate_test.py --name big_test --np 10000 --maxlen 40 --minlen 10 --deleted 0 --seed 43`


Example command for generating small_test_deleted:


`python3 generate_test.py --name small_test --np 10 --maxlen 20 --minlen 10 --deleted 1 --seed 42`

Example command for generating small_test_deleted:


`python3 generate_test.py --name small_test --np 10 --maxlen 20 --minlen 10 --deleted 0 --seed 41`


## Writing your own tests

Take `/tests/func_head.c` and `/src/client/dict_interface.h` - you have API to use with driver. 


# General description

This repository contains "python-like dictionary" implemented as LKM character device driver. It's features:
- Set, get (value, value_size, value_type) and delete methods are implemented
- Generic container - stores key/value pairs of any type, if size provided
- Support type storage - if multiple users use same type indicators (via enum from dict_header.h, for example), they can get value with type and interpret it correctly
- Single interface - implemented as IOCTL, supports sending pre-defined structure that contains key, values, sizes, types
- Consistent with multi-threaded usage (via mutex_lock)
  
Current version does not support nested key-value pairs (thus "python-like", not "python"), but it could be implemented based on adding special type to data_types enum and implementing logic for handling it. 

# Implementation details

## Hash table

Dictionary at its core - hash table with separate chaining. Inspired mostly by [James Aspnes Notes on Data Structures and Programming Techniques](http://www.cs.yale.edu/homes/aspnes/classes/223/notes.html). Table starts at lower than hash size (default - 64 buckets), and grows as necessary. Hash is unsigned long that's cutoff via `hash % dict_table_size`. Collisions are handled by chaining (i.e. using linked list): if two pairs falls into the same bucket, equality of full hashes are checked, and if they are not equal, than put new pair at the head of the bucket, and link previous pair as next. Implementation resides in `/src/driver/my_dict_driver.c` after `DICT CORE API` comment. 

## IOCTL

There are five IOCTL calls that defined:

- SET_PAIR - copy pair structure from user, overwrite existing/add new pair
- GET_VALUE - copy pair structure from user with key and its size, find pair if exists and copy value to user 
- GET_VALUE_SIZE - copy pair structure from user with key and its size, find pair if exists and return `value_size`
- GET_VALUE_TYPE - copy pair structure from user with key and its size, find pair if exists and return `value_type`
- DEL_PAIR - copy pair structure from user with key and its size, delete if exists

## Locking

Locking implemented with single `mutex_lock`. It locks whole IOCTL function, so only one thread can have access to it, treating whole IOCTL function as critical section. This was made due to concern with hash table achitecture - when growth of the table occurs is hard to estimate, and it either should be separate method that checks after any set operation for more granular locking. Without table growth there are methods to use more granular strategies, like locking only one bucket []()

## Testing framework

Testing framework is simple python script - it generates functions for threads and key-value pairs (support int and char arrays). It's a crude way, but most simple and fast for generating tests in bulk. There are two kind of tests - with deletion and without. First represents worst case scenario, where threads will try to delete other threads pairs and after then try to get their own (which was deleted by other thread). Due to mutex locking whole IOCTL operation, there is a need for threads to wait some time until others finished deletion. It is implemented in simplest way possible - some sleep. Other scenario is the best case, where threads are only working with their own data, no problems. 

## Test structure

Test consits of static upper part `/tests/func_head.c` (with API functions implementations) and generated part: randomized key-value pairs (by type and lenght), for each thread; functions for each thread and main with multithreaded implementation via `pthread.h`. Each thread gets its own function with scenario, which consists of:

- Assert set pairs worked without memory errors
- Assert get value got correct value via memcmp
- Assert del pair worked without memory errors
- Assert get value did not get deleted values

Deleted scenario additionally sets values back and then tries to get and delete other threads values.

## Motivation of IOCTL usage

IOCTL was chosen with single goal in mind - provide somewhat uniform API, without using some byte offseting like in approaches that works exclusively with write/read. IOCTL allows to handle the burden of formatting input to the IOCTL via pre-defined sturctures (on user and kernel side), that eases parsing significantly. As downside of this apporach - user need to have IOCTL command, and API becomes "private". 


# Testing

All testing scenarios worked correctly (i.e. all asserts were correct);

## Memory leaks

On the user-side API generated test functions was tested via [Valgrind](https://valgrind.org/); since tests have same structure with any size, here are results for `small_test.c` and `small_test_deleted.c`: 


```
==65680== HEAP SUMMARY:
==65680==     in use at exit: 0 bytes in 0 blocks
==65680==   total heap usage: 184 allocs, 184 frees, 10,488 bytes allocated
==65680== 
==65680== All heap blocks were freed -- no leaks are possible
```

```
==69058== HEAP SUMMARY:
==69058==     in use at exit: 0 bytes in 0 blocks
==69058==   total heap usage: 364 allocs, 364 frees, 19,136 bytes allocated
==69058== 
==69058== All heap blocks were freed -- no leaks are possible
```

## Performance

This driver was in no way optimized for performance, but on my machine average resutls across 5 runs with `time` as follow:

- Big test: ~0.7 seconds for 90k operations
- Big test with deletion: ~8 seconds for 210k operations (including sleep)
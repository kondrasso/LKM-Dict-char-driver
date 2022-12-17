# How to compile

In order to compile and insert your own custom LKM's, preliminary steps are required: please consult first and second chapters of the excellent [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/#headers) by Peter Jay Salzman, Michael Burian, Ori Pomerantz, Bob Mottram, Jim Huang. 

This project was compiled and tested on `Ubuntu 22.04.01 LTS` with `5.15.0-56-generic` kernel inside VM, with 2 cores and 8gb of RAM.


## Compilation and usage

Makefile in root directory will compile and link all tests and example with client api, as well as trigger Makefile in `src/driver/`. So order of laucnch should be:

```
make
sudo insmod src/driver/dict_driver.ko
sudo ./example/example_client
```


**WARNING: stress tests writes/reads 3 million of pairs in typed case, or use 300 threads in untyped**

To run tests:

```
sudo ./tests/test_error_codes
sudo ./tests/test_stress_untyped
sudo ./tests/test_stress_typed
```

To clean binaries and .o files use `make clean` in root directory;

## Using API

Example code for operations located in `/example/example_client.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "../src/client/client.h"


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
	
	free(got->value);
	free(got);
	
	return 0;
}

```


# General description

This repository contains "python-like dictionary" implemented as LKM character device driver. It's features:
- Set, get (value, value_size, value_type) and delete methods are implemented
- Generic container - stores key/value pairs of any type (except NULL), if size provided
- Support type storage - if multiple users use same type indicators (via enum from `dict_interface.h`, for example), they can get values with type and interpret them correctly
- Single interface - implemented as IOCTL, supports sending and recieving pre-defined structure that contains key, values, sizes, types
- Consistent with multi-threaded usage (via mutex_lock)
  
Current version does not support nested key-value pairs (thus "python-like", not "python"), but it could be implemented based on adding special type to data_types enum and implementing logic for handling it. 

# Implementation details


## Project structure

```
├── example
│   └── example_client.c
├── LICENSE
├── Makefile
├── README.md
├── src
│   ├── client
│   │   ├── client.c
│   │   └── client.h
│   └── driver
│       ├── dict_driver.c
│       ├── dict_driver.h
│       └── Makefile
└── test
    ├── test_error_codes.c
    ├── test_stress_typed.c
    └── test_stress_untyped.c

```

- `src/driver` contains code for driver part of the project (header is used for structure definition)

- `src/client` contains client part of the project that need's to be linked to object that supposes to use API; header contains structure definitions and IOCTL calls macro

- `examples` contains example of driver API usage via IOCTL calls
 
- `tests` contains tests that described below


## Hash table

Dictionary at its core - hash table with separate chaining. Inspired mostly by [James Aspnes Notes on Data Structures and Programming Techniques](http://www.cs.yale.edu/homes/aspnes/classes/223/notes.html). Table starts at lower than hash size (default - 64 buckets), and grows as necessary, performing rehasing each growth. Hash is unsigned long that's cutoff via `hash % dict_table_size`. Collisions are handled by chaining (i.e. using linked list): if two pairs falls into the same bucket, equality of full hashes are checked, and if they are not equal, than put new pair at the head of the bucket, and link previous pair as next. Implementation resides in `/src/driver/dict_driver.c` after `DICT CORE API` comment. 

## IOCTL

There are five IOCTL calls that defined:

- SET_PAIR - copy pair structure from user, overwrite existing/add new pair
- GET_VALUE - copy pair structure from user with key and its size, find pair if exists and copy value to user 
- GET_VALUE_SIZE - copy pair structure from user with key and its size, find pair if exists and return `value_size`
- GET_VALUE_TYPE - copy pair structure from user with key and its size, find pair if exists and return `value_type`
- DEL_PAIR - copy pair structure from user with key and its size, delete if exists

## Locking

Locking implemented with single `mutex_lock`. It locks whole IOCTL function, so only one thread can have access to it, treating whole IOCTL function as critical section. This was made due to concern with hash table achitecture - when growth of the table occurs is hard to estimate, and it either should be separate method that checks after any set operation for more granular locking. There are methods to use more granular strategies, like locking only one bucket [Resizable, Scalable, Concurrent Hash Tables via Relativistic Programming](https://www.usenix.org/legacy/event/atc11/tech/final_files/Triplett.pdf) via RCU.

## Test structure

`test_error_codes` - test for correct handling and error return with wrong input; there is no elegant way (as I aware) to test correcntess of sizew of user-provided input in generic case, so this case are not covered by this test. Assert that wrong input will result in correct error code.

`test_stress_typed` - test for correct table upsizing and general work of driver under load; using 3 threads, do:
- Generate `NUM_OF_PAIRS` pairs of some type (for simplicity here is either `int` or `char`) of respective length
- Set all pairs and assert that it returned 0
- Get values for all keys, assert `memcmp` with original value is 0
- Delete all values
- Try to get deleted valuesm, assert that get returns `NO_PAIR` code

`test_stress_untyped` - similar to previous test, but with varying number of threads (default 300); performs same operations as `test_stress_typed`.

## Motivation of IOCTL usage

IOCTL was chosen with single goal in mind - provide somewhat uniform API, without using complicated file reading logic in approaches that works exclusively with write/read, especially for generic input. IOCTL allows to handle the burden of formatting input to the IOCTL via pre-defined sturctures (on user and kernel side), that eases parsing significantly. 

Downside of using IOCTL is twofold:

- API becomes "private" - i.e. you need structure definitions and IOCTL command numbers to use the module, so one dependecy more
- Limitations of IOCTL - you need to fit logic into single IOCTL call, and that becomes wasteful quickly: in this porject, for example, you need to do 3 separate IOCTL calls in order to recieve value size, type and the value itself. It can be downsided for two calls, but it will pollute logic of API and make error handling significantly harder 

## Memory leaks

On the user-side API generated test functions was tested via [Valgrind](https://valgrind.org/); since tests have same structure with any size, here are results for `example/examle_client.c` and `tests/test_stress_untyped`: 


```
==41272==
==41272== HEAP SUMMARY:
==41272==     in use at exit: 0 bytes in 0 blocks
==41272==   total heap usage: 5 allocs, 5 frees, 1,223 bytes allocated
==41272==
==41272== All heap blocks were freed -- no leaks are possible
==41272==
==41272== Use --track-origins=yes to see where uninitialised values come from
==41272== For lists of detected and suppressed errors, rerun with: -s
==41272== ERROR SUMMARY: 12 errors from 4 contexts (suppressed: 0 from 0)
```

```
==42380==
==42380== HEAP SUMMARY:
==42380==     in use at exit: 0 bytes in 0 blocks
==42380==   total heap usage: 21,000,010 allocs, 21,000,010 frees, 1,080,001,840 bytes allocated
==42380==
==42380== All heap blocks were freed -- no leaks are possible
==42380==
==42380== Use --track-origins=yes to see where uninitialised values come from
==42380== For lists of detected and suppressed errors, rerun with: -s
==42380== ERROR SUMMARY: 3000000 errors from 3 contexts (suppressed: 0 from 0)
```

According to Valgrind, there are no lost butyes, so this is good, but it show a lot of errors with memory usage, so lets see what Valgring says with `--track-origins=yes` on testing example_client inside client to see root cause or errors:

<details>
  <summary>Valgrind error log</summary>
  
  ```
	==20708== Memcheck, a memory error detector
	==20708== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
	==20708== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
	==20708== Command: ./client
	==20708==
	==20708== Conditional jump or move depends on uninitialised value(s)
	==20708==    at 0x484ED79: __strlen_sse2 (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
	==20708==    by 0x48DFDB0: __vfprintf_internal (vfprintf-internal.c:1517)
	==20708==    by 0x48C981E: printf (printf.c:33)
	==20708==    by 0x1097DC: main (in /home/ivan/LKM-Dict-char-driver/src/client/client)
	==20708==
	==20708== Conditional jump or move depends on uninitialised value(s)
	==20708==    at 0x484ED88: __strlen_sse2 (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
	==20708==    by 0x48DFDB0: __vfprintf_internal (vfprintf-internal.c:1517)
	==20708==    by 0x48C981E: printf (printf.c:33)
	==20708==    by 0x1097DC: main (in /home/ivan/LKM-Dict-char-driver/src/client/client)
	==20708==
	==20708== Conditional jump or move depends on uninitialised value(s)
	==20708==    at 0x48F47B7: _IO_new_file_xsputn (fileops.c:1218)
	==20708==    by 0x48F47B7: _IO_file_xsputn@@GLIBC_2.2.5 (fileops.c:1196)
	==20708==    by 0x48E008B: outstring_func (vfprintf-internal.c:239)
	==20708==    by 0x48E008B: __vfprintf_internal (vfprintf-internal.c:1517)
	==20708==    by 0x48C981E: printf (printf.c:33)
	==20708==    by 0x1097DC: main (in /home/ivan/LKM-Dict-char-driver/src/client/client)
	==20708==
	==20708== Syscall param write(buf) points to uninitialised byte(s)
	==20708==    at 0x497DA37: write (write.c:26)
	==20708==    by 0x48F3F6C: _IO_file_write@@GLIBC_2.2.5 (fileops.c:1180)
	==20708==    by 0x48F5A60: new_do_write (fileops.c:448)
	==20708==    by 0x48F5A60: _IO_new_do_write (fileops.c:425)
	==20708==    by 0x48F5A60: _IO_do_write@@GLIBC_2.2.5 (fileops.c:422)
	==20708==    by 0x48F4754: _IO_new_file_xsputn (fileops.c:1243)
	==20708==    by 0x48F4754: _IO_file_xsputn@@GLIBC_2.2.5 (fileops.c:1196)
	==20708==    by 0x48DF049: outstring_func (vfprintf-internal.c:239)
	==20708==    by 0x48DF049: __vfprintf_internal (vfprintf-internal.c:1593)
	==20708==    by 0x48C981E: printf (printf.c:33)
	==20708==    by 0x1097DC: main (in /home/ivan/LKM-Dict-char-driver/src/client/client)
	==20708==  Address 0x4a9419a is 10 bytes inside a block of size 1,024 alloc'd
	==20708==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
	==20708==    by 0x48E7C23: _IO_file_doallocate (filedoalloc.c:101)
	==20708==    by 0x48F6D5F: _IO_doallocbuf (genops.c:347)
	==20708==    by 0x48F5FDF: _IO_file_overflow@@GLIBC_2.2.5 (fileops.c:744)
	==20708==    by 0x48F4754: _IO_new_file_xsputn (fileops.c:1243)
	==20708==    by 0x48F4754: _IO_file_xsputn@@GLIBC_2.2.5 (fileops.c:1196)
	==20708==    by 0x48DE1CC: outstring_func (vfprintf-internal.c:239)
	==20708==    by 0x48DE1CC: __vfprintf_internal (vfprintf-internal.c:1263)
	==20708==    by 0x48C981E: printf (printf.c:33)
	==20708==    by 0x1097DC: main (in /home/ivan/LKM-Dict-char-driver/src/client/client)
	==20708==
	==20708==
	==20708== HEAP SUMMARY:
	==20708==     in use at exit: 0 bytes in 0 blocks
	==20708==   total heap usage: 5 allocs, 5 frees, 1,223 bytes allocated
	==20708==
	==20708== All heap blocks were freed -- no leaks are possible
	==20708==
	==20708== Use --track-origins=yes to see where uninitialised values come from
	==20708== For lists of detected and suppressed errors, rerun with: -s
	==20708== ERROR SUMMARY: 12 errors from 4 contexts (suppressed: 0 from 0)
  ```  

</details>

As far as I can tell, errors in other tests caused by similar issue - checking if `calloc` returned null and using IOCTL as syscall to write bytes somewhere (in driver write function is not defined implicitly, used only via IOCTL calls) and Valgrind does not like it. Iт large tests all error codes are similar.

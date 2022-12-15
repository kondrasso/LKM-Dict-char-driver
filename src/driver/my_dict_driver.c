#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>   
#include <linux/err.h>     
#include <linux/mutex.h>
#include "dict_header.h"

/* IOCTL's commands definition */

#define SET_PAIR _IOWR('a', 'a', pyld_pair *)
#define DEL_PAIR _IOWR('a', 'b', pyld_pair *)
#define GET_VALUE _IOWR('b', 'b', pyld_pair *)
#define GET_VALUE_SIZE _IOR('b', 'c', pyld_pair *)
#define GET_VALUE_TYPE _IOR('c', 'c', pyld_pair *)

/*  Dict concstants */

#define INITIAL_DICTSIZE 64
#define DICTSIZE_MULTIPLIER 2
#define DICT_GROW_DENSITY 1

/* Character device strutc declaration and function prototypes */

dev_t dev = 0;
static struct class *dev_class;
static struct cdev pyld_cdev;

static int __init pyld_driver_init(void);
static void __exit pyld_driver_exit(void);
static long pyld_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* Callback registration, others should default to NULL */

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = pyld_ioctl,
};


/* Pointer for dict shared object for whole driver */

pyld_dict *pd_ptr; 

/* Mutex struct */

struct mutex pyld_mutex;

/*
 *
 *                                  DRIVER CORE API
 *
 */

/** @brief Core function to communicate with userspace; gets and processes requests from user
 *  @param file file descriptor of the device
 *  @param cmd Number of IOCTL command that dictates how to process arg
 *  @param arg Contents of IOCTL request -  memory adress that points to message structure
 *  @return 0 if any ot the commands worked correctly (except get, that returns struct or int), 
 *  -EFAULT if there is memory faults, -EINVAL if provided from user values was inadequate 
 */
static long pyld_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{   
    void *key_adress;
    void *value_adress;
    pyld_pair *msg_dict;
    pyld_pair *found_pair;

    msg_dict = kzalloc(sizeof(pyld_pair), GFP_KERNEL);

    mutex_lock(&pyld_mutex);

    switch (cmd) {
    /* 
     * SET_PAIR ioctl call - get structure from user that contains types and
     * sizes as values, and pointers to key and value in userspcae; try 
     * copy from user, then set values to dict via pyld_set; types and sizes
     * should be sanitized in userspace part of IOCTL;
     * 
     * Returns 0 if nothing failed, otherwise -EFAULT; 
     */
    case SET_PAIR:
		pr_debug("SET_PAIR: start");
		int set_result;

		if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
			pr_err("SET_PAIR: cannot get msg from user");
			goto set_efault;
		}

		key_adress = msg_dict->key;
		value_adress = msg_dict->value;
		msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);
		msg_dict->value = kmalloc(msg_dict->value_size, GFP_KERNEL);

		if (msg_dict->key == NULL || msg_dict->value == NULL) {
			pr_err("SET_PAIR: kmalloc failed");
			goto set_efault_full;
		}

		if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
			pr_err("SET_PAIR: cannot get key from user");
			goto set_efault_full;
		}

		if (copy_from_user(msg_dict->value, value_adress, msg_dict->value_size)) {
			pr_err("SET_PAIR: cannot value key from user");
			goto set_efault_full;
		}

		set_result = pyld_set(  pd_ptr, 
					msg_dict->key, 
					msg_dict->value, 
					msg_dict->key_size, 
					msg_dict->value_size, 
					msg_dict->key_type, 
					msg_dict->value_type
				);

		if (set_result != 0) {
			pr_err("SET_PAIR: error setting pair");
			goto set_efault_full;
		}

		kfree(msg_dict->key);
		kfree(msg_dict->value);
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return 0;

set_efault_full:
		kfree(msg_dict->key);
		kfree(msg_dict->value);
set_efault:
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return -EFAULT;
	    
    /* 
     * GET_VALUE ioctl call - get structure from user that contains key's
     * type and size, as well as value size and type (they can be obtained
     * via two following IOCTL's); search for pair and copy it to user if 
     * exists;
     * 
     * Returns 0 if nothing failed, otherwise -EFAULT if memory errors
     * and -EINVAL if pair does not exists; 
     */
    case GET_VALUE:
		pr_debug("GET_VALUE: start");
		pyld_pair *found_pair;
		
		if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
			pr_err("GET_VALUE: cannot get from user");
			goto get_efault;
		}

		key_adress = msg_dict->key;
		value_adress = msg_dict->value;
		msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (msg_dict->key == NULL) {
			pr_err("GET_VALUE: kmalloc failed");
			goto get_efault;
		}

		if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
			pr_err("GET_VALUE: cannot get key");
			goto get_efault_full;
		}

		found_pair = pyld_get(pd_ptr, msg_dict->key, msg_dict->key_size);

		if (found_pair == NULL) {
			pr_info("GET_VALUE: no such pair");
			kfree(msg_dict->key);
			kfree(msg_dict);
			mutex_unlock(&pyld_mutex);
			return -EINVAL;
		}

		if (copy_to_user(value_adress, found_pair->value, found_pair->value_size)) {
			pr_err("GET_VALUE: cannot sent value to user");
			goto get_efault_full;
		}

		kfree(msg_dict->key);
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return 0;

get_efault_full:
		kfree(msg_dict->key);
get_efault:
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return -EFAULT;

   /* GET_VALUE_SIZE ioctl call - get structure from user that contains key's
    * type and size, search for the pair and return size if exists; 
    * 
    * Returns value_size if nothing failed, otherwise -EFAULT if memory errors
    * and -EINVAL if pair does not exists; 
    */
    case GET_VALUE_SIZE:
		pr_debug("GET_VALUE_SIZE: start\n");

		if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
			pr_err("GET_VALUE_SIZE: cannot get msg from user");
			goto get_size_efault;
		}

		key_adress = msg_dict->key;
		msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (msg_dict->key == NULL) {
			pr_err("GET_VALUE_SIZE: kmalloc failed");
			goto get_size_efault;
		}

		if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
			pr_err("GET_VALUE_SIZE: cannot get key from user");
			goto get_size_efault_full;
		}

		found_pair = pyld_get(pd_ptr, msg_dict->key, msg_dict->key_size);

		if (found_pair == NULL) {
			pr_err("GET_VALUE_SIZE: no such pair");
			kfree(msg_dict->key);
			kfree(msg_dict);
			mutex_unlock(&pyld_mutex);
			return -EINVAL;
		}
		
		kfree(msg_dict->key);
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return found_pair->value_size;

get_size_efault_full:
		kfree(msg_dict->key);
get_size_efault:
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return -EFAULT;

   /* 
    * GET_VALUE_TYPE ioctl call - get structure from user that contains key's
    * type and size, search for the pair and return type if exists; 
    * 
    * Returns value_type if nothing failed, otherwise -EFAULT if memory errors
    * and -EINVAL if pair does not exists; 
    */
    case GET_VALUE_TYPE:
	
		pr_debug("GET_VALUE_TYPE: start");

		if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
			pr_err("GET_VALUE_TYPE: cannot get msg from user");
			goto get_type_efault;
		}

		key_adress = msg_dict->key;
		msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (msg_dict->key == NULL) {
			pr_err("GET_VALUE_TYPE: kmalloc failed");
			goto get_type_efault;
		}

		if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
			pr_err("GET_VALUE_TYPE: cannot get key from user");
			goto get_type_efault_full;
		}

		found_pair = pyld_get(pd_ptr, msg_dict->key, msg_dict->key_size);

		if (found_pair == NULL) {
			pr_err("GET_VALUE_TYPE: no such pair");
			kfree(msg_dict->key);
			kfree(msg_dict);
			mutex_unlock(&pyld_mutex);
			return -EINVAL;
		}
		
		kfree(msg_dict->key);
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return found_pair->value_type;

get_type_efault_full:
		kfree(msg_dict->key);
get_type_efault:
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return -EFAULT;

   /* 
    * DEL_PAIR ioctl call - get structure from user that contains key's
    * type and size, search for the pair delete it if exists; else - 
    * do nothing;
    * 
    * Returns 0 if nothing failed, otherwise -EFAULT if memory errors
    */  
    case DEL_PAIR:

		pr_debug("DEL_PAIR : START");

		if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
			pr_err("DEL_PAIR : cannot get msg from user");
			goto del_pair_efault;
		}

		key_adress = msg_dict->key;
		msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (msg_dict->key == NULL) {
			pr_err("DEL_PAIR: kmalloc failed");
			goto del_pair_efault;
		}
		
		if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
			pr_err("DEL_PAIR : cannot get key from user");
			goto del_pair_efault_full;
		}
		
		pyld_del(pd_ptr, msg_dict->key, msg_dict->key_size);
		kfree(msg_dict->key);
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);   
		return 0;

del_pair_efault_full:
		kfree(msg_dict->key);
del_pair_efault:
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
		return -EFAULT;
	
    default:
		pr_err("Bad IOCTL command\n");
		kfree(msg_dict);
		mutex_unlock(&pyld_mutex);
	return -EINVAL;
    }

    return 0;
}

/** @brief  Init driver function - get major/minor numbers, create device class,
 *  mount it and initilize dict shared structure that will be used for storage,
 *  called on using insmod
 *  @return 0 on success, -1 on others
 */
static int __init pyld_driver_init(void)
{

    if ((alloc_chrdev_region(&dev, 0, 1, "pyld_Dev")) < 0) {
		pr_err("PYLD_INIT: cannot allocate major number\n");
		return -1;
    }

    /* Dynamic major and minor number allocation */

    pr_info("PYLD_INIT: Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));
    cdev_init(&pyld_cdev, &fops);

    /* Adding device as character device */

    if ((cdev_add(&pyld_cdev, dev, 1)) < 0) {
		pr_err("PYLD_INIT: cannot add the device to the system\n");
		goto r_class;
    }

    /* Creating device class */
    
    if (IS_ERR(dev_class = class_create(THIS_MODULE, "pyld_class"))) {
		pr_err("PYLD_INIT: cannot create the struct class\n");
		goto r_class;
    }

    /* Creating device */

    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "pyld_device"))) {
		pr_err("PYLD_INIT: cannot create the device\n");
		goto r_device;
    }
    
    pr_info("PYLD_INIT: device driver inserted\n");

    /* Initilizing dict */

    pd_ptr = pyld_create();

    if (pd_ptr == NULL) {
		pr_err("PYLD_INIT: dict was not initialized\n");
		goto r_device;
    }

    /* Initilize mutex at runtime */
    
    mutex_init(&pyld_mutex);
    
    pr_info("PYLD_INIT: dict initialized\n");

    return 0;

r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev, 1);
    return -1;
}


/** @brief  Exit driver function - destroy all device stuff, call dict destructor,
 *  and destroy mutex as well; called on rmmod'ing driver
 *  @return 0 on success, -1 on others
 */
static void __exit pyld_driver_exit(void)
{
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&pyld_cdev);
    unregister_chrdev_region(dev, 1);
    pyld_dict_destroy(pd_ptr);
    pr_info("PYLD_EXIT: device removed\n");
}

/*
 *
 *                                  DICT CORE API
 *
 */


/** @brief Dictionary descructor; traverses all buckets and linked lists, kfreeing memory
 *  @param pd Pointer to a shared dictionary object
 *  @return pyld_dict pointer to initilized object
 */
static pyld_dict *pyld_create()
{   
    int i;
    pyld_dict *pd = kmalloc(sizeof(pyld_dict), GFP_KERNEL);
    
    if (!pd) {
		return NULL;
    }

    pd->dict_size   = INITIAL_DICTSIZE;
    pd->num_entries = 0;
    pd->dict_table  = kzalloc(INITIAL_DICTSIZE * sizeof(pyld_pair), GFP_KERNEL);
 
    if (!pd->dict_table) {
		kfree(pd);
		return NULL;
    }

    return pd;
}


/** @brief Dictionary descructor; traverses all buckets and linked lists, freeing memory
 *  @param pd Pointer to a shared dictionary object
 *  @return NULL
 */
static void pyld_dict_destroy(pyld_dict *d)
{
    int i;
    pyld_pair *curr;
    pyld_pair *next;

    for (i = 0; i < d->dict_size; i++) {
		for (curr = d->dict_table[i]; curr != NULL; curr = next) {
			next = curr->next;
			kfree(curr->key);
			kfree(curr->value);
			kfree(curr);
		}
    }

    kfree(d->dict_table);
    kfree(d);
}


/** @brief Search for pair with matching key in dict; if exists - rewrite value
 *  else - create new entry and link it to the head of the bucket
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param value  Pointer to value location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 *  @param value_size Size of value, follows sizeof() format with size_t
 *  @param key_type Number that charachterizes key for casting in userspace
 *  @param value_type Number that charachterizes key for casting in userspace
 *  @return 0 on success, -EFAULT is there are memory allocation problems
 */
static int pyld_set(
    pyld_dict *pd, 
    void *key, 
    void *value, 
    size_t key_size, 
    size_t value_size, 
    int key_type, 
    int value_type)
{
    int bucket_id;
    unsigned long hash;
    pyld_pair *curr;
    pyld_pair *new_entry;

    hash = hash_mem(key, key_size);
    bucket_id = hash % pd->dict_size;

    curr = pd->dict_table[bucket_id];

    while (curr != NULL) {   
		if (curr->key_hash == hash) {
			kfree(curr->value);
			curr->value = kzalloc(value_size, GFP_KERNEL);
			memcpy(curr->value, value, value_size);
			curr->value_size = value_size;
			curr->value_type = value_type;
			return 0;
		}
		curr = curr->next;
    }

    new_entry = kzalloc(1 * sizeof(pyld_pair), GFP_KERNEL);

    if (new_entry == NULL) {
		return -EFAULT;
    }

    new_entry->key_hash         = hash;
    new_entry->key_type         = key_type;
    new_entry->key_size         = key_size;
    new_entry->value_type       = value_type;
    new_entry->value_size       = value_size;
    new_entry->key              = kzalloc(1 * key_size, GFP_KERNEL);
    
    if (new_entry-> key == NULL) {
		kfree(new_entry);
		return -EFAULT;
    }

    new_entry->value            = kzalloc(1 * value_size, GFP_KERNEL);
    
    if (new_entry->value == NULL) {
		kfree(new_entry->key);
		kfree(new_entry);
		return -EFAULT;
    }
    
    memcpy(new_entry->key, key, key_size);
    memcpy(new_entry->value, value, value_size);

    new_entry->next = pd->dict_table[bucket_id];
    pd->dict_table[bucket_id] = new_entry;
    pd->num_entries++;

    if (pd->num_entries > pd->dict_size * DICT_GROW_DENSITY) {
		pyld_dict_grow(pd);
    }

    return 0;
}


/** @brief Get pair struct by the provided key with respective size
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param key_size  size of key, follows sizeof() format with size_t
 *  @return Struct containing value for matching key; NULL if pair does not exist
 */
static pyld_pair *pyld_get(pyld_dict *pd, const void *key, size_t key_size)
{
    int bucket_id;
    unsigned long hash;
    pyld_pair *curr;

    hash = hash_mem(key, key_size);
    bucket_id = hash % pd->dict_size;
    curr = pd->dict_table[bucket_id];

    if (curr == NULL) {
		pr_info("PLD_GET: no such pair");
		return NULL;
    }

    while (curr) {
		if (curr->key_hash == hash) {
			return curr;
		}
		curr = curr->next;
    }
    return NULL;
}

/** @brief Grow dict by DICTSIZE_MULTIPLIER to fit new entires, rehash all entries
 *  @param pd Pointer to a shared dictionary object
 *  @return NULL
 */
static void pyld_dict_grow(pyld_dict *pd)
{
    int i;
    int new_index;
    int new_size;

    pyld_pair *old_curr;
    pyld_pair *new_curr;
    pyld_pair **new_table;

    new_size = pd->dict_size * DICTSIZE_MULTIPLIER;
    new_table = kzalloc(new_size * sizeof(*new_table), GFP_KERNEL);

    if (new_table == NULL) {
		pr_err("DICT_GROW: kzalloc failed");
		return;
    }

    for (i = 0; i < pd->dict_size; i++) {
		old_curr = pd->dict_table[i];
		while (old_curr) {
			new_index = old_curr->key_hash % new_size;
			new_curr = old_curr;
			old_curr = old_curr->next;
			new_curr->next = new_table[new_index];
			new_table[new_index] = new_curr;
		}
    }

    pd->dict_size = new_size;
    kfree(pd->dict_table);
    pd->dict_table = new_table;
    return;
}




/** @brief Pair deletion - delete pair (free its memory) if it exists, else do nothing
 *  @param pd Pointer to a shared dictionary object
 *  @param key Pointer to key location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 */
static void pyld_del(pyld_dict *pd, void *key, size_t key_size)
{
    pyld_pair *curr;
    pyld_pair *prev;
    pyld_pair *head;
    unsigned long hash;
    int bucket_id;
    
    hash = hash_mem(key, key_size);
    bucket_id = hash % pd->dict_size;

    curr = pd->dict_table[bucket_id];
    head = pd->dict_table[bucket_id];

    if (curr == NULL) {
		return;
    }

    if (curr->key_hash == hash) {
		pd->dict_table[bucket_id] = curr->next;
		goto deleted;
    }

    prev = curr;
    curr = curr->next;

    while(curr) {
		if (curr->key_hash == hash) {
			prev->next = curr->next;
			goto deleted;
		}
		prev = curr;
		curr = curr->next;
    }

    return;

deleted:
    kfree(curr->key);
    kfree(curr->value);
    kfree(curr);
    if (pd->num_entries > 1) {
		pd->num_entries--; 
    }
    return; 
}

/* 
 * Simple memory hash function, credits to James Aspnes data structures course
 * http://www.cs.yale.edu/homes/aspnes/classes/223/notes.html
 */
static unsigned long hash_mem(const unsigned char *s, size_t len)
{
    unsigned long h;
    int i;

    h = 0;
    for (i = 0; i < len; i++) {
		h = (h << 13) + (h >> 7) + h + s[i];
    }
    return h;
}

module_init(pyld_driver_init);
module_exit(pyld_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Kondratov");
MODULE_DESCRIPTION("Python-like dict LKM");
MODULE_VERSION("1");
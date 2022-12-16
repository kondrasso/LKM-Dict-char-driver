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

#include "dict_driver.h"

/* IOCTL's commands definition */

#define SET_PAIR _IOWR('a', 'a', dict_pair *)
#define DEL_PAIR _IOWR('a', 'b', dict_pair *)
#define GET_VALUE _IOWR('b', 'b', dict_pair *)
#define GET_VALUE_SIZE _IOWR('b', 'c', dict_pair *)
#define GET_VALUE_TYPE _IOR('c', 'c', dict_pair *)

/*  Dict constants */

#define INITIAL_DICTSIZE 64
#define DICTSIZE_MULTIPLIER 2
#define DICT_GROW_DENSITY 1

/* Character device strutc declaration and function prototypes */

dev_t dev = 0;
static struct class *dev_class;
static struct cdev dict_cdev;

static int __init dict_driver_init(void);
static void __exit dict_driver_exit(void);
static long dict_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* Dictionary function prototypes */

static dict *dict_create(void);
static void dict_destroy(dict *);
static int dict_set(dict *, void *, void *, dict_pair *);
static dict_pair *dict_get(dict *, const void *, size_t);
static void dict_grow(dict *);
static void dict_del(dict *, void *, size_t);
static unsigned long hash_mem(const unsigned char *, size_t);

/* Callback registration, others should default to NULL */

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dict_ioctl,
};


/* Pointer for dict shared object for whole driver */

dict *pd_ptr;

/* Mutex struct */

struct mutex dict_mutex;

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
static long dict_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void *key;
	void *value;
	long retval;
	dict_pair *msg_dict;
	dict_pair *found_pair;

	msg_dict = kzalloc(sizeof(dict_pair), GFP_KERNEL);

	mutex_lock(&dict_mutex);

	switch (cmd) {
	/*
	 * SET_PAIR ioctl call - get structure from user that contains types and
	 * sizes as values, and pointers to key and value in userspcae; try
	 * copy from user, then set values to dict via dict_set; types and sizes
	 * should be sanitized in userspace part of IOCTL;
	 *
	 * Returns 0 if nothing failed, otherwise -EFAULT;
	 */
	case SET_PAIR:

		pr_debug("SET_PAIR: start");

		if (copy_from_user(msg_dict, (dict_pair *)arg, sizeof(dict_pair))) {
			pr_err("SET_PAIR: cannot get msg from user");
			retval = EFAULT;
			goto set_exit;
		}

		if (msg_dict->key == NULL || msg_dict->value == NULL) {
			pr_err("SET_PAIR: NULL as key or value");
			retval = EINVAL;
			goto set_exit;
		}

		if (msg_dict->key_size == 0 || msg_dict->value_size == 0
			|| msg_dict->key_type < 0 || msg_dict->value_type < 0) {
			pr_err("SET_PAIR: illegal size");
			retval = EINVAL;
			goto set_exit;
		}

		key = kmalloc(msg_dict->key_size, GFP_KERNEL);
		value = kmalloc(msg_dict->value_size, GFP_KERNEL);

		if (key == NULL || value == NULL) {
			pr_err("SET_PAIR: kmalloc failed");
			retval = ENOMEM;
			goto set_exit;
		}

		if (copy_from_user(key, msg_dict->key, msg_dict->key_size)) {
			pr_err("SET_PAIR: cannot get key from user");
			retval = EFAULT;
			goto set_exit_full;
		}

		if (copy_from_user(value, msg_dict->value, msg_dict->value_size)) {
			pr_err("SET_PAIR: cannot value key from user");
			retval = EFAULT;
			goto set_exit_full;
		}

		retval = dict_set(pd_ptr, key, value, msg_dict);

set_exit_full:
		kfree(key);
		kfree(value);
set_exit:
		kfree(msg_dict);
		mutex_unlock(&dict_mutex);
		return retval;

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

		if (copy_from_user(msg_dict, (dict_pair *)arg, sizeof(dict_pair))) {
			pr_err("GET_VALUE: cannot get from user");
			retval = EFAULT;
			goto get_exit;
		}

		if (msg_dict->key == NULL || msg_dict->key_size == 0) {
			pr_err("GET_VALUE: NULL as key or zero key size");
			retval = EINVAL;
			goto get_exit;
		}

		key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (key == NULL) {
			pr_err("GET_VALUE: kmalloc failed");
			retval = ENOMEM;
			goto get_exit;
		}

		if (copy_from_user(key, msg_dict->key, msg_dict->key_size)) {
			pr_err("GET_VALUE: cannot get key");
			retval = EFAULT;
			goto get_exit_full;
		}

		found_pair = dict_get(pd_ptr, key, msg_dict->key_size);

		if (found_pair == NULL) {
			pr_info("GET_VALUE: no such pair");
			retval = ENOENT;
			goto get_exit_full;
		}

		if (copy_to_user(msg_dict->value, found_pair->value, found_pair->value_size)) {
			pr_err("GET_VALUE: cannot sent value to user");
			retval = EFAULT;
			goto get_exit_full;
		}

		retval = 0;

get_exit_full:
		kfree(key);
get_exit:
		kfree(msg_dict);
		mutex_unlock(&dict_mutex);
		return retval;

   /* GET_VALUE_SIZE ioctl call - get structure from user that contains key's
	* type and size, search for the pair and return size if exists;
	*
	* Returns value_size if nothing failed, otherwise -EFAULT if memory errors
	* and -EINVAL if pair does not exists;
	*/
	case GET_VALUE_SIZE:
		pr_debug("GET_VALUE_SIZE: start\n");

		if (copy_from_user(msg_dict, (dict_pair *)arg, sizeof(dict_pair))) {
			pr_err("GET_VALUE_SIZE: cannot get msg from user");
			retval = EFAULT;
			goto get_size_exit;
		}

		if (msg_dict->key == NULL || msg_dict->key_size == 0) {
			pr_err("GET_VALUE_SIZE: NULL as key or zero key size");
			retval = EINVAL;
			goto get_size_exit;
		}
		
		if (msg_dict->value_size_adress == NULL) {
			pr_err("GET_VALUE_SIZE: NULL value size adress");
			retval = EINVAL;
			goto get_size_exit;
		}

		key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (key == NULL) {
			pr_err("GET_VALUE_SIZE: kmalloc failed");
			retval = ENOMEM;
			goto get_size_exit;
		}

		if (copy_from_user(key, msg_dict->key, msg_dict->key_size)) {
			pr_err("GET_VALUE_SIZE: cannot get key from user");
			retval = EFAULT;
			goto get_size_exit_full;
		}

		found_pair = dict_get(pd_ptr, key, msg_dict->key_size);

		if (found_pair == NULL) {
			pr_err("GET_VALUE_SIZE: no such pair");
			retval = NO_PAIR;
			goto get_size_exit_full;
		}
		
		if (copy_to_user(msg_dict->value_size_adress, &found_pair->value_size, sizeof(size_t))) {
			pr_err("GET_VALUE_SIZE: cannot get key from user");
			retval = EFAULT;
			goto get_size_exit_full;
		}

		retval = 0;

get_size_exit_full:
		kfree(key);
get_size_exit:
		kfree(msg_dict);
		mutex_unlock(&dict_mutex);
		return retval;

   /*
	* GET_VALUE_TYPE ioctl call - get structure from user that contains key's
	* type and size, search for the pair and return type if exists;
	*
	* Returns value_type if nothing failed, otherwise -EFAULT if memory errors
	* and -EINVAL if pair does not exists;
	*/
	case GET_VALUE_TYPE:

		pr_debug("GET_VALUE_TYPE: start");

		if (copy_from_user(msg_dict, (dict_pair *)arg, sizeof(dict_pair))) {
			pr_err("GET_VALUE_TYPE: cannot get msg from user");
			retval = EFAULT;
			goto get_type_exit;
		}

		if (msg_dict->key == NULL || msg_dict->key_size == 0) {
			pr_err("GET_VALUE_TYPE: NULL as key or key_size is zero");
			retval = EINVAL;
			goto get_type_exit;
		}

		key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (key == NULL) {
			pr_err("GET_VALUE_TYPE: kmalloc failed");
			retval = ENOMEM;
			goto get_type_exit;
		}

		if (copy_from_user(key, msg_dict->key, msg_dict->key_size)) {
			pr_err("GET_VALUE_TYPE: cannot get key from user");
			retval = EFAULT;
			goto get_type_exit_full;
		}

		found_pair = dict_get(pd_ptr, key, msg_dict->key_size);

		if (found_pair == NULL) {
			pr_err("GET_VALUE_TYPE: no such pair");
			retval = ENOENT;
			goto get_type_exit_full;
		}

		retval = found_pair->value_type;

get_type_exit_full:
		kfree(key);
get_type_exit:
		kfree(msg_dict);
		mutex_unlock(&dict_mutex);
		return retval;

   /*
	* DEL_PAIR ioctl call - get structure from user that contains key's
	* type and size, search for the pair delete it if exists; else -
	* do nothing;
	*
	* Returns 0 if nothing failed, otherwise -EFAULT if memory errors
	*/
	case DEL_PAIR:

		pr_debug("DEL_PAIR : START");

		if (copy_from_user(msg_dict, (dict_pair *)arg, sizeof(dict_pair))) {
			pr_err("DEL_PAIR : cannot get msg from user");
			retval = EFAULT;
			goto del_pair_exit;
		}

		if (msg_dict->key == NULL || msg_dict->key_size == 0) {
			pr_err("GET_VALUE_TYPE: NULL as key or key_size is zero");
			retval = EINVAL;
			goto del_pair_exit;
		}

		key = kmalloc(msg_dict->key_size, GFP_KERNEL);

		if (key == NULL) {
			pr_err("GET_VALUE_TYPE: kmalloc failed");
			retval = ENOMEM;
			goto del_pair_exit;
		}

		if (copy_from_user(key, msg_dict->key, msg_dict->key_size)) {
			pr_err("DEL_PAIR : cannot get key from user");
			retval = EFAULT;
			goto del_pair_exit_full;
		}

		dict_del(pd_ptr, key, msg_dict->key_size);

		retval = 0;

del_pair_exit_full:
		kfree(key);
del_pair_exit:
		kfree(msg_dict);
		mutex_unlock(&dict_mutex);
		return retval;

	default:
		pr_err("Bad IOCTL command\n");
		kfree(msg_dict);
		mutex_unlock(&dict_mutex);
		return EINVAL;
	}

	return 0;
}

/** @brief  Init driver function - get major/minor numbers, create device class,
 *  mount it and initilize dict shared structure that will be used for storage,
 *  called on using insmod
 *  @return 0 on success, -1 on others
 */
static int __init dict_driver_init(void)
{

	if ((alloc_chrdev_region(&dev, 0, 1, "dict_Dev")) < 0) {
		pr_err("DICT_INIT: cannot allocate major number\n");
		return -1;
	}

	/* Dynamic major and minor number allocation */

	pr_info("DICT_INIT: Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));
	cdev_init(&dict_cdev, &fops);

	/* Adding device as character device */

	if ((cdev_add(&dict_cdev, dev, 1)) < 0) {
		pr_err("DICT_INIT: cannot add the device to the system\n");
		goto r_class;
	}

	/* Creating device class */

	if (IS_ERR(dev_class = class_create(THIS_MODULE, "dict_class"))) {
		pr_err("DICT_INIT: cannot create the struct class\n");
		goto r_class;
	}

	/* Creating device */

	if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "dict_device"))) {
		pr_err("DICT_INIT: cannot create the device\n");
		goto r_device;
	}

	pr_info("DICT_INIT: device driver inserted\n");

	/* Initilizing dict */

	pd_ptr = dict_create();

	if (pd_ptr == NULL) {
		pr_err("DICT_INIT: dict was not initialized\n");
		goto r_device;
	}

	/* Initilize mutex at runtime */

	mutex_init(&dict_mutex);

	pr_info("DICT_INIT: dict initialized\n");

	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	cdev_del(&dict_cdev);
	return -1;
}


/** @brief  Exit driver function - destroy all device stuff, call dict destructor,
 *  and destroy mutex as well; called on rmmod'ing driver
 *  @return 0 on success, -1 on others
 */
static void __exit dict_driver_exit(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&dict_cdev);
	unregister_chrdev_region(dev, 1);
	dict_destroy(pd_ptr);
	pr_info("DICT_EXIT: device removed\n");
}

/*
 *
 *                                  DICT CORE API
 *
 */


/** @brief Dictionary constructor; allocates initial table
 *  @param pd Pointer to a shared dictionary object
 *  @return dict pointer to initilized object
 */
static dict *dict_create()
{
	dict *pd = kmalloc(sizeof(dict), GFP_KERNEL);

	if (pd == NULL) {
		pr_err("DICT_CREATE: kmalloc for dict failed");
		return NULL;
	}

	pd->dict_size   = INITIAL_DICTSIZE;
	pd->num_entries = 0;
	pd->dict_table  = kzalloc(INITIAL_DICTSIZE * sizeof(dict_pair), GFP_KERNEL);

	if (pd->dict_table == NULL) {
		pr_err("DICT_CREATE: kzalloc for dict_table failed");
		kfree(pd);
		return NULL;
	}

	return pd;
}


/** @brief Dictionary descructor; traverses all buckets and linked lists, freeing memory
 *  @param pd Pointer to a shared dictionary object
 *  @return NULL
 */
static void dict_destroy(dict *d)
{
	int i;
	dict_pair *curr;
	dict_pair *next;

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
 *  @param msg_dict Container from user that contains size/type info
 *  @return 0 on success
 */
static int dict_set(dict *pd, void *key, void *value, dict_pair *msg_dict)
{
	int bucket_id;
	unsigned long hash;
	dict_pair *curr;
	dict_pair *new_entry;

	hash = hash_mem(msg_dict->key, msg_dict->key_size);
	bucket_id = hash % pd->dict_size;

	curr = pd->dict_table[bucket_id];

	while (curr != NULL) {
		if (curr->key_hash == hash && curr->key_size == msg_dict->key_size) {
			if (!memcmp(curr->key, msg_dict->key, msg_dict->key_size)) {
				kfree(curr->value);
				curr->value = kzalloc(msg_dict->value_size, GFP_KERNEL);
				memcpy(curr->value, value, msg_dict->value_size);
				curr->value_size = msg_dict->value_size;
				curr->value_type = msg_dict->value_type;
				return 0;
			}
		}
		curr = curr->next;
	}

	new_entry = kzalloc(sizeof(dict_pair), GFP_KERNEL);

	if (new_entry == NULL) {
		return -ENOMEM;
	}

	new_entry->key_hash         = hash;
	new_entry->key_type         = msg_dict->key_type;
	new_entry->key_size         = msg_dict->key_size;
	new_entry->value_type       = msg_dict->value_type;
	new_entry->value_size       = msg_dict->value_size;
	new_entry->key              = kzalloc(msg_dict->key_size, GFP_KERNEL);
	new_entry->value            = kzalloc(msg_dict->value_size, GFP_KERNEL);

	if (new_entry-> key == NULL) {
		kfree(new_entry);
		return -ENOMEM;
	}
	
	if (new_entry->value == NULL) {
		kfree(new_entry->key);
		kfree(new_entry);
		return -ENOMEM;
	}

	memcpy(new_entry->key, key, msg_dict->key_size);
	memcpy(new_entry->value, value, msg_dict->value_size);

	new_entry->next = pd->dict_table[bucket_id];
	pd->dict_table[bucket_id] = new_entry;
	pd->num_entries++;

	if (pd->num_entries > pd->dict_size * DICT_GROW_DENSITY) {
		dict_grow(pd);
	}

	return 0;
}


/** @brief Get pair struct by the provided key with respective size
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param key_size  size of key, follows sizeof() format with size_t
 *  @return Struct containing value for matching key; NULL if pair does not exist
 */
static dict_pair *dict_get(dict *pd, const void *key, const size_t key_size)
{
	int bucket_id;
	unsigned long hash;
	dict_pair *curr;

	hash = hash_mem(key, key_size);
	bucket_id = hash % pd->dict_size;
	curr = pd->dict_table[bucket_id];

	if (curr == NULL) {
		return NULL;
	}

	while (curr) {
		if (curr->key_hash == hash && curr->key_size == key_size) {
			if (!memcmp(curr->key, key, key_size)) {
				return curr;
			}
		}
		curr = curr->next;
	}
	return NULL;
}

/** @brief Grow dict by DICTSIZE_MULTIPLIER to fit new entires, rehash all entries
 *  @param pd Pointer to a shared dictionary object
 *  @return NULL
 */
static void dict_grow(dict *pd)
{
	int i;
	int new_index;
	int new_size;

	dict_pair *old_curr;
	dict_pair *new_curr;
	dict_pair **new_table;

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
	
	kfree(pd->dict_table);
	pd->dict_size = new_size;
	pd->dict_table = new_table;
	return;
}




/** @brief Pair deletion - delete pair (free its memory) if it exists, else do nothing
 *  @param pd Pointer to a shared dictionary object
 *  @param key Pointer to key location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 */
static void dict_del(dict *pd, void *key, size_t key_size)
{
	int bucket_id;
	unsigned long hash;

	dict_pair *curr;
	dict_pair *prev;

	hash = hash_mem(key, key_size);
	bucket_id = hash % pd->dict_size;

	curr = pd->dict_table[bucket_id];

	if (curr == NULL) {
		return;
	}

	if (curr->key_hash == hash && curr->key_size == key_size) {
		if (!memcmp(curr->key, key, key_size)) {
			pd->dict_table[bucket_id] = curr->next;
			goto deleted;	
		}
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

module_init(dict_driver_init);
module_exit(dict_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Kondratov");
MODULE_DESCRIPTION("Python-like dict LKM");
MODULE_VERSION("1");
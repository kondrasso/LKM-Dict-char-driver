#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>    // kzalloc() / kmalloc
#include <linux/uaccess.h> // copy_to_user() / copy_from_user()
#include <linux/ioctl.h>   // ioctl helper macros and definition
#include <linux/err.h>     
#include <linux/mutex.h>
#include "dict_header.h"

// IOCTL's commands definition

#define SET_PAIR _IOWR('a', 'a', pyld_pair *)
#define DEL_PAIR _IOWR('a', 'b', pyld_pair *)
#define GET_VALUE _IOWR('b', 'b', pyld_pair *)
#define GET_VALUE_SIZE _IOR('b', 'c', pyld_pair *)
#define GET_VALUE_TYPE _IOR('c', 'c', pyld_pair *)

// Dict concstants 

#define INITIAL_DICTSIZE (64)
#define DICTSIZE_MULTIPLIER (2)
#define DICT_GROW_DENSITY (1)

// Character device strutc declaration and function prototypes

dev_t dev = 0;
static struct class *dev_class;
static struct cdev pyld_cdev;

static int __init pyld_driver_init(void);
static void __exit pyld_driver_exit(void);
static long pyld_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

// Callback registration, others should default to NULL

static struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .unlocked_ioctl = pyld_ioctl,
    };


// Pointer for dict shared object for whole driver

pyld_dict *pd_ptr; 

// Mutex struct

struct mutex pyld_mutex;

/*
 *
 *                                  DRIVER CORE API
 *
 */

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static long pyld_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{   
    void *key_adress;
    void *value_adress;
    pyld_pair *msg_dict;
    pyld_pair *ans;

    msg_dict = kzalloc(sizeof(pyld_pair), GFP_KERNEL);

    mutex_lock(&pyld_mutex);

    switch (cmd) {
    case SET_PAIR:
        
        pr_info("SET_PAIR: start");

        if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
            pr_err("SET_PAIR: cannot get msg from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        key_adress = msg_dict->key;
        value_adress = msg_dict->value;
        msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);
        msg_dict->value = kmalloc(msg_dict->value_size, GFP_KERNEL);

        if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
            pr_err("SET_PAIR: cannot get key from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        if (copy_from_user(msg_dict->value, value_adress, msg_dict->value_size)) {
            pr_err("SET_PAIR: cannot value key from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        pyld_set(
            pd_ptr, 
            msg_dict->key, 
            msg_dict->value, 
            msg_dict->key_size, 
            msg_dict->value_size, 
            msg_dict->key_type, 
            msg_dict->value_type
        );

        kfree(msg_dict);
        mutex_unlock(&pyld_mutex);
        return 0;
        break;

    case GET_VALUE:
        
        pr_info("GET_VALUE: start\n");
        
        pyld_pair *ans;
        
        if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
            pr_err("GET_VALUE: cannot get from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        key_adress = msg_dict->key;
        value_adress = msg_dict->value;
        msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

        if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
            pr_err("GET_VALUE: cannot get key");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        ans = pyld_get(pd_ptr, msg_dict->key, msg_dict->key_size);

        if (ans == NULL) {
            pr_info("GET_VALUE: no such pair");
            mutex_unlock(&pyld_mutex);
            return -EINVAL;
        }

        if (copy_to_user(value_adress, ans->value, ans->value_size)) {
            pr_err("GET_VALUE: cannot sent value to user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        kfree(msg_dict);
        mutex_unlock(&pyld_mutex);
        
        return 0;
        
        break;

    case GET_VALUE_SIZE:
        
        pr_info("GET_VALUE_SIZE: start\n");

        if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
            pr_err("GET_VALUE_SIZE: cannot get msg from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        key_adress = msg_dict->key;
        msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

        if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
            pr_err("GET_VALUE_SIZE: cannot get key from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        ans = pyld_get(pd_ptr, msg_dict->key, msg_dict->key_size);

        if (ans == NULL) {
            pr_info("GET_VALUE_SIZE: no such pair");
            mutex_unlock(&pyld_mutex);
            return -EINVAL;
        }
        
        kfree(msg_dict);
        mutex_unlock(&pyld_mutex);
        
        return ans->value_size;
        
        break;

    case GET_VALUE_TYPE:
        
        pr_info("GET_VALUE_TYPE: start\n");

        if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
            pr_err("GET_VALUE_TYPE: cannot get msg from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        key_adress = msg_dict->key;
        msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);

        if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
            pr_err("GET_VALUE_TYPE: cannot get key from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        ans = pyld_get(pd_ptr, msg_dict->key, msg_dict->key_size);

        if (ans == NULL) {
            pr_info("GET_VALUE_TYPE: no such pair");
            mutex_unlock(&pyld_mutex);
            return -EINVAL;
        }
        
        kfree(msg_dict);
        mutex_unlock(&pyld_mutex);
        
        return ans->value_type;
        
        break;
        
    case DEL_PAIR:

        pr_info("DEL_PAIR : START");

        if (copy_from_user(msg_dict, (pyld_pair *)arg, sizeof(pyld_pair))) {
            pr_info("DEL_PAIR : cannot get msg from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }

        key_adress = msg_dict->key;
        msg_dict->key = kmalloc(msg_dict->key_size, GFP_KERNEL);
        
        if (copy_from_user(msg_dict->key, key_adress, msg_dict->key_size)) {
            pr_info("DEL_PAIR : cannot get key from user");
            mutex_unlock(&pyld_mutex);
            return -EFAULT;
        }
        
        pyld_del(pd_ptr, msg_dict->key, msg_dict->key_size);
        kfree(msg_dict);
        mutex_unlock(&pyld_mutex);
        
        return 0;
        
        break;
    
    default:
        pr_err("Bad IOCTL command\n");
        mutex_unlock(&pyld_mutex);
        return -EINVAL;
        break;
    }
    return 0;
}

static int __init pyld_driver_init(void)
{
    // Dynamic major number allocation 

    if ((alloc_chrdev_region(&dev, 0, 1, "pyld_Dev")) < 0) {
        pr_err("PYLD_INIT: cannot allocate major number\n");
        return -1;
    }

    pr_info("PYLD_INIT: Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

    // Creating cdev structure 

    cdev_init(&pyld_cdev, &fops);

    // Adding device as character device

    if ((cdev_add(&pyld_cdev, dev, 1)) < 0) {
        pr_err("PYLD_INIT: cannot add the device to the system\n");
        goto r_class;
    }

    // Creating device class
    
    if (IS_ERR(dev_class = class_create(THIS_MODULE, "pyld_class"))) {
        pr_err("PYLD_INIT: cannot create the struct class\n");
        goto r_class;
    }

    // Creating device

    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "pyld_device"))) {
        pr_err("PYLD_INIT: cannot create the device\n");
        goto r_device;
    }
    
    pr_info("PYLD_INIT: device driver inserted\n");

    // Initilizing dict

    pd_ptr = pyld_create();

    if (pd_ptr == NULL) {
        pr_err("PYLD_INIT: dict was not initialized\n");
        goto r_device;
    }

  
    mutex_init(&pyld_mutex);
    pr_info("PYLD_INIT: dict initialized\n");

    return 0;

r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev, 1);
    return -1;
}

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


/** @brief Dictionary descructor; traverses all buckets and linked lists, freeing memory
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
 
    // just to be sure
    for (i = 0; i < pd->dict_size; i++) {
        pd->dict_table[i] = NULL;
    }
    
    if (!pd->dict_table) {
        kfree(pd);
        return NULL;
    }

    return pd;
}


/** @brief Dictionary descructor; traverses all buckets and linked lists, freeing memory
 *  @param pd  Pointer to a shared dictionary object
 *  @return NULL
 */
static void pyld_dict_destroy(pyld_dict *d)
{
    int i;
    pyld_pair *curr;
    pyld_pair *next;

    for (i = 0; i < d->dict_size; i++)
    {
        for (curr = d->dict_table[i]; curr != NULL; curr = next)
        {
            next = curr->next;
            if (curr->key != NULL) {
                kfree(curr->key);
            }
            if (curr->value != NULL) {
                kfree(curr->value);
            }
            kfree(curr);
        }
    }

    kfree(d->dict_table);
    kfree(d);
}


/** @brief Search for pair with matching key in dict
 *  @param pd  Pointer to a shared dictionary object
 *  @param key  Pointer to key location in memory
 *  @param key_size Size of key, follows sizeof() format with size_t
 *  @return NULL
 */
static void pyld_set(
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

    hash = hash_mem(key, key_size);
    bucket_id = hash % pd->dict_size;

    curr = pd->dict_table[bucket_id];

    while (curr != NULL)
    {   
        if (curr->key_hash == hash)
        {
            kfree(curr->value);
            curr->value = kzalloc(value_size, GFP_KERNEL);
            memcpy(curr->value, value, value_size);
            curr->value_size = value_size;
            curr->value_type = value_type;
            return;
        }
        curr = curr->next;
    }

    pyld_pair *new_entry = kzalloc(1 * sizeof(pyld_pair), GFP_KERNEL);

    new_entry->key_hash         = hash;
    new_entry->key_type         = key_type;
    new_entry->key_size         = key_size;
    new_entry->value_type       = value_type;
    new_entry->value_size       = value_size;
    new_entry->key              = kzalloc(1 * key_size, GFP_KERNEL);
    new_entry->value            = kzalloc(1 * value_size, GFP_KERNEL);
    
    memcpy(new_entry->key, key, key_size);
    memcpy(new_entry->value, value, value_size);


    new_entry->next = pd->dict_table[bucket_id];
    if (new_entry->next == NULL) {
        pr_info("Null is correct in next");
    }
    pd->dict_table[bucket_id] = new_entry;

    if (new_entry->next == NULL) {
        pr_info("Null is correct in next 2");
    }

    if (pd->dict_table[bucket_id] != NULL) {
        pr_info("Table state is correct");
    }

    pd->num_entries++;

    if (pd->num_entries > pd->dict_size * DICT_GROW_DENSITY) {
        pyld_dict_grow(pd);
    }
}


/** @brief Get pair struct by the provided key with provided size
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
    pr_info("PLD_GET: bucket %d", bucket_id);
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

/** @brief Grow dict by DICTSIZE_MULTIPLIER to fit new entires rehash all entries
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
}




/** @brief Implementation pair deletion - delete pair if it exists, else do nothing
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

    // pr_info("DEL: BUCKET %d", bucket_id);

    curr = pd->dict_table[bucket_id];
    head = pd->dict_table[bucket_id];

    while (head) {
        // pr_info("DELL_BUCKET: key %s with size %d", head->key, head->key_size);
        head = head->next;
    }

    // if (curr->key_hash == hash) {
    //     pr_info("DEL: HASH CORRECT FOR SINGLE CASE");
    // } else {
    //     pr_info("DEL: HASH NOT CORRECT");
    //     pr_info("HASH IN PAIR %lu, for key size %d", curr->key_hash, curr->key_size);
    //     pr_info("HASH IN HERE %lu for key size %d", hash, key_size);
    // }

    if (curr == NULL) {
        return;
    }

    // pr_info("DEL: CURR IS NOT NULL");

    if (curr->key_hash == hash) {
        // pr_info("DEL: CASE 2");
        pd->dict_table[bucket_id] = curr->next;
        kfree(curr->key);
        kfree(curr->value);
        kfree(curr);
        goto deleted;
    }

    // pr_info("DEL: CURR IS NOT ONLY THING IN THE BUCKET?");

    prev = curr;
    curr = curr->next;

    while(curr) {
        // pr_info("DEL: CASE 3");
        // pr_info("CURR key is %s", (char *)curr->key);
        if (curr->key_hash == hash) {
            // pr_info("CURR key is %s", (char *)curr->key);
            prev->next = curr->next;
            kfree(curr->key);
            kfree(curr->value);
            kfree(curr);
            // pr_info("DEL: FOUND");
            goto deleted;
        }
        prev = curr;
        curr = curr->next;
    }

    return;

deleted:
    // head = pd->dict_table[bucket_id];
    if (pd->num_entries > 1) {
        pd->num_entries--; 
    }
    return; 
}

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
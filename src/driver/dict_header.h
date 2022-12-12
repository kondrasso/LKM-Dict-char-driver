typedef struct pyld_pair pyld_pair;
typedef struct pyld_dict pyld_dict;

struct pyld_pair
{
    unsigned long key_hash;

    int key_type;
    int value_type;

    size_t key_size;
    size_t value_size;

    void *key;
    void *value;

    pyld_pair *next;
};

struct pyld_dict
{
    int dict_size;
    int num_entries;

    pyld_pair **dict_table;
};


static pyld_dict *pyld_create(void);
static void pyld_dict_destroy(pyld_dict *);
static void pyld_set(pyld_dict *, void *, void *, size_t, size_t, int, int);
static void pyld_del(pyld_dict *, void *, size_t);
static pyld_pair *pyld_get(pyld_dict *, const void *, size_t);
static void pyld_dict_grow(pyld_dict *pd);
static unsigned long hash_mem(const unsigned char *, size_t);
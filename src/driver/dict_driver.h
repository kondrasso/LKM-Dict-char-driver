#define NO_PAIR 420

typedef struct dict_pair dict_pair;
typedef struct dict dict;

struct dict_pair
{
    unsigned long key_hash;

    int key_type;
    int value_type;

    size_t key_size;
    size_t value_size;
    size_t *value_size_adress;

    void *key;
    void *value;

    dict_pair *next;
};

struct dict
{
    int dict_size;
    int num_entries;

    dict_pair **dict_table;
};
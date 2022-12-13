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
#include "dict_interface.h"


int set_pair(int fd, void *key, size_t key_size, int key_type, void* value, size_t value_size, int value_type)
{
    pyld_pair *message;

    if (key == NULL || value == NULL) {
        printf("SET_PAIR: NULL key and values not allowed\n");
        return -1;
    }

    if (key_size == 0 || value_size == 0) {
        printf("SET_PAIR: illegal size\n");
        return -1;
    }

    message = calloc(1, sizeof(pyld_pair));

    if (message == NULL) {
        printf("SET_PAIR: message calloc failed\n");
        return -1;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;
    message->value              = malloc(value_size);

    if (message->value == NULL) {
        printf("SET_PAIR: malloc failed\n");
        return -1;
    }
    
    memcpy(message->value, value, value_size);
    message->value_size         = value_size;
    message->value_type         = value_type;

    if (ioctl(fd, SET_PAIR, message)) {
        printf("SET_PAIR: error setting pair\n");
        free(message->value);
        free(message);
        return -1;
    }
    free(message->value);
    free(message);

    return 0;
}


pyld_pair *get_value(int fd, void *key, size_t key_size, int key_type)
{
    int value_type;
    long value_size;
    pyld_pair *message;

    if (key == NULL) {
        printf("GET_VALUE: NULL key not allowed\n");
        return NULL;
    }

    if (key_size == 0) {
        printf("GET_VALUE: illegal key size\n");
        return NULL;
    }

    message = calloc(1, sizeof(pyld_pair));

    if (message == NULL) {
        printf("GET_VALUE: message calloc failed\n");
        return NULL;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;

    value_size = ioctl(fd, GET_VALUE_SIZE, message);

    if (value_size < 0) {
        printf("GET_VALUE: Could not get size of value\n");
        free(message);
        return NULL;
    }

    value_type = ioctl(fd, GET_VALUE_TYPE, message);

    if (value_type < 0) {
        printf("GET_VALUE: Could not get type of value\n");
        free(message);
        return NULL;
    }

    message->value_size         = (size_t)value_size;
    message->value              = malloc(message->value_size);

    if (message->value == NULL) {
        printf("GET_VALUE: value malloc failed\n");
        free(message);
        return NULL;
    }

    message->value_type         = value_type;

    if (ioctl(fd, GET_VALUE, message)) {
        printf("GET_VALUE: no such key-value pair\n");
        free(message->value);
        free(message);
        return NULL;
    }

    return message;
}


int del_pair(int fd, void *key, size_t key_size, int key_type)
{
    pyld_pair *message;
    long value_size;

    if (key == NULL) {
        printf("DEL_PAIR: NULL key not allowed\n");
        return -1;
    }

    if (key_size == 0) {
        printf("DEL_PAIR: illegal key size\n");
        return -1;
    }


    message = calloc(1, sizeof(pyld_pair));

    if (message == NULL) {
        printf("DEL_PAIR: calloc failed\n");
        return -1;
    }

    message->key                = key;
    message->key_size           = key_size;
    message->key_type           = key_type;

    if (ioctl(fd, DEL_PAIR, message)) {
        printf("DEL_PAIR: could not delete pair\n");
        free(message);
        return -1;
    }
    free(message);
    return 0;
}
char key_0_0[21] = "daxihhexdvxrcsnbacgh\0";
char value_0_0[19] = "targwuwrnhosizayzf\0";
char key_0_1[17] = "kiegykdcmdlltizb\0";
char value_0_1[18] = "rdmcrjutlsgwcbvhy\0";
char key_0_2[15] = "chdmioulfllgvi\0";
char value_0_2[21] = "uctufrxhfomiuwrhvkyy\0";
char key_0_3[11] = "hbzkmicgsw\0";
char value_0_3[16] = "gupmuoeiehxrrix\0";
char key_0_4[20] = "nsmlheqpcybdeufzvnt\0";
char value_0_4[12] = "mmtoqiravxd\0";
char key_0_5[21] = "ryiyukdjnfoaxxiqyfqd\0";
char value_0_5[21] = "juqtgelyfryqatkpadlz\0";
char key_0_6[15] = "hbhsccxpcyryee\0";
char value_0_6[21] = "prfiqtngryxwgwjmvulo\0";
char key_0_7[19] = "odhhckasrhshacwubh\0";
char value_0_7[12] = "bkcqhivpgre\0";
char key_0_8[20] = "sphzpzngddvnlnnoxbv\0";
char value_0_8[21] = "udbmxkzdhggroenfiohc\0";
char key_0_9[18] = "zrdburacyhfnppgmb\0";
char value_0_9[13] = "mamizzojnwxz\0";
int key_1_0[18] = {21689, 23541, 15948, 5073, 6223, 9723, 7134, 31743, 1917, 18979, 24109, 17767, 1998, 24510, 10277, 1874, 1644, 19143};
char value_1_0[18] = "qqfbqcfctcvhmdshs\0";
int key_1_1[19] = {1303, 20296, 2687, 13738, 21541, 19126, 18522, 17131, 10367, 30629, 8545, 6694, 21946, 23469, 10296, 7822, 8704, 12970, 4289};
char value_1_1[21] = "ujokycaotsdcrgqielch\0";
int key_1_2[15] = {9339, 5170, 14359, 27321, 17801, 23054, 9913, 20044, 26448, 21430, 17333, 257, 21885, 26775, 18174};
char value_1_2[15] = "vdeiddxreijtgw\0";
int key_1_3[15] = {6672, 22528, 20783, 27949, 8651, 16562, 16009, 8229, 29667, 29759, 27726, 1665, 3025, 20785, 13880};
char value_1_3[15] = "bakyeuifxorwnr\0";
int key_1_4[10] = {3666, 2466, 30968, 28938, 22644, 29620, 4885, 17878, 1181, 27349};
char value_1_4[16] = "srenebjlzblgvhv\0";
int key_1_5[11] = {11590, 25563, 18347, 28972, 28663, 13317, 31911, 20338, 24559, 5065, 30336};
char value_1_5[14] = "fzzfnafxkznzv\0";
int key_1_6[13] = {8743, 5217, 25801, 22980, 3543, 12536, 28580, 1269, 28135, 15424, 7289, 6540, 26758};
char value_1_6[18] = "ljzhhavgmkicyiluq\0";
int key_1_7[16] = {22267, 27646, 17571, 10852, 30777, 905, 3780, 28739, 31799, 8560, 5852, 19025, 31541, 8699, 1254, 3553};
char value_1_7[20] = "nlxzkntqdmsgibwnaqz\0";
int key_1_8[18] = {22507, 23575, 30811, 24313, 24152, 21976, 6457, 11935, 14133, 2293, 31093, 21766, 30172, 10820, 20420, 10287, 21738, 27788};
char value_1_8[12] = "xjqjvnkmwjr\0";
int key_1_9[12] = {6287, 13778, 21789, 30814, 12424, 22195, 24515, 29584, 5703, 20170, 18649, 9862};
char value_1_9[17] = "rajjgnzstukooovg\0";
int key_2_0[18] = {15506, 26010, 29530, 31452, 26085, 24116, 5561, 21590, 2779, 9300, 16891, 21754, 20741, 20292, 10984, 3061, 26819, 31196};
int value_2_0[13] = {22047, 10172, 7362, 26430, 6526, 4829, 801, 1515, 8024, 15570, 20031, 27845, 25182};
int key_2_1[11] = {14924, 13581, 29037, 20637, 18864, 6372, 23539, 22819, 12583, 16200, 13096};
int value_2_1[13] = {4836, 21498, 22533, 182, 29251, 24608, 28195, 25233, 29015, 3493, 25508, 13932, 7171};
int key_2_2[12] = {26350, 31391, 22804, 16973, 15223, 1646, 18266, 8166, 30064, 27799, 3977, 14958};
int value_2_2[12] = {26261, 15226, 21876, 17405, 18315, 19512, 10398, 31131, 24749, 29211, 14503, 20076};
int key_2_3[18] = {13984, 27217, 29716, 17953, 14612, 29400, 5216, 24369, 28230, 15555, 14748, 8494, 24635, 8102, 27521, 20895, 9087, 25095};
int value_2_3[18] = {15880, 20538, 7840, 8999, 14414, 2539, 23382, 9363, 7684, 8904, 11006, 10477, 29263, 17700, 2641, 4535, 4943, 7578};
int key_2_4[16] = {22740, 5008, 23148, 7011, 2105, 13595, 13357, 10843, 17781, 15268, 13625, 2041, 6778, 27293, 13768, 12763};
int value_2_4[19] = {31001, 22791, 641, 28074, 28858, 25088, 18865, 12465, 15630, 194, 30892, 11527, 9785, 24690, 12780, 27966, 29218, 31243, 27386};
int key_2_5[16] = {17637, 24495, 24073, 17896, 26211, 19768, 29420, 7227, 15999, 7191, 8944, 14282, 15914, 952, 12743, 11015};
int value_2_5[20] = {22255, 26154, 13249, 23730, 5409, 27542, 15316, 30132, 4183, 20391, 17503, 884, 29719, 12912, 19396, 18494, 21726, 889, 2751, 21062};
int key_2_6[16] = {4447, 28405, 15129, 5955, 1648, 8525, 12423, 10727, 6936, 14900, 10711, 11060, 24944, 28825, 12424, 9118};
int value_2_6[16] = {8267, 27359, 2684, 15412, 636, 24545, 17676, 1707, 31193, 11468, 7348, 21304, 2249, 25600, 31369, 21357};
int key_2_7[10] = {24715, 1017, 31117, 8103, 6533, 27500, 668, 20360, 4994, 7817};
int value_2_7[12] = {15518, 21937, 3749, 18481, 31057, 7143, 15239, 22921, 8397, 25130, 12088, 5499};
int key_2_8[19] = {19899, 31580, 24509, 23540, 3754, 25487, 26847, 5367, 31612, 10193, 3543, 18963, 842, 30438, 10223, 18868, 22196, 29755, 31398};
int value_2_8[16] = {12998, 30851, 23430, 6499, 2491, 19402, 22631, 27217, 20554, 7958, 3340, 22846, 25313, 9883, 27863, 22422};
int key_2_9[19] = {26396, 3967, 26091, 18545, 25646, 1346, 11378, 17457, 14038, 21677, 12143, 2260, 16580, 21219, 11182, 415, 27839, 13765, 26947};
int value_2_9[17] = {3459, 14206, 31484, 11869, 20827, 29216, 27156, 15065, 23178, 5014, 14271, 5772, 24046, 17097, 31637, 21315, 8851};

void *test_thread_0(void* fd_0)
{
int fd = (int *)fd_0;
pyld_pair *recieve;
assert(set_pair(fd, key_0_0, sizeof(key_0_0), MY_CHAR, value_0_0, sizeof(value_0_0), MY_CHAR) == 0);
assert(set_pair(fd, key_0_1, sizeof(key_0_1), MY_CHAR, value_0_1, sizeof(value_0_1), MY_CHAR) == 0);
assert(set_pair(fd, key_0_2, sizeof(key_0_2), MY_CHAR, value_0_2, sizeof(value_0_2), MY_CHAR) == 0);
assert(set_pair(fd, key_0_3, sizeof(key_0_3), MY_CHAR, value_0_3, sizeof(value_0_3), MY_CHAR) == 0);
assert(set_pair(fd, key_0_4, sizeof(key_0_4), MY_CHAR, value_0_4, sizeof(value_0_4), MY_CHAR) == 0);
assert(set_pair(fd, key_0_5, sizeof(key_0_5), MY_CHAR, value_0_5, sizeof(value_0_5), MY_CHAR) == 0);
assert(set_pair(fd, key_0_6, sizeof(key_0_6), MY_CHAR, value_0_6, sizeof(value_0_6), MY_CHAR) == 0);
assert(set_pair(fd, key_0_7, sizeof(key_0_7), MY_CHAR, value_0_7, sizeof(value_0_7), MY_CHAR) == 0);
assert(set_pair(fd, key_0_8, sizeof(key_0_8), MY_CHAR, value_0_8, sizeof(value_0_8), MY_CHAR) == 0);
assert(set_pair(fd, key_0_9, sizeof(key_0_9), MY_CHAR, value_0_9, sizeof(value_0_9), MY_CHAR) == 0);
recieve = get_value(fd, key_0_0, sizeof(key_0_0), MY_CHAR);
assert(memcmp(recieve->value, value_0_0, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_1, sizeof(key_0_1), MY_CHAR);
assert(memcmp(recieve->value, value_0_1, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_2, sizeof(key_0_2), MY_CHAR);
assert(memcmp(recieve->value, value_0_2, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_3, sizeof(key_0_3), MY_CHAR);
assert(memcmp(recieve->value, value_0_3, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_4, sizeof(key_0_4), MY_CHAR);
assert(memcmp(recieve->value, value_0_4, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_5, sizeof(key_0_5), MY_CHAR);
assert(memcmp(recieve->value, value_0_5, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_6, sizeof(key_0_6), MY_CHAR);
assert(memcmp(recieve->value, value_0_6, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_7, sizeof(key_0_7), MY_CHAR);
assert(memcmp(recieve->value, value_0_7, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_8, sizeof(key_0_8), MY_CHAR);
assert(memcmp(recieve->value, value_0_8, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_0_9, sizeof(key_0_9), MY_CHAR);
assert(memcmp(recieve->value, value_0_9, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
assert(del_pair(fd, key_0_0, sizeof(key_0_0), MY_CHAR) == 0);
assert(del_pair(fd, key_0_1, sizeof(key_0_1), MY_CHAR) == 0);
assert(del_pair(fd, key_0_2, sizeof(key_0_2), MY_CHAR) == 0);
assert(del_pair(fd, key_0_3, sizeof(key_0_3), MY_CHAR) == 0);
assert(del_pair(fd, key_0_4, sizeof(key_0_4), MY_CHAR) == 0);
assert(del_pair(fd, key_0_5, sizeof(key_0_5), MY_CHAR) == 0);
assert(del_pair(fd, key_0_6, sizeof(key_0_6), MY_CHAR) == 0);
assert(del_pair(fd, key_0_7, sizeof(key_0_7), MY_CHAR) == 0);
assert(del_pair(fd, key_0_8, sizeof(key_0_8), MY_CHAR) == 0);
assert(del_pair(fd, key_0_9, sizeof(key_0_9), MY_CHAR) == 0);
assert(get_value(fd, key_0_0, sizeof(key_0_0), MY_CHAR) == NULL);
assert(get_value(fd, key_0_1, sizeof(key_0_1), MY_CHAR) == NULL);
assert(get_value(fd, key_0_2, sizeof(key_0_2), MY_CHAR) == NULL);
assert(get_value(fd, key_0_3, sizeof(key_0_3), MY_CHAR) == NULL);
assert(get_value(fd, key_0_4, sizeof(key_0_4), MY_CHAR) == NULL);
assert(get_value(fd, key_0_5, sizeof(key_0_5), MY_CHAR) == NULL);
assert(get_value(fd, key_0_6, sizeof(key_0_6), MY_CHAR) == NULL);
assert(get_value(fd, key_0_7, sizeof(key_0_7), MY_CHAR) == NULL);
assert(get_value(fd, key_0_8, sizeof(key_0_8), MY_CHAR) == NULL);
assert(get_value(fd, key_0_9, sizeof(key_0_9), MY_CHAR) == NULL);
}
void *test_thread_1(void* fd_1)
{
int fd = (int *)fd_1;
pyld_pair *recieve;
assert(set_pair(fd, key_1_0, sizeof(key_1_0), MY_INT, value_1_0, sizeof(value_1_0), MY_CHAR) == 0);
assert(set_pair(fd, key_1_1, sizeof(key_1_1), MY_INT, value_1_1, sizeof(value_1_1), MY_CHAR) == 0);
assert(set_pair(fd, key_1_2, sizeof(key_1_2), MY_INT, value_1_2, sizeof(value_1_2), MY_CHAR) == 0);
assert(set_pair(fd, key_1_3, sizeof(key_1_3), MY_INT, value_1_3, sizeof(value_1_3), MY_CHAR) == 0);
assert(set_pair(fd, key_1_4, sizeof(key_1_4), MY_INT, value_1_4, sizeof(value_1_4), MY_CHAR) == 0);
assert(set_pair(fd, key_1_5, sizeof(key_1_5), MY_INT, value_1_5, sizeof(value_1_5), MY_CHAR) == 0);
assert(set_pair(fd, key_1_6, sizeof(key_1_6), MY_INT, value_1_6, sizeof(value_1_6), MY_CHAR) == 0);
assert(set_pair(fd, key_1_7, sizeof(key_1_7), MY_INT, value_1_7, sizeof(value_1_7), MY_CHAR) == 0);
assert(set_pair(fd, key_1_8, sizeof(key_1_8), MY_INT, value_1_8, sizeof(value_1_8), MY_CHAR) == 0);
assert(set_pair(fd, key_1_9, sizeof(key_1_9), MY_INT, value_1_9, sizeof(value_1_9), MY_CHAR) == 0);
recieve = get_value(fd, key_1_0, sizeof(key_1_0), MY_INT);
assert(memcmp(recieve->value, value_1_0, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_1, sizeof(key_1_1), MY_INT);
assert(memcmp(recieve->value, value_1_1, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_2, sizeof(key_1_2), MY_INT);
assert(memcmp(recieve->value, value_1_2, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_3, sizeof(key_1_3), MY_INT);
assert(memcmp(recieve->value, value_1_3, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_4, sizeof(key_1_4), MY_INT);
assert(memcmp(recieve->value, value_1_4, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_5, sizeof(key_1_5), MY_INT);
assert(memcmp(recieve->value, value_1_5, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_6, sizeof(key_1_6), MY_INT);
assert(memcmp(recieve->value, value_1_6, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_7, sizeof(key_1_7), MY_INT);
assert(memcmp(recieve->value, value_1_7, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_8, sizeof(key_1_8), MY_INT);
assert(memcmp(recieve->value, value_1_8, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_1_9, sizeof(key_1_9), MY_INT);
assert(memcmp(recieve->value, value_1_9, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
assert(del_pair(fd, key_1_0, sizeof(key_1_0), MY_INT) == 0);
assert(del_pair(fd, key_1_1, sizeof(key_1_1), MY_INT) == 0);
assert(del_pair(fd, key_1_2, sizeof(key_1_2), MY_INT) == 0);
assert(del_pair(fd, key_1_3, sizeof(key_1_3), MY_INT) == 0);
assert(del_pair(fd, key_1_4, sizeof(key_1_4), MY_INT) == 0);
assert(del_pair(fd, key_1_5, sizeof(key_1_5), MY_INT) == 0);
assert(del_pair(fd, key_1_6, sizeof(key_1_6), MY_INT) == 0);
assert(del_pair(fd, key_1_7, sizeof(key_1_7), MY_INT) == 0);
assert(del_pair(fd, key_1_8, sizeof(key_1_8), MY_INT) == 0);
assert(del_pair(fd, key_1_9, sizeof(key_1_9), MY_INT) == 0);
assert(get_value(fd, key_1_0, sizeof(key_1_0), MY_INT) == NULL);
assert(get_value(fd, key_1_1, sizeof(key_1_1), MY_INT) == NULL);
assert(get_value(fd, key_1_2, sizeof(key_1_2), MY_INT) == NULL);
assert(get_value(fd, key_1_3, sizeof(key_1_3), MY_INT) == NULL);
assert(get_value(fd, key_1_4, sizeof(key_1_4), MY_INT) == NULL);
assert(get_value(fd, key_1_5, sizeof(key_1_5), MY_INT) == NULL);
assert(get_value(fd, key_1_6, sizeof(key_1_6), MY_INT) == NULL);
assert(get_value(fd, key_1_7, sizeof(key_1_7), MY_INT) == NULL);
assert(get_value(fd, key_1_8, sizeof(key_1_8), MY_INT) == NULL);
assert(get_value(fd, key_1_9, sizeof(key_1_9), MY_INT) == NULL);
}
void *test_thread_2(void* fd_2)
{
int fd = (int *)fd_2;
pyld_pair *recieve;
assert(set_pair(fd, key_2_0, sizeof(key_2_0), MY_INT, value_2_0, sizeof(value_2_0), MY_INT) == 0);
assert(set_pair(fd, key_2_1, sizeof(key_2_1), MY_INT, value_2_1, sizeof(value_2_1), MY_INT) == 0);
assert(set_pair(fd, key_2_2, sizeof(key_2_2), MY_INT, value_2_2, sizeof(value_2_2), MY_INT) == 0);
assert(set_pair(fd, key_2_3, sizeof(key_2_3), MY_INT, value_2_3, sizeof(value_2_3), MY_INT) == 0);
assert(set_pair(fd, key_2_4, sizeof(key_2_4), MY_INT, value_2_4, sizeof(value_2_4), MY_INT) == 0);
assert(set_pair(fd, key_2_5, sizeof(key_2_5), MY_INT, value_2_5, sizeof(value_2_5), MY_INT) == 0);
assert(set_pair(fd, key_2_6, sizeof(key_2_6), MY_INT, value_2_6, sizeof(value_2_6), MY_INT) == 0);
assert(set_pair(fd, key_2_7, sizeof(key_2_7), MY_INT, value_2_7, sizeof(value_2_7), MY_INT) == 0);
assert(set_pair(fd, key_2_8, sizeof(key_2_8), MY_INT, value_2_8, sizeof(value_2_8), MY_INT) == 0);
assert(set_pair(fd, key_2_9, sizeof(key_2_9), MY_INT, value_2_9, sizeof(value_2_9), MY_INT) == 0);
recieve = get_value(fd, key_2_0, sizeof(key_2_0), MY_INT);
assert(memcmp(recieve->value, value_2_0, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_1, sizeof(key_2_1), MY_INT);
assert(memcmp(recieve->value, value_2_1, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_2, sizeof(key_2_2), MY_INT);
assert(memcmp(recieve->value, value_2_2, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_3, sizeof(key_2_3), MY_INT);
assert(memcmp(recieve->value, value_2_3, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_4, sizeof(key_2_4), MY_INT);
assert(memcmp(recieve->value, value_2_4, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_5, sizeof(key_2_5), MY_INT);
assert(memcmp(recieve->value, value_2_5, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_6, sizeof(key_2_6), MY_INT);
assert(memcmp(recieve->value, value_2_6, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_7, sizeof(key_2_7), MY_INT);
assert(memcmp(recieve->value, value_2_7, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_8, sizeof(key_2_8), MY_INT);
assert(memcmp(recieve->value, value_2_8, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
recieve = get_value(fd, key_2_9, sizeof(key_2_9), MY_INT);
assert(memcmp(recieve->value, value_2_9, recieve->value_size) == 0);
free(recieve->value);
free(recieve);
assert(del_pair(fd, key_2_0, sizeof(key_2_0), MY_INT) == 0);
assert(del_pair(fd, key_2_1, sizeof(key_2_1), MY_INT) == 0);
assert(del_pair(fd, key_2_2, sizeof(key_2_2), MY_INT) == 0);
assert(del_pair(fd, key_2_3, sizeof(key_2_3), MY_INT) == 0);
assert(del_pair(fd, key_2_4, sizeof(key_2_4), MY_INT) == 0);
assert(del_pair(fd, key_2_5, sizeof(key_2_5), MY_INT) == 0);
assert(del_pair(fd, key_2_6, sizeof(key_2_6), MY_INT) == 0);
assert(del_pair(fd, key_2_7, sizeof(key_2_7), MY_INT) == 0);
assert(del_pair(fd, key_2_8, sizeof(key_2_8), MY_INT) == 0);
assert(del_pair(fd, key_2_9, sizeof(key_2_9), MY_INT) == 0);
assert(get_value(fd, key_2_0, sizeof(key_2_0), MY_INT) == NULL);
assert(get_value(fd, key_2_1, sizeof(key_2_1), MY_INT) == NULL);
assert(get_value(fd, key_2_2, sizeof(key_2_2), MY_INT) == NULL);
assert(get_value(fd, key_2_3, sizeof(key_2_3), MY_INT) == NULL);
assert(get_value(fd, key_2_4, sizeof(key_2_4), MY_INT) == NULL);
assert(get_value(fd, key_2_5, sizeof(key_2_5), MY_INT) == NULL);
assert(get_value(fd, key_2_6, sizeof(key_2_6), MY_INT) == NULL);
assert(get_value(fd, key_2_7, sizeof(key_2_7), MY_INT) == NULL);
assert(get_value(fd, key_2_8, sizeof(key_2_8), MY_INT) == NULL);
assert(get_value(fd, key_2_9, sizeof(key_2_9), MY_INT) == NULL);
}
int main()
{
int fd;
fd = open(DEVICE_PATH, O_RDWR);
pthread_t thread_0, thread_1, thread_2;
pthread_create(&thread_0, NULL, test_thread_0, fd);
pthread_create(&thread_1, NULL, test_thread_1, fd);
pthread_create(&thread_2, NULL, test_thread_2, fd);
pthread_join(thread_0, NULL);;
pthread_join(thread_1, NULL);;
pthread_join(thread_2, NULL);;
}

import os
import random
import string
import argparse

SEED = 42
TYPE_UNPACK_DICT = {"MY_CHAR": "char", "MY_INT": "int"}
BASE_DIR = os.path.dirname( __file__ )
FUNCTION_HEAD_PATH = os.path.join(BASE_DIR, "func_head.c")
DEFAULT_TEST_LOCATION = os.path.join(os.path.split(BASE_DIR)[0], "src", "client/")

def gen_random_ascii_sting(min_len: int, max_len: int) -> tuple[str, int]:
    str_len = random.randint(min_len, max_len)
    random_str = '"' + "".join(
        random.choice(string.ascii_lowercase) for _ in range(str_len)
    )
    random_str += r"\0" + '"'
    return random_str, str_len


def gen_random_int_array(min_len: int, max_len: int) -> tuple[str, int]:
    arr_len = random.randint(min_len, max_len)
    random_array = "{" + "".join(
        "{}, ".format(random.randint(1, 32000)) for _ in range(arr_len)
    )
    random_array = random_array[:-2] + "}"
    return random_array, arr_len


GENERATOR_FUN_DICT = {
    "MY_CHAR": gen_random_ascii_sting,
    "MY_INT": gen_random_int_array,
}


def gen_key_value_pairs(
    name_prefix: int,
    number_of_pairs: int,
    key_min_len: int,
    key_max_len: int,
    value_min_len: int,
    value_max_len: int,
    key_type: str = "MY_CHAR",
    value_type: str = "MY_CHAR",
) -> dict[str : dict[str : str | int]]:

    key_value_pairs = {}

    for i in range(number_of_pairs):
        key, key_len = GENERATOR_FUN_DICT[key_type](key_min_len, key_max_len)
        value, value_len = GENERATOR_FUN_DICT[value_type](value_min_len, value_max_len)

        # use thread number as name prefix to get unique names for each thread
        key_value_pairs[key] = {
            "value": value,
            "key_len": key_len,
            "value_len": value_len,
            "key_type": key_type,
            "value_type": value_type,
            "key_declared_name": f"key_{name_prefix}_{i}",
            "value_declared_name": f"value_{name_prefix}_{i}",
        }

    # correct len for null-terminated charachter for MY_CHAR
    for value in key_value_pairs.values():
        if value["key_type"] == "MY_CHAR":
            value["key_len"] += 1
        if value["value_type"] == "MY_CHAR":
            value["value_len"] += 1

    return key_value_pairs


def unpack_for_key(
    key: str, pair_dict: dict[str : dict[str : str | int]]
) -> tuple[str]:
    key_name = pair_dict[key]["key_declared_name"]
    key_len = pair_dict[key]["key_len"]
    value = pair_dict[key]["value"]
    value_name = pair_dict[key]["value_declared_name"]
    value_len = pair_dict[key]["value_len"]
    key_type = pair_dict[key]["key_type"]
    value_type = pair_dict[key]["value_type"]
    return key_name, key_len, value, value_name, value_len, key_type, value_type


def gen_declarations(
    key_list: list[str],
    pair_dict: dict[str : dict[str : str | int]],
) -> list[str]:

    decl_list = []

    for key in key_list:

        (
            key_name,
            key_len,
            value,
            value_name,
            value_len,
            key_type,
            value_type,
        ) = unpack_for_key(key, pair_dict)

        decl_list.append(
            f"{TYPE_UNPACK_DICT[key_type]} {key_name}[{key_len}] = {key};\n"
        )

        decl_list.append(
            f"{TYPE_UNPACK_DICT[value_type]} {value_name}[{value_len}] = {value};\n"
        )

    return decl_list


def gen_set_asserts(
    key_list: list[str], pair_dict: dict[str : str | int]
) -> list[str]:

    set_assert_list = []

    for key in key_list:

        key_name, _, _, value_name, _, key_type, value_type = unpack_for_key(
            key, pair_dict
        )

        set_assert_list.append(
            f"assert(set_pair(fd, {key_name}, sizeof({key_name}), {key_type}, {value_name}, sizeof({value_name}), {value_type}) == 0);\n"
        )

    return set_assert_list


def gen_get_assert(
    key_list: list[str], pair_dict: dict[str : str | int], deleted: bool = False
) -> list[str]:

    get_assert_list = []

    if deleted:
        for key in key_list:

            key_name, _, _, value_name, _, key_type, _ = unpack_for_key(key, pair_dict)

            get_assert_list.append(
                f"assert(get_value(fd, {key_name}, sizeof({key_name}), {key_type}) == NULL);\n"
            )

    else:
        for key in key_list:

            key_name, _, _, value_name, _, key_type, _ = unpack_for_key(key, pair_dict)

            get_assert_list.append(
                f"recieve = get_value(fd, {key_name}, sizeof({key_name}), {key_type});\n"
            )

            get_assert_list.append(
                f"assert(memcmp(recieve->value, {value_name}, recieve->value_size) == 0);\n"
            )

            get_assert_list.append(
                f"free(recieve->value);\n"
            )

            get_assert_list.append(
                f"free(recieve);\n"
            )

    return get_assert_list


def gen_del_asserts(
    key_list: list[str], pair_dict: dict[str : str | int]
) -> list[str]:

    del_assert_list = []

    for key in key_list:

        key_name, _, _, _, _, key_type, _ = unpack_for_key(key, pair_dict)

        del_assert_list.append(
            f"assert(del_pair(fd, {key_name}, sizeof({key_name}), {key_type}) == 0);\n"
        )

    return del_assert_list


def gen_thread_pairs(experiment_params: list[tuple[int | str]]):
    return {
        f"thread_{idx}": gen_key_value_pairs(idx, *param_set)
        for idx, param_set in enumerate(experiment_params)
    }


def tid_helper(idx: int, threads: list):
    max = len(threads)
    idx += 1
    if idx >= max:
        idx = 0
    return idx


def gen_thread_asserts(
    thread_pairs_dict: dict[str : dict[str : dict[str : str | int]]],
    deletion: bool = False,
    num_of_entries: int = 10
) -> tuple[list[str], dict[str : list[str]]]:

    thread_asserts = {}
    declarations = []
    threads = list(thread_pairs_dict.keys())
    sleep = max(1, num_of_entries // 10)
    
    # order here - for single thread generate declarations, set them all, check if they are set via get, delete them, check if they are deleted
    for thread in threads:
        key_list = list(thread_pairs_dict[thread].keys())
        thread_asserts[thread] = []
        declarations.extend(gen_declarations(key_list, thread_pairs_dict[thread]))
        thread_asserts[thread].extend(
            gen_set_asserts(key_list, thread_pairs_dict[thread])
        )
        thread_asserts[thread].extend(
            gen_get_assert(key_list, thread_pairs_dict[thread])
        )
        thread_asserts[thread].extend(
            gen_del_asserts(key_list, thread_pairs_dict[thread])
        )
        thread_asserts[thread].extend(
            gen_get_assert(key_list, thread_pairs_dict[thread], deleted=True)
        )
        if deletion:
            thread_asserts[thread].extend(
                gen_set_asserts(key_list, thread_pairs_dict[thread])
            )

    if deletion:
        # there are some limitations in this kind of test for deletion due to single mutex lock, so add some sleep
        for thread in threads:
            thread_asserts[thread].append(f"sleep({sleep});\n")

        # declare next's thread key-value pairs and get that was setted in other threads, then delete
        for idx, thread in enumerate(threads):
            new_idx = tid_helper(idx, threads)
            key_list = list(thread_pairs_dict[threads[new_idx]].keys())
            thread_asserts[thread].extend(
                gen_get_assert(key_list, thread_pairs_dict[threads[new_idx]])
            )
        
        for thread in threads:
            thread_asserts[thread].append(f"sleep({sleep});\n")

        for idx, thread in enumerate(threads):
            new_idx = tid_helper(idx, threads)
            key_list = list(thread_pairs_dict[threads[new_idx]].keys())
            thread_asserts[thread].extend(
                gen_del_asserts(key_list, thread_pairs_dict[threads[new_idx]])
            )

        
        for thread in threads:
            thread_asserts[thread].append(f"sleep({sleep});\n")

        # check if all of them was deleted
        for thread in threads:
            key_list = list(thread_pairs_dict[thread].keys())
            thread_asserts[thread].extend(
                gen_get_assert(key_list, thread_pairs_dict[thread], deleted=True)
            )

    return declarations, thread_asserts


def gen_function_header(thread_name: str) -> list[str]:
    function_header = [
        f"void *test_{thread_name}(void* fd_{thread_name[-1]})\n",
        "{\n",
        f"int fd = (int *)fd_{thread_name[-1]};\n",
        "pyld_pair *recieve;\n",
    ]
    return function_header


def gen_main(threads: list[str]) -> list [str]:
    main_body = [
        "int main()\n",
        "{\n",
        "int fd;\n",
        "fd = open(DEVICE_PATH, O_RDWR);\n"
    ]
    
    thread_decl_str = ''.join(f"{thread}, " for thread in threads)
    thread_decl_str = thread_decl_str[:-2]

    main_body.append(
        "pthread_t " + thread_decl_str + ";\n"
    )

    for thread in threads:
        main_body.append(
            f"pthread_create(&{thread}, NULL, test_{thread}, fd);\n"
        )

    for thread in threads:
        main_body.append(
            f"pthread_join({thread}, NULL);;\n"
        )

    main_body.append("}\n")
    return main_body


def file_writer(
    file_path: str, thread_asserts: dict[str : list[str]], declarations: list[str]
):
    # read head of test file
    with open(FUNCTION_HEAD_PATH, "r") as h_f: 
        function_head = h_f.readlines()

    #generate main    
    main_function = gen_main(list(thread_asserts.keys()))
    
    # write it all down
    with open(file_path, "w+") as f:

        f.writelines(function_head)
        f.writelines(declarations)
        f.writelines("\n")

        for thread, asserts in thread_asserts.items():
            f.writelines(gen_function_header(thread))
            f.writelines(asserts)
            f.writelines("}\n")
        f.writelines(main_function)


def main():
    parser = argparse.ArgumentParser(description='Input test parameters')
    parser.add_argument('--name', type=str, default="test_small.c", help="Name of test")
    parser.add_argument('--np', type=int, default=20, help="Number of pairs to use in test")
    parser.add_argument('--maxlen', type=int, default=20, help="Max lenght of key/value")
    parser.add_argument('--minlen', type=int, default=5, help="Min lenght of key/value")
    parser.add_argument('--deleted', type=int, default=0, help="Use deleted scenario - 1, do not use - 0")
    parser.add_argument('--seed', type=int, default=SEED, help="Seed to use for random generation")

    args = parser.parse_args()
    
    if args.name.endswith(".c"):
        cleared_name = args.name
    else:
        cleared_name = args.name + ".c"

    if args.deleted == 0:
        deleted = False
    else:
        deleted = True


    random.seed(SEED)
    thread_params = [
        (args.np, args.minlen, args.maxlen, args.minlen, args.maxlen, "MY_CHAR", "MY_CHAR"),
        (args.np, args.minlen, args.maxlen, args.minlen, args.maxlen, "MY_INT", "MY_CHAR"),
        (args.np, args.minlen, args.maxlen, args.minlen, args.maxlen, "MY_INT", "MY_INT"),
    ]

    thread_pairs = gen_thread_pairs(thread_params)
    declarations, thread_asserts = gen_thread_asserts(thread_pairs, deleted, args.np)
    file_writer(os.path.join(DEFAULT_TEST_LOCATION, cleared_name), thread_asserts, declarations)



if __name__ == "__main__":
    main()

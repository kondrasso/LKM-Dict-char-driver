import random
import string
from pprint import pprint

SEED = 42
TYPE_UNPACK_DICT = {"MY_CHAR": "char", "MY_INT": "int"}
FUNCTION_HEAD_PATH = "func_head.txt"

def generate_random_ascii_sting(min_len: int, max_len: int) -> tuple[str, int]:
    str_len = random.randint(min_len, max_len)
    random_str = '"' + "".join(
        random.choice(string.ascii_lowercase) for _ in range(str_len)
    )
    random_str += r"\0" + '"'
    return random_str, str_len


def generate_random_int_array(min_len: int, max_len: int) -> tuple[str, int]:
    arr_len = random.randint(min_len, max_len)
    random_array = "{" + "".join(
        "{}, ".format(random.randint(1, 32000)) for _ in range(arr_len)
    )
    random_array = random_array[:-2] + "}"
    return random_array, arr_len


GENERATOR_FUN_DICT = {
    "MY_CHAR": generate_random_ascii_sting,
    "MY_INT": generate_random_int_array,
}


def generate_key_value_pairs(
    name_prefix: int,
    number_of_pairs: int,
    key_min_len: int,
    key_max_len: int,
    value_min_len: int,
    value_max_len: int,
    key_type: str = "MY_CHAR",
    value_type: str = "MY_CHAR",
) -> dict[str : dict[str : str | bool | int]]:

    key_value_pairs = {}

    for i in range(number_of_pairs):
        key, key_len = GENERATOR_FUN_DICT[key_type](key_min_len, key_max_len)
        value, value_len = GENERATOR_FUN_DICT[value_type](value_min_len, value_max_len)

        key_value_pairs[key] = {
            "value": value,
            "key_len": key_len,
            "value_len": value_len,
            "key_type": key_type,
            "value_type": value_type,
            "key_declared_name": f"key_{name_prefix}_{i}",
            "value_declared_name": f"value_{name_prefix}_{i}",
            "is_set": False,
            "is_deleted": False,
        }

    # correct len for null-terminated charachter for CHAR
    for value in key_value_pairs.values():
        if value["key_type"] == "MY_CHAR":
            value["key_len"] += 1
        if value["value_type"] == "MY_CHAR":
            value["value_len"] += 1

    return key_value_pairs


def unpack_for_key(
    key: str, pair_dict: dict[str : dict[str : str | bool | int]]
) -> tuple[str]:
    key_name = pair_dict[key]["key_declared_name"]
    key_len = pair_dict[key]["key_len"]
    value = pair_dict[key]["value"]
    value_name = pair_dict[key]["value_declared_name"]
    value_len = pair_dict[key]["value_len"]
    key_type = pair_dict[key]["key_type"]
    value_type = pair_dict[key]["value_type"]
    return key_name, key_len, value, value_name, value_len, key_type, value_type


def generate_declarations(
    key_list: list[str],
    pair_dict: dict[str : dict[str : str | bool | int]],
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


def generate_set_asserts(
    key_list: list[str], pair_dict: dict[str : str | bool | int]
) -> list[str]:

    set_assert_list = []

    for key in key_list:

        key_name, _, _, value_name, _, key_type, value_type = unpack_for_key(
            key, pair_dict
        )

        set_assert_list.append(
            f"assert(set_pair(fd, {key_name}, sizeof({key_name}), {key_type}, {value_name}, sizeof({value_name}), {value_type}) != NULL);\n"
        )

    return set_assert_list


def generate_get_asserts(
    key_list: list[str], pair_dict: dict[str : str | bool | int], deleted: bool = False
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

    return get_assert_list


def generate_del_asserts(
    key_list: list[str], pair_dict: dict[str : str | bool | int]
) -> list[str]:

    del_assert_list = []

    for key in key_list:

        key_name, _, _, _, _, key_type, _ = unpack_for_key(key, pair_dict)

        del_assert_list.append(
            f"assert(del_pair(fd, {key_name}, sizeof({key_name}), {key_type}) == 0);\n"
        )

    return del_assert_list


def generate_pairs_for_threads(experiment_params: list[tuple[int | str]]):
    return {
        f"thread_{idx}": generate_key_value_pairs(idx, *param_set)
        for idx, param_set in enumerate(experiment_params)
    }


def tid_helper(idx: int, threads: list):
    max = len(threads)
    idx += 1
    if idx >= max:
        idx = 0
    return idx


def construct_thread_asserts(
    thread_pairs_dict: dict[str : dict[str : dict[str : str | bool | int]]],
    deletion: bool = True,
    num_of_entries: int = 10
) -> tuple[list[str], dict[str : list[str]]]:

    thread_asserts = {}
    declarations = []
    threads = list(thread_pairs_dict.keys())
    sleep = max(1, num_of_entries // 10)
    
    # order here - for single thread generate declarations, set them all to dict, get them, delete them, check if they are deleted
    for thread in threads:
        key_list = list(thread_pairs_dict[thread].keys())
        thread_asserts[thread] = []
        declarations.extend(generate_declarations(key_list, thread_pairs_dict[thread]))
        thread_asserts[thread].extend(
            generate_set_asserts(key_list, thread_pairs_dict[thread])
        )
        thread_asserts[thread].extend(
            generate_get_asserts(key_list, thread_pairs_dict[thread])
        )
        thread_asserts[thread].extend(
            generate_del_asserts(key_list, thread_pairs_dict[thread])
        )
        thread_asserts[thread].extend(
            generate_get_asserts(key_list, thread_pairs_dict[thread], deleted=True)
        )
        if deletion:
            thread_asserts[thread].extend(
                generate_set_asserts(key_list, thread_pairs_dict[thread])
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
                generate_get_asserts(key_list, thread_pairs_dict[threads[new_idx]])
            )
        
        for thread in threads:
            thread_asserts[thread].append(f"sleep({sleep});\n")

        for idx, thread in enumerate(threads):
            new_idx = tid_helper(idx, threads)
            key_list = list(thread_pairs_dict[threads[new_idx]].keys())
            thread_asserts[thread].extend(
                generate_del_asserts(key_list, thread_pairs_dict[threads[new_idx]])
            )

        
        for thread in threads:
            thread_asserts[thread].append(f"sleep({sleep});\n")

        # check if all of them was deleted
        for thread in threads:
            key_list = list(thread_pairs_dict[thread].keys())
            thread_asserts[thread].extend(
                generate_get_asserts(key_list, thread_pairs_dict[thread], deleted=True)
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
    random.seed(SEED)
    thread_params = [
        (10000, 5, 40, 5, 40, "MY_CHAR", "MY_CHAR"),
        (10000, 5, 40, 5, 40, "MY_INT", "MY_CHAR"),
        (10000, 5, 40, 5, 40, "MY_INT", "MY_INT"),
    ]

    thread_pairs = generate_pairs_for_threads(thread_params)
    declarations, thread_asserts = construct_thread_asserts(thread_pairs)
    file_writer("text.txt", thread_asserts, declarations)
    # test_tuple = ( 20, 10, 20, 10, 20, "MY_INT", "MY_CHAR")
    # pair_dict = generate_key_value_pairs(1, *test_tuple)
    # key_list = list(pair_dict.keys())
    # key_list_verify_del = random.sample(key_list, 10)
    # pprint(generate_declarations(key_list, pair_dict))
    # pprint(generate_set_asserts(key_list, pair_dict))

    # with open("text.txt", 'w+') as f:
    #     f.writelines(generate_declarations(key_list, pair_dict))
    #     f.writelines(generate_set_asserts(key_list, pair_dict))
    #     f.writelines(generate_get_asserts(key_list, pair_dict))
    #     f.writelines(generate_del_asserts(key_list_verify_del, pair_dict))
    #     f.writelines(generate_get_asserts(key_list_verify_del, pair_dict, True))


if __name__ == "__main__":
    main()
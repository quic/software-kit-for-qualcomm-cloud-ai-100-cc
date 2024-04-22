## Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
## SPDX-License-Identifier: BSD-3-Clause-Clear

# goal : generate struct AICExecContext_ from the input path and write it to the output path using fire

import fire, pathlib, datetime

POINTER_TYPES = [
    "uint32_t *", "uint64_t *", "uint16_t *", "uint8_t *", "void **",
    "nnc_log_fp", "nnc_exit_fp", "nnc_pmu_set", "nnc_err_fatal_fp",
    "nnc_notify_hang_fp", "nnc_udma_read_fp", "nnc_mmap_fp", "nnc_munmap_fp",
    "nnc_pmu_get", "SemaphoreInfo *", "nnc_reprog_mcid_fp", "dlopen_fp",
    "dlopenbuf_fp", "dlclose_fp", "dlsym_fp", "dladdr_fp", "dlerror_fp",
    "dlinfo_fp"
]


def load_string(filepath):
    try:
        f = open(filepath, 'r')
        data = f.read()
        return data
    except:
        raise ValueError(f"Could not open file {filepath}")


def save_string(filepath, string):
    try:
        f = open(filepath, 'w')
        f.write(string)
    except:
        raise ValueError(f"Could not write to file {filepath}")


#extract the struct inside "typedef struct AICExecContext_ { } AICExecContext;"
def extract_struct(instring):
    #using partition to split the string into 3 parts
    #the separator is the string "typedef struct AICExecContext_ {"
    #the first part is the string before the struct
    #the second part is the struct
    #the third part is the string after the struct
    front_separator = "typedef struct AICExecContext_ {"
    structWithTail = instring.partition(front_separator)[2]
    rear_separator = "} AICExecContext;"
    struct = front_separator + structWithTail.partition(
        rear_separator)[0] + rear_separator
    #rename AICExecContext_ to AICExecContext_32bitPointers
    struct = struct.replace("AICExecContext_", "AICExecContext_32bitPointers")
    struct = struct.replace("AICExecContext;", "AICExecContext32bitPointers;")
    return struct


def substitutePointerTypes(struct, destination_type):
    for pointer_type in POINTER_TYPES:
        struct = struct.replace(pointer_type, destination_type + " ")
    return struct


#from the output filename, generate include guards and return head, tail as a tuple
def generateIncludeGuards(outputpath):
    header_guard_name = str(outputpath.name).upper().replace(".", "_")
    header_guard = f"\n#ifndef {header_guard_name}\n#define {header_guard_name}\n\n"
    footer_guard = f"\n\n#endif // {header_guard_name}"
    return header_guard, footer_guard

def get_copyright_notice():
    current_year = str(datetime.datetime.now().year)
    copyright_notice = "/*\n" \
    " * Copyright (c) 2021-" + current_year + " Qualcomm Innovation Center, Inc. All rights reserved.\n" \
    " * SPDX-License-Identifier: BSD-3-Clause-Clear\n" \
    " */\n"
    return copyright_notice

def generateExecContextHeader(
        inputpath="../../AICMetadataExecCtx.h",
        outputpath="../deps-generated/execContextGenerated_32bitPointers.h",
        pointer_type="uint32_t"):
    # check if the input and output paths are valid with pathlib
    inputpath = pathlib.Path(inputpath)
    if not inputpath.exists():
        raise ValueError(f"Input path {inputpath} does not exist")
    outputpath = pathlib.Path(outputpath)
    if not outputpath.parent.exists():
        raise ValueError(f"Output path {outputpath} does not exist")
    infile = load_string(inputpath)
    extracted_struct = extract_struct(infile)
    completedstruct = substitutePointerTypes(extracted_struct, pointer_type)
    header_guard, footer_guard = generateIncludeGuards(outputpath)
    completefile = get_copyright_notice(
    ) + header_guard + completedstruct + footer_guard
    save_string(outputpath, completefile)


if __name__ == '__main__':
    fire.Fire()

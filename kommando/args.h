#pragma once
#include "types.h"

kommando_result kommando_parse(kommando_flag* flags, size_t flagCount,
                               kommando_positional* positionals, size_t posCount,
                               int argc, const char** argv);

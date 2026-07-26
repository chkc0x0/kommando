#pragma once
#include "types.h"

kommando_result kommando_flags_parse(kommando_flag* flags, size_t count,
                                     int argc, const char** argv);

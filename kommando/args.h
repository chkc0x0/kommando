#pragma once
#include "types.h"

int kommando_flags_parse(kommando_flag* flags, size_t flagCount, int argc,
						 const char** argv);
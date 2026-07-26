#pragma once
#include <stddef.h>
#include "types.h"

void kommando_list_create(kommando_list* list, size_t elemSize);
kommando_result kommando_list_add(kommando_list* list, const void* elem);
void kommando_list_destroy(kommando_list* list);

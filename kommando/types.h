#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct kommando_ctx_t kommando_ctx;

typedef enum
{
	KOMMANDO_OK,
	KOMMANDO_ERR_OOM,
	KOMMANDO_ERR_ARG_PARSE,
	KOMMANDO_ERR_INVALID_VALUE,
	KOMMANDO_HANDLED
} kommando_result;

// should this be opaque?
// of course, what would it be otherwise?
typedef struct
{
	void* data;
	size_t size;
	size_t cap;
	size_t elem_size;
} kommando_list;
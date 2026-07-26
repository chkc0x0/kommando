#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct kommando_ctx_t kommando_ctx;

typedef enum
{
	KOMMANDO_OK,
	KOMMANDO_ERR_UNKNOWN_FLAG,
	KOMMANDO_ERR_UNEXPECTED_VALUE,
	KOMMANDO_ERR_MISSING_VALUE,
	KOMMANDO_ERR_TOO_MANY_ARGS,
	KOMMANDO_ERR_MISSING_FLAG,
	KOMMANDO_ERR_MISSING_POSITIONAL,
	KOMMANDO_ERR_UNKNOWN_CMD
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
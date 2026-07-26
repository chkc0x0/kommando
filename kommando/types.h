#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum
{
	KOMMANDO_FLAG_BOOL,
	KOMMANDO_FLAG_STRING,
	KOMMANDO_FLAG_INT,

	// this implies repeatable
	KOMMANDO_FLAG_INT_LIST,
	KOMMANDO_FLAG_STRING_LIST
} kommando_flag_type;

typedef enum
{
	KOMMANDO_OK,
	KOMMANDO_ERR_UNKNOWN_FLAG,
	KOMMANDO_ERR_UNEXPECTED_VALUE,
	KOMMANDO_ERR_MISSING_VALUE,
	KOMMANDO_ERR_TOO_MANY_ARGS,
	KOMMANDO_ERR_MISSING_FLAG,
	KOMMANDO_ERR_MISSING_POSITIONAL
} kommando_result;

typedef struct
{
	const char* long_name;
	char short_name;
	kommando_flag_type type;
	void* target;
	const void* default_val;
	const char* help;
	bool required;
} kommando_flag;

typedef struct
{
	const char* name;
	const char* help;
	kommando_flag_type type;
	void* target;
	bool required;
} kommando_positional;

// should this be opaque?
// of course, what would it be otherwise?
typedef struct
{
	void* data;
	size_t size;
	size_t cap;
	size_t elem_size;
} kommando_list;
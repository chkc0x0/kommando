#pragma once
#include "types.h"
#include <stddef.h>

#define ko_arg_member(type, field) offsetof(type, field)
#define ko_offset_none ((size_t)-1)

typedef enum
{
	KOMMANDO_FLAG_BOOL,
	KOMMANDO_FLAG_STRING,
	KOMMANDO_FLAG_INT,
	KOMMANDO_FLAG_COUNT,

	// this implies repeatable
	KOMMANDO_FLAG_INT_LIST,
	KOMMANDO_FLAG_STRING_LIST
} kommando_flag_type;

typedef enum
{
	KOMMANDO_ARG_ERR_NONE = 0,
	KOMMANDO_ARG_ERR_UNKNOWN_FLAG,
	KOMMANDO_ARG_ERR_MISSING_VALUE,
	KOMMANDO_ARG_ERR_MISSING_FLAG,
	KOMMANDO_ARG_ERR_MISSING_POSITIONAL,
	KOMMANDO_ARG_ERR_TOO_MANY_ARGS,
	KOMMANDO_ARG_ERR_UNKNOWN_CMD,
	KOMMANDO_ARG_ERR_INVALID_VALUE,
	KOMMANDO_ARG_ERR_VALIDATION_FAILED,
	KOMMANDO_ARG_ERR_OOM,
} kommando_arg_error;

typedef struct
{
	kommando_arg_error error;
	const char* flag_name;
	const char* offending_value;
} kommando_arg_err_info;

struct kommando_cmd;

typedef void (*kommando_arg_err_handler)(struct kommando_cmd* cmd,
										 const kommando_arg_err_info* info);

typedef struct
{
	const char* long_name;
	char short_name;
	kommando_flag_type type;
	size_t target_offset;
	const void* default_val;
	const char* help;
	bool required;
} kommando_flag;

typedef struct
{
	const char* name;
	const char* help;
	kommando_flag_type type;
	size_t target_offset;
	bool required;
	size_t min_count;
	size_t max_count;
} kommando_positional;

typedef struct kommando_cmd
{
	const char* name;
	const char* help;

	kommando_flag* flags;
	size_t flag_count;

	kommando_positional* positionals;
	size_t positional_count;

	struct kommando_cmd* subcommands;
	size_t subcommand_count;

	int (*handler)(struct kommando_cmd* cmd);
	void* user_data;
	struct kommando_cmd* parent;

	kommando_arg_err_handler on_error;
} kommando_cmd;

// populate command's parent fields
void kommando_cmd_finalize(kommando_cmd* cmd);

kommando_result kommando_parse(kommando_cmd* cmd, int argc, const char** argv);
kommando_result kommando_parse_nodispatch(kommando_cmd* cmd, kommando_cmd** leaf,
										  int argc, const char** argv);

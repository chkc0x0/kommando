#pragma once
#include "types.h"

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
} kommando_cmd;

// populate command's parent fields
void kommando_cmd_finalize(kommando_cmd* cmd);

kommando_result kommando_parse(kommando_cmd* cmd, int argc, const char** argv);
kommando_result kommando_parse_nodispatch(kommando_cmd* cmd, kommando_cmd** leaf,
										  int argc, const char** argv);

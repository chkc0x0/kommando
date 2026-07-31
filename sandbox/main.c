#include "kommando/args.h"
#include <stdio.h>

typedef struct
{
	const char* name;
	int times;
	bool verbose;
	bool verbose2;
	bool verbose3;
	int queue;
} greet_opts_t;

static greet_opts_t opts = {nullptr};

static kommando_flag say_flags[] = {
	{
		.long_name = "name",
		.short_name = 'n',
		.type = KOMMANDO_FLAG_STRING,
		.target_offset = ko_arg_member(greet_opts_t, name),
		.default_val = (void*)&(const char*){"world"},
		.help = "who to greet",
	},
	{
		.long_name = "times",
		.short_name = 't',
		.type = KOMMANDO_FLAG_INT,
		.target_offset = ko_arg_member(greet_opts_t, times),
		.default_val = &(int){1},
		.help = "how many times to repeat",
	},
	{
		.long_name = "verbose",
		.short_name = 'v',
		.type = KOMMANDO_FLAG_BOOL,
		.target_offset = ko_arg_member(greet_opts_t, verbose),
		.help = "print extra output",
	},
	{
		.long_name = "verbose",
		.short_name = 'z',
		.type = KOMMANDO_FLAG_BOOL,
		.target_offset = ko_arg_member(greet_opts_t, verbose2),
		.help = "print extra output",
	},
	{
		.long_name = "verbose",
		.short_name = 'x',
		.type = KOMMANDO_FLAG_BOOL,
		.target_offset = ko_arg_member(greet_opts_t, verbose3),
		.help = "print extra output",
	},
	{
		.long_name = "verbose",
		.short_name = 'q',
		.type = KOMMANDO_FLAG_COUNT,
		.target_offset = ko_arg_member(greet_opts_t, queue),
		.help = "print extra output",
	},
};

static kommando_cmd subs[] = {
	{
		.name = "say",
		.help = "from flags",
		.flags = say_flags,
		.flag_count = 3,
		.user_data = &opts,
	},
	{
		.name = "ask",
		.help = "ask for name",
	},
};

static kommando_cmd root = {
	.name = "greet",
	.help = "for greeting",
	.flags = say_flags + 3,
	.flag_count = 3,
	.subcommands = subs,
	.subcommand_count = 2,
	.user_data = &opts,
};

int main(int argc, const char** argv)
{
	kommando_cmd_finalize(&root);
	kommando_result r = kommando_parse(&root, argc, argv);

	printf("%s %d %d %d %d %d\n", opts.name, opts.times, (int)opts.verbose, (int)opts.verbose2,
		   (int)opts.verbose3, opts.queue);
	return r;
}

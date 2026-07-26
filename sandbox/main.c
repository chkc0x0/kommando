#include "args.h"
#include "kommando/types.h"

typedef struct
{
	const char* name;
	int times;
	bool verbose;
} greet_opts;

static greet_opts opts = {0};

static kommando_flag flags[] = {
	{
		.long_name = "name",
		.short_name = 'n',
		.type = KOMMANDO_FLAG_STRING,
		.target = (void*)&opts.name,
		.default_val = (void*)&(const char*){"world"},
		.help = "who to greet",
	},
	{
		.long_name = "times",
		.short_name = 't',
		.type = KOMMANDO_FLAG_INT,
		.target = &opts.times,
		.default_val = &(int){1},
		.help = "how many times to repeat",
	},
	{
		.long_name = "verbose",
		.short_name = 'v',
		.type = KOMMANDO_FLAG_BOOL,
		.target = &opts.verbose,
		.help = "print extra output",
	},
};

int main(int argc, const char** argv)
{
	return kommando_flags_parse(flags, sizeof(flags) / sizeof(kommando_flag), argc, argv);
}
#include "args.h"
#include "list.h"
#include "kommando/types.h"
#include <stdio.h>

typedef struct
{
	const char* name;
	int times;
	bool verbose;
	const char* package;
	kommando_list packages;
} greet_opts;

static greet_opts opts = {0};

static kommando_flag flags[] = {
	{
		.long_name = "verbose",
		.short_name = 'v',
		.type = KOMMANDO_FLAG_BOOL,
		.target = &opts.verbose,
		.help = "print extra output",
	},
};

static kommando_positional positionals[] = {
	{
		.name = "greeting",
		.type = KOMMANDO_FLAG_STRING,
		.target = &opts.name,
		.required = true,
	},
	{
		.name = "times",
		.type = KOMMANDO_FLAG_INT,
		.target = &opts.times,
	},
	{
		.name = "packages",
		.type = KOMMANDO_FLAG_STRING_LIST,
		.target = &opts.packages,
		.required = true,
	},
};

int main(int argc, const char** argv)
{
	kommando_result result =
		kommando_parse(flags, sizeof(flags) / sizeof(*flags),
					   positionals, sizeof(positionals) / sizeof(*positionals),
					   argc, argv);

	if (result != KOMMANDO_OK)
	{
		fprintf(stderr, "error: %s\n",
				result == KOMMANDO_ERR_UNKNOWN_FLAG		? "unknown flag"
				: result == KOMMANDO_ERR_MISSING_VALUE	? "missing value for flag"
				: result == KOMMANDO_ERR_MISSING_FLAG	? "missing required flag"
				: result == KOMMANDO_ERR_TOO_MANY_ARGS	? "too many arguments"
				: result == KOMMANDO_ERR_MISSING_POSITIONAL ? "missing required positional"
														   : "parse error");
		return 1;
	}

	printf("name=%s times=%d verbose=%d\n", opts.name, opts.times, (int)opts.verbose);
	for (size_t i = 0; i < opts.packages.size; i++)
	{
		printf("package[%zu]=%s\n", i, ((const char**)opts.packages.data)[i]);
	}

	kommando_list_destroy(&opts.packages);
	return 0;
}

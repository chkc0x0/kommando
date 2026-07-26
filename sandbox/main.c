#include "args.h"
#include "kommando/types.h"
#include <stdio.h>

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
		.help = "how many times to repeat",
		.required = true,
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
	kommando_result result =
		kommando_flags_parse(flags, sizeof(flags) / sizeof(*flags), argc, argv);

	if (result != KOMMANDO_OK)
	{
		fprintf(stderr, "error: %s\n",
				result == KOMMANDO_ERR_UNKNOWN_FLAG	   ? "unknown flag"
				: result == KOMMANDO_ERR_MISSING_VALUE ? "missing value for flag"
				: result == KOMMANDO_ERR_MISSING_FLAG  ? "missing required flag"
													   : "parse error");
		return 1;
	}

	printf("name=%s times=%d verbose=%d\n", opts.name, opts.times, (int)opts.verbose);
	for (int i = 0; i < opts.times; i++)
	{
		printf("hello, %s!\n", opts.name);
	}
	return 0;
}

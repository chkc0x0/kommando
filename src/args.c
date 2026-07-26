#include "kommando/args.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

kommando_flag* kommando_flag_find_long(kommando_flag* flags, size_t flagCount,
									   const char* name)
{
	for (size_t i = 0; i < flagCount; i++)
	{
		if (strcmp(flags[i].long_name, name) == 0)
		{
			return &flags[i];
		}
	}

	return NULL;
}

int kommando_flags_parse(kommando_flag* flags, size_t flagCount, int argc,
						 const char** argv)
{
	int i = 0;
	bool double_dash = false;

	while (i < argc && !double_dash)
	{
		// we modify it a bit
		char* arg = strdup(argv[i]);
		size_t arg_len = strlen(arg);

		if (arg_len == 2 && arg[0] == '-' && arg[1] == '-')
		{
			double_dash = true;
			goto cleanup;
		}

		if (arg_len > 2 && arg[0] == '-' && arg[1] == '-')
		{
			char* arg_name = arg + 2;
			const char* arg_val;

			char* equals = strchr(arg, '=');
			bool value_inline = equals != NULL;

			if (value_inline)
			{
				*equals = '\0';
				arg_val = equals + 1;
			}

			kommando_flag* flag = kommando_flag_find_long(flags, flagCount, arg_name);
			if (flag == NULL)
			{
				free(arg);
				return KOMMANDO_ERR_UNKNOWN_FLAG;
			}

			if (flag->type == KOMMANDO_FLAG_BOOL)
			{
				if (value_inline)
				{
					*(bool*)flag->target = (strcmp(arg_val, "false") != 0);
				}
				else
				{
					*(bool*)flag->target = true;
				}
				i++;
			}
			else
			{
				if (!value_inline)
				{
					if (i + 1 >= argc)
					{
						free(arg);
						return KOMMANDO_ERR_MISSING_VALUE;
					}

					arg_val = argv[++i];
				}

				if (flag->type == KOMMANDO_FLAG_STRING)
				{
					char** target_string = (char**)flag->target;
					free(*target_string);
					*target_string = strdup(arg_val);
				}
				else
				{
					*(int*)flag->target = atoi(arg_val);
				}
			}

			goto cleanup;
		}

	cleanup:
		i++;
		free(arg);
	}

	return KOMMANDO_OK;
}
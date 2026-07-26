#include "args.h"
#include <stdlib.h>
#include <string.h>

static kommando_flag* kommando_flag_find_long(kommando_flag* f, size_t n, const char* name, size_t len)
{
	for (size_t i = 0; i < n; i++)
	{
		if (f[i].long_name && strlen(f[i].long_name) == len &&
			memcmp(f[i].long_name, name, len) == 0)
		{
			return &f[i];
		}
	}
	return nullptr;
}

static kommando_flag* kommando_flag_find_short(kommando_flag* f, size_t n, char c)
{
	for (size_t i = 0; i < n; i++)
	{
		if (f[i].short_name == c)
		{
			return &f[i];
		}
	}
	return nullptr;
}

static kommando_result kommando_flag_set(kommando_flag* f, const char* value)
{
	switch (f->type)
	{
	case KOMMANDO_FLAG_BOOL:
		*(bool*)f->target = ((value ? (strcmp(value, "false") != 0) : 1) != 0);
		break;
	case KOMMANDO_FLAG_INT:
		if (!value)
		{
			return KOMMANDO_ERR_MISSING_VALUE;
		}
		*(int*)f->target = atoi(value);
		break;
	case KOMMANDO_FLAG_STRING:
		if (!value)
		{
			return KOMMANDO_ERR_MISSING_VALUE;
		}
		*(const char**)f->target = value;
		break;
	default:
		break;
	}
	return KOMMANDO_OK;
}

static void kommando_flags_apply_defaults(kommando_flag* flags, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		if (!flags[i].default_val)
		{
			continue;
		}
		switch (flags[i].type)
		{
		case KOMMANDO_FLAG_BOOL:
			*(bool*)flags[i].target = *(const bool*)flags[i].default_val;
			break;
		case KOMMANDO_FLAG_INT:
			*(int*)flags[i].target = *(const int*)flags[i].default_val;
			break;
		case KOMMANDO_FLAG_STRING:
			*(const char**)flags[i].target = *(const char* const*)flags[i].default_val;
			break;
		default:
			break;
		}
	}
}

kommando_result kommando_flags_parse(kommando_flag* flags, size_t count, int argc,
									 const char** argv)
{
	bool set[count];
	memset(set, 0, sizeof(set));

	int i = 1;

	while (i < argc)
	{
		const char* arg = argv[i];
		size_t arg_len = strlen(arg);

		if (arg_len == 2 && arg[0] == '-' && arg[1] == '-')
		{
			break;
		}

		if (arg_len > 2 && arg[0] == '-' && arg[1] == '-')
		{
			const char* name = arg + 2;
			const char* eq = strchr(name, '=');
			size_t name_len = eq ? (size_t)(eq - name) : strlen(name);
			const char* value = eq ? eq + 1 : nullptr;

			kommando_flag* f = kommando_flag_find_long(flags, count, name, name_len);
			if (!f)
			{
				return KOMMANDO_ERR_UNKNOWN_FLAG;
			}

			size_t idx = (size_t)(f - flags);

			if (f->type != KOMMANDO_FLAG_BOOL && !value)
			{
				if (i + 1 >= argc)
				{
					return KOMMANDO_ERR_MISSING_VALUE;
				}
				value = argv[++i];
			}

			kommando_result r = kommando_flag_set(f, value);
			if (r != KOMMANDO_OK)
			{
				return r;
			}
			set[idx] = true;
			i++;
			continue;
		}

		if (arg_len >= 2 && arg [0] == '-' && arg[1] != '-')
		{
			kommando_flag* f = kommando_flag_find_short(flags, count, arg[1]);
			if (!f)
			{
				return KOMMANDO_ERR_UNKNOWN_FLAG;
			}

			size_t idx = (size_t)(f - flags);
			const char* value = (arg[2] == '=') ? arg + 3 : nullptr;

			if (f->type != KOMMANDO_FLAG_BOOL && !value)
			{
				if (i + 1 >= argc)
				{
					return KOMMANDO_ERR_MISSING_VALUE;
				}
				value = argv[++i];
			}

			kommando_result r = kommando_flag_set(f, value);
			if (r != KOMMANDO_OK)
			{
				return r;
			}
			set[idx] = true;
			i++;
			continue;
		}

		i++;
	}

	kommando_flags_apply_defaults(flags, count);

	for (size_t j = 0; j < count; j++)
	{
		if (flags[j].required && !set[j])
		{
			return KOMMANDO_ERR_MISSING_FLAG;
		}
	}

	return KOMMANDO_OK;
}

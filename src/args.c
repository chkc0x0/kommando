#include "args.h"
#include "list.h"
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

static bool kommando_is_list_type(kommando_flag_type type)
{
	return (type == KOMMANDO_FLAG_STRING_LIST || type == KOMMANDO_FLAG_INT_LIST) != 0;
}

static kommando_result kommando_positional_set(kommando_positional* p, const char* value)
{
	switch (p->type)
	{
	case KOMMANDO_FLAG_STRING:
		*(const char**)p->target = value;
		break;
	case KOMMANDO_FLAG_INT:
		*(int*)p->target = atoi(value);
		break;
	case KOMMANDO_FLAG_STRING_LIST:
	{
		kommando_list* list = p->target;
		kommando_result r = kommando_list_add(list, &value);
		if (r != KOMMANDO_OK)
		{
			return r;
		}
		break;
	}
	case KOMMANDO_FLAG_INT_LIST:
	{
		kommando_list* list = p->target;
		int v = atoi(value);
		kommando_result r = kommando_list_add(list, &v);
		if (r != KOMMANDO_OK)
		{
			return r;
		}
		break;
	}
	default:
		break;
	}
	return KOMMANDO_OK;
}

kommando_result kommando_parse(kommando_flag* flags, size_t flagCount,
							   kommando_positional* positionals, size_t posCount,
							   int argc, const char** argv)
{
	bool flag_set[flagCount];
	memset(flag_set, 0, sizeof(flag_set));

	const char* rest[argc];
	size_t rest_count = 0;

	for (size_t p = 0; p < posCount; p++)
	{
		if (positionals[p].min_count == 0 && positionals[p].required)
		{
			positionals[p].min_count = 1;
		}
		if (positionals[p].max_count == 0 && !kommando_is_list_type(positionals[p].type))
		{
			positionals[p].max_count = 1;
		}
	}

	for (size_t p = 0; p < posCount; p++)
	{
		if (kommando_is_list_type(positionals[p].type))
		{
			size_t elem = (positionals[p].type == KOMMANDO_FLAG_STRING_LIST)
							  ? sizeof(const char*)
							  : sizeof(int);
			kommando_list_create(positionals[p].target, elem);
		}
	}

	int i = 1;

	while (i < argc)
	{
		const char* arg = argv[i];
		size_t arg_len = strlen(arg);

		if (arg_len == 2 && arg[0] == '-' && arg[1] == '-')
		{
			i++;
			break;
		}

		if (arg_len > 2 && arg[0] == '-' && arg[1] == '-')
		{
			const char* name = arg + 2;
			const char* eq = strchr(name, '=');
			size_t name_len = eq ? (size_t)(eq - name) : strlen(name);
			const char* value = eq ? eq + 1 : nullptr;

			kommando_flag* flag = kommando_flag_find_long(flags, flagCount, name, name_len);
			if (!flag)
			{
				return KOMMANDO_ERR_UNKNOWN_FLAG;
			}

			size_t idx = (size_t)(flag - flags);

			if (flag->type != KOMMANDO_FLAG_BOOL && !value)
			{
				if (i + 1 >= argc)
				{
					return KOMMANDO_ERR_MISSING_VALUE;
				}
				value = argv[++i];
			}

			kommando_result r = kommando_flag_set(flag, value);
			if (r != KOMMANDO_OK)
			{
				return r;
			}
			flag_set[idx] = true;
			i++;
			continue;
		}

		if (arg_len >= 2 && arg[0] == '-' && arg[1] != '-')
		{
			kommando_flag* flag = kommando_flag_find_short(flags, flagCount, arg[1]);
			if (!flag)
			{
				return KOMMANDO_ERR_UNKNOWN_FLAG;
			}

			size_t idx = (size_t)(flag - flags);
			const char* value = (arg[2] == '=') ? arg + 3 : nullptr;

			if (flag->type != KOMMANDO_FLAG_BOOL && !value)
			{
				if (i + 1 >= argc)
				{
					return KOMMANDO_ERR_MISSING_VALUE;
				}
				value = argv[++i];
			}

			kommando_result r = kommando_flag_set(flag, value);
			if (r != KOMMANDO_OK)
			{
				return r;
			}
			flag_set[idx] = true;
			i++;
			continue;
		}

		rest[rest_count++] = arg;
		i++;
	}

	while (i < argc)
	{
		rest[rest_count++] = argv[i++];
	}

	size_t r = 0;
	size_t pos_idx = 0;
	size_t pos_collected = 0;

	while (r < rest_count && pos_idx < posCount)
	{
		kommando_positional* pos = &positionals[pos_idx];

		if (pos->max_count > 0 && pos_collected >= pos->max_count)
		{
			pos_idx++;
			pos_collected = 0;

			if (pos_idx >= posCount)
			{
				return KOMMANDO_ERR_TOO_MANY_ARGS;
			}
			pos = &positionals[pos_idx];
		}

		kommando_result set_r = kommando_positional_set(pos, rest[r]);
		if (set_r != KOMMANDO_OK)
		{
			return set_r;
		}
		pos_collected++;
		r++;
	}

	if (r < rest_count)
	{
		return KOMMANDO_ERR_TOO_MANY_ARGS;
	}

	kommando_flags_apply_defaults(flags, flagCount);

	for (size_t j = 0; j < flagCount; j++)
	{
		if (flags[j].required && !flag_set[j])
		{
			return KOMMANDO_ERR_MISSING_FLAG;
		}
	}

	for (size_t p = 0; p < posCount; p++)
	{
		size_t collected = 0;

		if (kommando_is_list_type(positionals[p].type))
		{
			collected = ((kommando_list*)positionals[p].target)->size;
		}
		else if (positionals[p].required)
		{
			switch (positionals[p].type)
			{
			case KOMMANDO_FLAG_STRING:
				collected = (*(const char**)positionals[p].target) ? 1 : 0;
				break;
			case KOMMANDO_FLAG_INT:
				collected = (*(int*)positionals[p].target) ? 1 : 0;
				break;
			default:
				break;
			}
		}

		if (collected < positionals[p].min_count)
		{
			return KOMMANDO_ERR_MISSING_POSITIONAL;
		}
	}

	return KOMMANDO_OK;
}

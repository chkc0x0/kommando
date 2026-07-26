#include "kommando/args.h"
#include "kommando/list.h"
#include <stdlib.h>
#include <string.h>

#define KOMMANDO_INLINE_FLAGS 32
#define KOMMANDO_INLINE_REST 32
#define KOMMANDO_INLINE_SYNTH 8

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
	case KOMMANDO_FLAG_COUNT:
		(*(int*)f->target)++;
		break;
	default:
		break;
	}
	return KOMMANDO_OK;
}

static void kommando_flags_apply_defaults(kommando_flag* flags, size_t count,
	const bool* flagSet)
{
	for (size_t i = 0; i < count; i++)
	{
		if (flagSet[i] || !flags[i].default_val)
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
		case KOMMANDO_FLAG_COUNT:
			*(int*)flags[i].target = *(const int*)flags[i].default_val;
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
		kommando_result r = kommando_list_add(list, (void*)&value);
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

static kommando_result kommando_consume_flags(
	kommando_flag* flags, size_t flagCount,
	int argc, const char** argv,
	bool strict,
	bool* flagSet,
	const char** rest, size_t* restCount,
	const char** synthFree, size_t* synthFreeCount)
{
	size_t rc = 0;
	size_t sfc = 0;
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
				if (strict)
				{
					return KOMMANDO_ERR_UNKNOWN_FLAG;
				}
				rest[rc++] = arg;
				i++;
				continue;
			}

			size_t idx = (size_t)(flag - flags);

			if (flag->type != KOMMANDO_FLAG_BOOL &&
				flag->type != KOMMANDO_FLAG_COUNT && !value)
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
			flagSet[idx] = true;
			i++;
			continue;
		}

		if (arg_len >= 2 && arg[0] == '-' && arg[1] != '-')
		{
			const char* cur = arg + 1;

			char synth_buf[256];
			size_t synth_len = 0;

			while (*cur)
			{
				kommando_flag* flag = kommando_flag_find_short(flags, flagCount, *cur);
				if (!flag)
				{
					if (strict)
					{
						return KOMMANDO_ERR_UNKNOWN_FLAG;
					}
					synth_buf[synth_len++] = *cur;
					cur++;
					continue;
				}

				if (synth_len > 0)
				{
					synth_buf[synth_len] = '\0';
					char* synth = malloc(synth_len + 2);
					if (!synth)
					{
						return KOMMANDO_ERR_OOM;
					}
					synth[0] = '-';
					memcpy(synth + 1, synth_buf, synth_len + 1);
					synthFree[sfc++] = synth;
					rest[rc++] = synth;
					synth_len = 0;
				}

				size_t idx = (size_t)(flag - flags);
				cur++;

				if (flag->type == KOMMANDO_FLAG_BOOL ||
					flag->type == KOMMANDO_FLAG_COUNT)
				{
					kommando_flag_set(flag, nullptr);
					flagSet[idx] = true;
					continue;
				}

				const char* value = nullptr;
				if (*cur == '=')
				{
					value = cur + 1;
					cur += strlen(cur);
				}
				else if (*cur)
				{
					value = cur;
					cur += strlen(cur);
				}
				else
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
				flagSet[idx] = true;
			}

			if (synth_len > 0)
			{
				synth_buf[synth_len] = '\0';
				char* synth = malloc(synth_len + 2);
				if (!synth)
				{
					return KOMMANDO_ERR_OOM;
				}
				synth[0] = '-';
				memcpy(synth + 1, synth_buf, synth_len + 1);
				synthFree[sfc++] = synth;
				rest[rc++] = synth;
			}

			i++;
			continue;
		}

		rest[rc++] = argv[i];
		i++;
		continue;

		i++;
	}

	while (i < argc)
	{
		rest[rc++] = argv[i++];
	}

	*restCount = rc;
	*synthFreeCount = sfc;
	return KOMMANDO_OK;
}

void kommando_cmd_finalize(kommando_cmd* cmd)
{
	for (size_t s = 0; s < cmd->subcommand_count; s++)
	{
		cmd->subcommands[s].parent = cmd;
		kommando_cmd_finalize(&cmd->subcommands[s]);
	}
}

static kommando_result kommando_do_parse(kommando_cmd* cmd, kommando_cmd** leaf,
										 int argc, const char** argv)
{
	if (cmd->subcommands && cmd->subcommand_count > 0 && argc > 1)
	{
		bool flag_set_inline[KOMMANDO_INLINE_FLAGS];
		bool* flag_set = flag_set_inline;
		if (cmd->flag_count > KOMMANDO_INLINE_FLAGS)
		{
			flag_set = malloc(cmd->flag_count * sizeof(bool));
			if (!flag_set)
			{
				return KOMMANDO_ERR_OOM;
			}
		}
		memset(flag_set, 0, cmd->flag_count * sizeof(bool));

		const char* rest_inline[KOMMANDO_INLINE_REST];
		const char** rest = rest_inline;
		if ((size_t)argc > KOMMANDO_INLINE_REST)
		{
			rest = (const char**)malloc((size_t)argc * sizeof(const char*));
			if (!rest)
			{
				if (flag_set != flag_set_inline)
				{
					free(flag_set);
				}
				return KOMMANDO_ERR_OOM;
			}
		}
		size_t rest_count = 0;

		const char* synth_inline[KOMMANDO_INLINE_SYNTH];
		const char** synth_free = synth_inline;
		size_t synth_count = 0;

		kommando_result r = kommando_consume_flags(
			cmd->flags, cmd->flag_count,
			argc, argv, false,
			flag_set, rest, &rest_count,
			synth_free, &synth_count);

		if (r != KOMMANDO_OK)
		{
			goto descent_cleanup;
		}

		kommando_flags_apply_defaults(cmd->flags, cmd->flag_count, flag_set);
		for (size_t j = 0; j < cmd->flag_count; j++)
		{
			if (cmd->flags[j].required && !flag_set[j])
			{
				r = KOMMANDO_ERR_MISSING_FLAG;
				goto descent_cleanup;
			}
		}

		{
			int sub_rest_idx = -1;
			int sub_cmd_idx = -1;
			for (size_t n = 0; n < rest_count; n++)
			{
				for (size_t s = 0; s < cmd->subcommand_count; s++)
				{
					if (strcmp(cmd->subcommands[s].name, rest[n]) == 0)
					{
						sub_rest_idx = (int)n;
						sub_cmd_idx = (int)s;
						break;
					}
				}
				if (sub_rest_idx >= 0)
				{
					break;
				}
			}

			if (sub_cmd_idx < 0)
			{
				r = KOMMANDO_ERR_UNKNOWN_CMD;
				goto descent_cleanup;
			}

			const char* filtered_inline[KOMMANDO_INLINE_REST];
			const char** filtered = filtered_inline;
			size_t filtered_count = 0;

			if (rest_count > KOMMANDO_INLINE_REST)
			{
				filtered = (const char**)malloc((rest_count + 1) * sizeof(const char*));
				if (!filtered)
				{
					r = KOMMANDO_ERR_OOM;
					goto descent_cleanup;
				}
			}

			filtered[filtered_count++] = argv[0];
			for (size_t n = 0; n < rest_count; n++)
			{
				if ((int)n != sub_rest_idx)
				{
					filtered[filtered_count++] = rest[n];
				}
			}

			r = kommando_do_parse(
				&cmd->subcommands[sub_cmd_idx], leaf, (int)filtered_count, filtered);
			if (filtered != filtered_inline)
			{
				free((void*)filtered);
			}
		}

descent_cleanup:
		for (size_t s = 0; s < synth_count; s++)
		{
			free((void*)synth_free[s]);
		}
		if (rest != rest_inline)
		{
			free((void*)rest);
		}
		if (flag_set != flag_set_inline)
		{
			free(flag_set);
		}
		return r;
	}

	*leaf = cmd;

	kommando_flag* flags = cmd->flags;
	size_t flag_count = cmd->flag_count;
	kommando_positional* positionals = cmd->positionals;
	size_t pos_count = cmd->positional_count;

	kommando_result result = KOMMANDO_OK;

	bool flag_set_inline[KOMMANDO_INLINE_FLAGS];
	bool* flag_set = flag_set_inline;
	if (flag_count > KOMMANDO_INLINE_FLAGS)
	{
		flag_set = malloc(flag_count * sizeof(bool));
		if (!flag_set)
		{
			return KOMMANDO_ERR_OOM;
		}
	}
	memset(flag_set, 0, flag_count * sizeof(bool));

	const char* rest_inline[KOMMANDO_INLINE_REST];
	const char** rest = rest_inline;
	if ((size_t)argc > KOMMANDO_INLINE_REST)
	{
		rest = (const char**)malloc((size_t)argc * sizeof(const char*));
		if (!rest)
		{
			if (flag_set != flag_set_inline)
			{
				free(flag_set);
			}
			return KOMMANDO_ERR_OOM;
		}
	}
	size_t rest_count = 0;

	const char* synth_inline[KOMMANDO_INLINE_SYNTH];
	const char** synth_free = synth_inline;
	size_t synth_count = 0;

	size_t pos_seen_inline[KOMMANDO_INLINE_FLAGS];
	size_t* pos_seen = pos_seen_inline;
	if (pos_count > KOMMANDO_INLINE_FLAGS)
	{
		pos_seen = malloc(pos_count * sizeof(size_t));
		if (!pos_seen)
		{
			if (rest != rest_inline)
			{
				free((void*)rest);
			}
			if (flag_set != flag_set_inline)
			{
				free(flag_set);
			}
			return KOMMANDO_ERR_OOM;
		}
	}
	memset(pos_seen, 0, pos_count * sizeof(size_t));

	for (size_t p = 0; p < pos_count; p++)
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

	for (size_t p = 0; p < pos_count; p++)
	{
		if (kommando_is_list_type(positionals[p].type))
		{
			size_t elem = (positionals[p].type == KOMMANDO_FLAG_STRING_LIST)
							  ? sizeof(const char*)
							  : sizeof(int);
			kommando_list_create(positionals[p].target, elem);
		}
	}

	result = kommando_consume_flags(
		flags, flag_count,
		argc, argv, true,
		flag_set, rest, &rest_count,
		synth_free, &synth_count);

	if (result != KOMMANDO_OK)
	{
		goto leaf_cleanup;
	}

	{
		size_t ri = 0;
		size_t pos_idx = 0;
		size_t pos_collected = 0;

		while (ri < rest_count && pos_idx < pos_count)
		{
			kommando_positional* pos = &positionals[pos_idx];

			if (pos->max_count > 0 && pos_collected >= pos->max_count)
			{
				pos_idx++;
				pos_collected = 0;

				if (pos_idx >= pos_count)
				{
					result = KOMMANDO_ERR_TOO_MANY_ARGS;
					goto leaf_cleanup;
				}
				pos = &positionals[pos_idx];
			}

			kommando_result set_r = kommando_positional_set(pos, rest[ri]);
			if (set_r != KOMMANDO_OK)
			{
				result = set_r;
				goto leaf_cleanup;
			}
			pos_seen[pos_idx]++;
			pos_collected++;
			ri++;
		}

		if (ri < rest_count)
		{
			result = KOMMANDO_ERR_TOO_MANY_ARGS;
			goto leaf_cleanup;
		}
	}

	kommando_flags_apply_defaults(flags, flag_count, flag_set);

	for (size_t j = 0; j < flag_count; j++)
	{
		if (flags[j].required && !flag_set[j])
		{
			result = KOMMANDO_ERR_MISSING_FLAG;
			goto leaf_cleanup;
		}
	}

	for (size_t p = 0; p < pos_count; p++)
	{
		if (pos_seen[p] < positionals[p].min_count)
		{
			result = KOMMANDO_ERR_MISSING_POSITIONAL;
			goto leaf_cleanup;
		}
	}

leaf_cleanup:
	if (pos_seen != pos_seen_inline)
	{
		free(pos_seen);
	}
	for (size_t s = 0; s < synth_count; s++)
	{
		free((void*)synth_free[s]);
	}
	if (flag_set != flag_set_inline)
	{
		free(flag_set);
	}
	if (rest != rest_inline)
	{
		free((void*)rest);
	}
	return result;
}

kommando_result kommando_parse(kommando_cmd* cmd, int argc, const char** argv)
{
	kommando_cmd* leaf = nullptr;
	kommando_result r = kommando_do_parse(cmd, &leaf, argc, argv);
	if (r != KOMMANDO_OK)
	{
		return r;
	}
	if (leaf && leaf->handler)
	{
		return leaf->handler(leaf);
	}
	return KOMMANDO_OK;
}

kommando_result kommando_parse_nodispatch(kommando_cmd* cmd, kommando_cmd** leaf,
										   int argc, const char** argv)
{
	return kommando_do_parse(cmd, leaf, argc, argv);
}

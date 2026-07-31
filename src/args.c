#include "kommando/args.h"
#include "kommando/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KOMMANDO_INLINE_FLAGS 32
#define KOMMANDO_INLINE_REST 32
#define KOMMANDO_INLINE_SYNTH 8

static const char* kommando_arg_err_to_string(kommando_arg_error err)
{
	switch (err)
	{
	case KOMMANDO_ARG_ERR_NONE:
		return "no error";
	case KOMMANDO_ARG_ERR_UNKNOWN_FLAG:
		return "unknown flag";
	case KOMMANDO_ARG_ERR_MISSING_VALUE:
		return "missing value for flag";
	case KOMMANDO_ARG_ERR_MISSING_FLAG:
		return "missing required flag";
	case KOMMANDO_ARG_ERR_MISSING_POSITIONAL:
		return "missing required argument";
	case KOMMANDO_ARG_ERR_TOO_MANY_ARGS:
		return "too many arguments";
	case KOMMANDO_ARG_ERR_UNKNOWN_CMD:
		return "unknown command";
	case KOMMANDO_ARG_ERR_INVALID_VALUE:
		return "invalid value";
	case KOMMANDO_ARG_ERR_VALIDATION_FAILED:
		return "validation failed";
	case KOMMANDO_ARG_ERR_OOM:
		return "out of memory";
	default:
		return "unknown error";
	}
}

static void kommando_default_error_handler(kommando_cmd* cmd,
										   const kommando_arg_err_info* info)
{
	if (info->flag_name)
	{
		fprintf(stderr, "%s: %s: %s\n", cmd->name, info->flag_name,
				kommando_arg_err_to_string(info->error));
	}
	else
	{
		fprintf(stderr, "%s: %s\n", cmd->name, kommando_arg_err_to_string(info->error));
	}
}

static kommando_arg_err_handler kommando_resolve_error_handler(kommando_cmd* cmd)
{
	for (kommando_cmd* c = cmd; c; c = c->parent)
	{
		if (c->on_error)
		{
			return c->on_error;
		}
	}
	return kommando_default_error_handler;
}

static kommando_flag* kommando_flag_find_long(kommando_flag* f, size_t n,
											  const char* name, size_t len)
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

static inline void* kommando_arg_target(void* userData, size_t offset)
{
	if (offset == ko_offset_none)
	{
		return nullptr;
	}
	return (char*)userData + offset;
}

static kommando_result kommando_flag_set(kommando_flag* f, void* userData,
										 const char* value,
										 kommando_arg_err_info* errInfo)
{
	void* target = kommando_arg_target(userData, f->target_offset);
	if (!target)
	{
		return KOMMANDO_OK;
	}
	switch (f->type)
	{
	case KOMMANDO_FLAG_BOOL:
		*(bool*)target = ((value ? (strcmp(value, "false") != 0) : 1) != 0);
		break;
	case KOMMANDO_FLAG_INT:
		if (!value)
		{
			errInfo->error = KOMMANDO_ARG_ERR_MISSING_VALUE;
			errInfo->flag_name = f->long_name;
			errInfo->offending_value = nullptr;
			return KOMMANDO_ERR_ARG_PARSE;
		}
		*(int*)target = atoi(value);
		break;
	case KOMMANDO_FLAG_STRING:
		if (!value)
		{
			errInfo->error = KOMMANDO_ARG_ERR_MISSING_VALUE;
			errInfo->flag_name = f->long_name;
			errInfo->offending_value = nullptr;
			return KOMMANDO_ERR_ARG_PARSE;
		}
		*(const char**)target = value;
		break;
	case KOMMANDO_FLAG_COUNT:
		(*(int*)target)++;
		break;
	default:
		break;
	}
	return KOMMANDO_OK;
}

static void kommando_flags_apply_defaults(kommando_flag* flags, size_t count,
										  const bool* flagSet, void* user_data)
{
	for (size_t i = 0; i < count; i++)
	{
		if (flagSet[i] || !flags[i].default_val)
		{
			continue;
		}
		void* target = kommando_arg_target(user_data, flags[i].target_offset);
		if (!target) {
			continue;
}
		switch (flags[i].type)
		{
		case KOMMANDO_FLAG_BOOL:
			*(bool*)target = *(const bool*)flags[i].default_val;
			break;
		case KOMMANDO_FLAG_INT:
			*(int*)target = *(const int*)flags[i].default_val;
			break;
		case KOMMANDO_FLAG_STRING:
			*(const char**)target = *(const char* const*)flags[i].default_val;
			break;
		case KOMMANDO_FLAG_COUNT:
			*(int*)target = *(const int*)flags[i].default_val;
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

static kommando_result kommando_positional_set(kommando_positional* p, void* user_data,
											   const char* value)
{
	void* target = kommando_arg_target(user_data, p->target_offset);
	switch (p->type)
	{
	case KOMMANDO_FLAG_STRING:
		*(const char**)target = value;
		break;
	case KOMMANDO_FLAG_INT:
		*(int*)target = atoi(value);
		break;
	case KOMMANDO_FLAG_STRING_LIST:
	{
		kommando_list* list = target;
		kommando_result r = kommando_list_add(list, (void*)&value);
		if (r != KOMMANDO_OK)
		{
			return r;
		}
		break;
	}
	case KOMMANDO_FLAG_INT_LIST:
	{
		kommando_list* list = target;
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
	kommando_flag* flags, size_t flagCount, void* userData, int argc, const char** argv,
	bool strict, bool* flagSet, const char** rest, size_t* restCount,
	const char** synthFree, size_t synthFreeCapacity, size_t* synthFreeCount,
	kommando_arg_err_info* errInfo)
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

			kommando_flag* flag =
				kommando_flag_find_long(flags, flagCount, name, name_len);
			if (!flag)
			{
				if (strict)
				{
					errInfo->error = KOMMANDO_ARG_ERR_UNKNOWN_FLAG;
					errInfo->flag_name = name;
					errInfo->offending_value = nullptr;
					return KOMMANDO_ERR_ARG_PARSE;
				}
				rest[rc++] = arg;
				i++;
				continue;
			}

			size_t idx = (size_t)(flag - flags);

			if (flag->type != KOMMANDO_FLAG_BOOL && flag->type != KOMMANDO_FLAG_COUNT &&
				!value)
			{
				if (i + 1 >= argc)
				{
					errInfo->error = KOMMANDO_ARG_ERR_MISSING_VALUE;
					errInfo->flag_name = flag->long_name;
					errInfo->offending_value = nullptr;
					return KOMMANDO_ERR_ARG_PARSE;
				}
				value = argv[++i];
			}

			kommando_result r = kommando_flag_set(flag, userData, value, errInfo);
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
						errInfo->error = KOMMANDO_ARG_ERR_UNKNOWN_FLAG;
						errInfo->flag_name = nullptr;
						errInfo->offending_value = cur;
						return KOMMANDO_ERR_ARG_PARSE;
					}
					if (synth_len + 1 >= sizeof(synth_buf))
					{
						errInfo->error = KOMMANDO_ARG_ERR_TOO_MANY_ARGS;
						errInfo->flag_name = nullptr;
						errInfo->offending_value = arg;
						return KOMMANDO_ERR_ARG_PARSE;
					}
					synth_buf[synth_len++] = *cur;
					cur++;
					continue;
				}

				if (synth_len > 0)
				{
					if (sfc >= synthFreeCapacity)
					{
						return KOMMANDO_ERR_OOM;
					}
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

				if (flag->type == KOMMANDO_FLAG_BOOL || flag->type == KOMMANDO_FLAG_COUNT)
				{
					kommando_flag_set(flag, userData, nullptr, errInfo);
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
						errInfo->error = KOMMANDO_ARG_ERR_MISSING_VALUE;
						errInfo->flag_name = flag->long_name;
						errInfo->offending_value = nullptr;
						return KOMMANDO_ERR_ARG_PARSE;
					}
					value = argv[++i];
				}

				kommando_result r = kommando_flag_set(flag, userData, value, errInfo);
				if (r != KOMMANDO_OK)
				{
					return r;
				}
				flagSet[idx] = true;
			}

			if (synth_len > 0)
			{
				if (sfc >= synthFreeCapacity)
				{
					return KOMMANDO_ERR_OOM;
				}
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
										 kommando_arg_err_info* errInfo, int argc,
										 const char** argv)
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
		size_t synth_capacity = KOMMANDO_INLINE_SYNTH;
		if ((size_t)argc > KOMMANDO_INLINE_SYNTH)
		{
			synth_free = (const char**)malloc((size_t)argc * sizeof(const char*));
			if (!synth_free)
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
			synth_capacity = (size_t)argc;
		}
		size_t synth_count = 0;

		kommando_result r = kommando_consume_flags(
			cmd->flags, cmd->flag_count, cmd->user_data, argc, argv, false, flag_set,
			rest, &rest_count, synth_free, synth_capacity, &synth_count, errInfo);

		if (r != KOMMANDO_OK)
		{
			goto descent_cleanup;
		}

		kommando_flags_apply_defaults(cmd->flags, cmd->flag_count, flag_set,
									  cmd->user_data);
		for (size_t j = 0; j < cmd->flag_count; j++)
		{
			if (cmd->flags[j].required && !flag_set[j])
			{
				errInfo->error = KOMMANDO_ARG_ERR_MISSING_FLAG;
				errInfo->flag_name = cmd->flags[j].long_name;
				errInfo->offending_value = nullptr;
				r = KOMMANDO_ERR_ARG_PARSE;
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
				errInfo->error = KOMMANDO_ARG_ERR_UNKNOWN_CMD;
				errInfo->flag_name = nullptr;
				errInfo->offending_value = (rest_count > 0) ? rest[0] : nullptr;
				r = KOMMANDO_ERR_ARG_PARSE;
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

			filtered[filtered_count++] = cmd->subcommands[sub_cmd_idx].name;
			for (size_t n = 0; n < rest_count; n++)
			{
				if ((int)n != sub_rest_idx)
				{
					filtered[filtered_count++] = rest[n];
				}
			}

			r = kommando_do_parse(&cmd->subcommands[sub_cmd_idx], leaf, errInfo,
								  (int)filtered_count, filtered);
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
		if (synth_free != synth_inline)
		{
			free((void*)synth_free);
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
	size_t synth_capacity = KOMMANDO_INLINE_SYNTH;
	if ((size_t)argc > KOMMANDO_INLINE_SYNTH)
	{
		synth_free = (const char**)malloc((size_t)argc * sizeof(const char*));
		if (!synth_free)
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
		synth_capacity = (size_t)argc;
	}
	size_t synth_count = 0;

	size_t pos_seen_inline[KOMMANDO_INLINE_FLAGS];
	size_t* pos_seen = pos_seen_inline;
	if (pos_count > KOMMANDO_INLINE_FLAGS)
	{
		pos_seen = malloc(pos_count * sizeof(size_t));
		if (!pos_seen)
		{
			if (synth_free != synth_inline)
			{
				free((void*)synth_free);
			}
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
			kommando_list* list =
				kommando_arg_target(cmd->user_data, positionals[p].target_offset);
			kommando_list_create(list, elem);
		}
	}

	result = kommando_consume_flags(flags, flag_count, cmd->user_data, argc, argv, true,
									flag_set, rest, &rest_count, synth_free,
									synth_capacity, &synth_count, errInfo);

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
					errInfo->error = KOMMANDO_ARG_ERR_TOO_MANY_ARGS;
					errInfo->flag_name = nullptr;
					errInfo->offending_value = rest[ri];
					result = KOMMANDO_ERR_ARG_PARSE;
					goto leaf_cleanup;
				}
				pos = &positionals[pos_idx];
			}

			kommando_result set_r =
				kommando_positional_set(pos, cmd->user_data, rest[ri]);
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
			errInfo->error = KOMMANDO_ARG_ERR_TOO_MANY_ARGS;
			errInfo->flag_name = nullptr;
			errInfo->offending_value = rest[ri];
			result = KOMMANDO_ERR_ARG_PARSE;
			goto leaf_cleanup;
		}
	}

	kommando_flags_apply_defaults(flags, flag_count, flag_set, cmd->user_data);

	for (size_t j = 0; j < flag_count; j++)
	{
		if (flags[j].required && !flag_set[j])
		{
			errInfo->error = KOMMANDO_ARG_ERR_MISSING_FLAG;
			errInfo->flag_name = flags[j].long_name;
			errInfo->offending_value = nullptr;
			result = KOMMANDO_ERR_ARG_PARSE;
			goto leaf_cleanup;
		}
	}

	for (size_t p = 0; p < pos_count; p++)
	{
		if (pos_seen[p] < positionals[p].min_count)
		{
			errInfo->error = KOMMANDO_ARG_ERR_MISSING_POSITIONAL;
			errInfo->flag_name = positionals[p].name;
			errInfo->offending_value = nullptr;
			result = KOMMANDO_ERR_ARG_PARSE;
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
	if (synth_free != synth_inline)
	{
		free((void*)synth_free);
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
	kommando_arg_err_info err_info = {0};

	kommando_result r = kommando_do_parse(cmd, &leaf, &err_info, argc, argv);

	if (r != KOMMANDO_OK)
	{
		kommando_cmd* context_cmd = leaf ? leaf : cmd;
		kommando_arg_err_handler handler = kommando_resolve_error_handler(context_cmd);
		handler(context_cmd, &err_info);
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
	kommando_arg_err_info err_info = {0};
	return kommando_do_parse(cmd, leaf, &err_info, argc, argv);
}
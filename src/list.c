#include "list.h"
#include <stdlib.h>
#include <string.h>

void kommando_list_create(kommando_list* list, size_t elemSize)
{
	list->data = NULL;
	list->size = 0;
	list->cap = 0;
	list->elem_size = elemSize;
}

kommando_result kommando_list_add(kommando_list* list, const void* elem)
{
	if (list->size >= list->cap)
	{
		size_t new_cap = list->cap ? list->cap * 2 : 4;
		void* new_data = realloc(list->data, new_cap * list->elem_size);
		if (!new_data)
		{
			return KOMMANDO_ERR_MISSING_VALUE;
		}
		list->data = new_data;
		list->cap = new_cap;
	}
	void* dest = (char*)list->data + (list->size * list->elem_size);
	memcpy(dest, elem, list->elem_size);
	list->size++;
	return KOMMANDO_OK;
}

void kommando_list_destroy(kommando_list* list)
{
	free(list->data);
	list->data = NULL;
	list->size = 0;
	list->cap = 0;
}

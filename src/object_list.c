//
//  object_list.c
//  
//
//  Created by Parker, Charles W. on 8/18/18.
//

#include <stdlib.h>
#include <string.h>

#include "global.h"

#include "object_list.h"

object_list * object_list_new(int n, unsigned item_size)
{
  object_list * list = (object_list*) malloc(sizeof(object_list));
  list->item_size = item_size;
  list->size = n;
  list->count = 0;
  list->items = malloc(n*item_size);
  list->ops = NULL;
  return list;
}

void object_list_delete(object_list * list)
{
  object_list_clear(list);
  free(list);
}

void object_list_clear(object_list * list)
{
  int i=0;
  DBG_MSG("Clearing list\n");
  if (list->ops && list->ops->clear_object){
    for(i=0; i < list->count; i++){
      list->ops->clear_object(object_list_get_item(list, i));
    }
  } else {
    memset(list->items, 0, list->size*list->item_size);
  }
  list->count = 0;
}

object_list * object_list_expand(object_list * list, int n)
{
  object_list * new_list =
                object_list_new(list->size + n, list->item_size);
  DBG_MSG("Expanding list by %d\n", n);
  new_list->count = list->count;
  memcpy(new_list->items, list->items, list->size*list->item_size);
  object_list_delete(list);
  return new_list;
}

object_list * object_list_append(object_list * list, void * item)
{
  DBG_MSG("Appending to list... item %d\n", list->count);
  if (list->count == list->size)  list = object_list_expand(list, 1);
  if (list->ops && list->ops->copy_object)
    list->ops->copy_object(list->items + list->item_size*list->count,
                           item);
  list->count++;
  return list;
}

void object_list_describe(object_list * list)
{
  
}

void * object_list_get_item(object_list * list, int n)
{
  return list->items + list->item_size*n;
}

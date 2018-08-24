//
//  object_list.c
//  
//
//  Created by Parker, Charles W. on 8/18/18.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#include "global.h"
#define DEBUG
#if defined(DEBUG) //&& DEBUG > 0
#define DBG_MSG(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
__FILE__, __LINE__, __func__, ##args)
#else
#define DBG_MSG(fmt, args...) /* Don't do anything in release builds */
#endif

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

void object_list_expand(object_list * list, int n)
{
  void * new_items = malloc((list->size + n)*list->item_size);
  DBG_MSG("Expanding list by %d\n", n);
  memcpy(new_items, list->items, list->size*list->item_size);
  list->size += n;
  free(list->items);
  list->items = new_items;
}

void object_list_append(object_list * list, void * item)
{
  DBG_MSG("Appending to list... item %d\n", list->count);
  if (list->count == list->size) object_list_expand(list, 1);
  if (list->ops && list->ops->copy_object){
    DBG_MSG("Copying with copy operation object\n");
    list->ops->copy_object(list->items + list->item_size*list->count,
                           item);
  } else {
    DBG_MSG("Copying with memcpy\n");
    memcpy(list->items + list->item_size*list->count, item, list->item_size);
  }
  list->count++;
}

void object_list_describe(object_list * list)
{
  printf("object list has %d / %d items of size %d bytes\n", list->count, list->size, list->item_size);
}

void * object_list_get_item(object_list * list, int n)
{
  if (n >= list->count) return 0;
  return list->items + list->item_size*n;
}

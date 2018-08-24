//
//  object_list.h
//  
//
//  Created by Parker, Charles W. on 8/18/18.
//

#ifndef object_list_h
#define object_list_h

typedef struct object_operations
{
  void (*copy_object)(void *, void *);
  void (*clear_object)(void *);
  void (*delete_object)(void *);
} object_operations;

typedef struct object_list
{
  int count;
  int size;
  int item_size;
  void * items;
  object_operations * ops;
} object_list;

object_list * object_list_new(int n, unsigned item_size);
void object_list_delete(object_list * list);
void object_list_clear(object_list * list);
void object_list_expand(object_list * list, int n);
void object_list_append(object_list * list, void * item);
void object_list_describe(object_list * list);
void * object_list_get_item(object_list * list, int n);
#endif /* object_list_h */

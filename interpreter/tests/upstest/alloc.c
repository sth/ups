/* 
* alloc.c - a fast memory allocator 
*
* dibyendu majumdar                 
* adapted from code in C++ STL lib 
*
* This file implements a fast fixed size  memory allocator. It is very useful
* for situations where you need to allocate and free fixed size memory chunks
* fairly frequently.
*
*/

#include <stdlib.h>

#ifdef _USE_SAFE_MALLOC
#include "poc_util.h"
#else
#define safe_malloc malloc
#define safe_free free
#endif

#include "alloc.h"


/***
*	FUNCTION	: new_allocator
*
*	DESCRIPTION	: create a new allocator object
*
*	ARGS		: size_t [IN] - object size
*			: size_t [IN] - no of objects to be allocated initially
*
*	RETURNS		: ptr to newly created object
*/
allocator      *
new_allocator(size_t size, size_t n)
{
#ifndef __CXC__
	allocator      *a = (allocator *) safe_malloc(sizeof(allocator));
#else
	allocator      *a;

	a = (allocator *) safe_malloc(sizeof(allocator));
#endif
	a->buffer_list = 0;
	a->free_list = 0;
	a->next_avail = 0;
	a->last = 0;
	a->size = size;
	a->n = n;
	return a;
}

/***
*
*	FUNCTION	: grow_allocator
*
*	DESCRIPTION	: grows an allocator object 
*
*	ARGS		: allocator * - ptr to allocator object
*
*	RETTURNS	:
*/
void 
grow_allocator(allocator * a)
{
#ifndef __CXC__
	buffer_type_t    *tmp = (buffer_type_t *) safe_malloc(sizeof(buffer_type_t));
#else
	buffer_type_t    *tmp;

	tmp = (buffer_type_t *) safe_malloc(sizeof(buffer_type_t));
#endif
	tmp->buffer = (char *) safe_malloc(a->size * a->n);
	tmp->next_buffer = a->buffer_list;
	a->buffer_list = tmp;
	a->next_avail = a->buffer_list->buffer;
	a->last = a->next_avail + (a->size * a->n);
}

/***
*
*	FUNCTION	: alloc_node
*
*	DESCRIPTION	: allocates memory using an allocator
*
*	ARGS		: allocator *
*
*	RETURNS		: void * - ptr to memory allocated
*/
void           *
alloc_node(allocator * a)
{
#ifndef __CXC__
	link_t           *tmp = a->free_list;
#else
	link_t           *tmp;
	tmp = a->free_list;
#endif

	if (a->free_list) {
		a->free_list = (link_t *) (a->free_list->next);
		return (void *) tmp;
	} else {
		if (a->next_avail == a->last) {
			grow_allocator(a);
		} 
		{
#ifndef __CXC__
			void           *tmp = (void *) a->next_avail;
#else
			void           *tmp;
			tmp = (void *) a->next_avail;
#endif
			a->next_avail += a->size;
			return tmp;
		}
	}
}

/***
*
*	FUNCTION	: dealloc_node
*
*	DESCRIPTION	: free memory allocated by alloc_node
*
*	ARGS		: allocator *
*			: void * - ptr to memory to be freed
*
*	RETURNS		: 
*/
void 
dealloc_node(allocator * a, void *n)
{
	((link_t *) n)->next = a->free_list;
	a->free_list = (link_t *) n;
}

/***
*	FUNCTION	: destroy_allocator
*
*	DESCRIPTION	: free memory associated with an allocator
*
*	ARGS		: allocator *
*
*	RETURNS		:
*/
void 
destroy_allocator(allocator * a)
{
	while (a->buffer_list) {
#ifndef __CXC__
		buffer_type_t    *tmp = a->buffer_list;
#else
		buffer_type_t    *tmp;
		tmp = a->buffer_list;
#endif
		a->buffer_list = a->buffer_list->next_buffer;
		safe_free(tmp->buffer);
		safe_free(tmp);
	}
	safe_free(a);
}


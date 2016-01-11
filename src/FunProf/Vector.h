#ifndef _VECTOR_H_
#define _VECTOR_H_

/* 
 * Basic dynamic array implementation
 * Expected usage is to allocate a vector and add to the back of the
 * vector. The vector should be cleaned up when complete with the
 * provided 'free'
 *
 * An array of void*'s. The user is responsible for being aware that
 * ONLY POINTERS CAN BE STORED IN THIS VECTOR, and for allocating
 * and deallocating the individual array elements.
 */

#include "Common.h"

typedef struct _Vector Vector;

/* Return an initialized vector of size 'size' */
Vector* vector_init(UInt32 size);

/* Return the used space in the vector */
UInt32  vector_used(const Vector *v);

/* Return the element pointer at idx */
void*  vector_get(const Vector *v, UInt32 idx);

/* Add the pointer to the end of the vector */
void   vector_add(Vector *v, const void* const elem);

/* Set the pointer at an index; ERROR uninitialized or out of range */
void   vector_set(Vector *v, UInt32 idx, void* elem);

/* Manually resize the vector */
void   vector_resize(Vector* v);

//THE USER IS RESPONSIBLE FOR FREEING INDIVIDUAL VECTOR ELEMENTS
//THIS DOES NOT DO WHAT YOU THINK IT DOES
/* frees all malloc'd meta data for the vector and the vector itself */
void   vector_free(Vector *v);

#endif

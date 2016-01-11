#include "Vector.h"

struct _Vector{
	UInt32 used;
	UInt32 size;
	const void** arr;
};

Vector* vector_init(UInt32 size){
	Vector* v = SGL_MALLOC("sgl.vector",sizeof(Vector));
	v->used = 0;
	v->size = size;
	v->arr = SGL_MALLOC("sgl.vector", size*sizeof(void*));
	return v;
}

void vector_free(Vector* v){
	SGL_FREE(v->arr);
	SGL_FREE(v);
}

void* vector_get(const Vector *v, UInt32 idx){
	if ( idx >= v->used )
		exitAtError("Vector","Array out of bounds");
	return (void*)v->arr[idx];
}

UInt32  vector_used(const Vector *v){
	return v->used;
}

void vector_add(Vector *v, const void* const elem){
	if( v->used == v->size )
		vector_resize(v);
	v->arr[v->used++] = elem;
}

void vector_set(Vector *v, UInt32 idx, void* elem){
	if ( idx >= v->used )
		exitAtError("Vector","Array out of bounds");
	v->arr[idx] = elem;
}

void vector_resize(Vector* v){
	v->size *= 2;
	v->arr = SGL_REALLOC("SGL.resizeVec", v->arr, v->size*sizeof(void*));
}

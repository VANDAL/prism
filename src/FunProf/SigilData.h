#ifndef _SIGILDATA_H_
#define _SIGILDATA_H_

#include "Prototype.h"
#include "Common.h"
//#include "Vector.h"

typedef struct _ClientFnData ClientFnData;
typedef struct _FnCxtNode FnCxtNode;
typedef struct _FnCxtNodeList FnCxtNodeList;
typedef struct _CallEdges CallEdges;
typedef struct _DepNode DepNode;

/*! \brief Function metadata.
 *
 *  Book-keeping structure for every function called from the client code.
 *
 *  \see fn_node
 *  \see new_fn_node
 */
struct _ClientFnData
{
	/* A unique id of the function. 
	   0 to (2^16 - 2) are valid values.
	   (2^16 - 1) is an undefined fid */
	UInt32 uid;
	UInt32 cxt_node_cnt;

  //fn_node* clg_fn_node; //< metadata from Callgrind
};

/*
 * Both callees and callers are stored so that the path to either the leaves
 * or the root node can be traversed.
 *
 * The array of callees is in calling order, so the array of callees has the
 * order of functions called in that function context node. A function 
 * context node can only have one caller, because each call creates a new 
 * function context.
 */
struct _CallEdges
{
	FnCxtNode* caller;
//	Vector* callees;
};

/*
 * The dependence nodes are the functions that a function context node reads
 * froms. There is no data stored about the nodes that it writes to.
 *
 * The set of nodes that are read from are stored as an ordered tree. Each
 * node's value is the base function's id (recall function contexts are not given
 * unique id's). Then different context nodes of that function are stored as a linked
 * list. The length of these linked lists is expected to be manageable.
 */
struct _DepNode
{
	UInt32 fid;
	DepNode* nextfn;
	DepNode* left_child;
	DepNode* right_child;

	FnCxtNode* thisfn;
	UInt64 unique_bytes_read;
	UInt64 reused_bytes_read;
};

/*
 * A function context node is the unique instance of a function. If function
 * 'Z' was called from function 'Y' and called again from function 'X', then
 * function 'Z' would have two "function context nodes", one for the path
 * from 'X', and one for the path from 'Y'.
 *
 * A function context is a unique path of function context nodes. Together, 
 * all the function contexts create the callgraph. A function context is
 * derived from metadata on callees and callers that exist within a given
 * function context node. Function contexts are not explicitly stored.
 *
 *
 */
struct _FnCxtNode 
{
	/* This id comes from the ClientFnData */
	UInt16 fid; 

	//call graph data
	CallEdges cedges;

	//communication stats
	UInt32 unique_local_cnt; 
	UInt32 nonunique_local_cnt; //non-unique data is reuse data
	DepNode* root;

	//computation stats
	UInt32 iops_cnt;
	UInt32 flops_cnt;
};
	
struct _FnCxtNodeList
{
	FnCxtNodeList* next;
	const FnCxtNode* fn;
}; 

#endif

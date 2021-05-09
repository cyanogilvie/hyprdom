#ifndef _HYPRDOM_INT_H
#define _HYPRDOM_INT_H

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <expat.h>
#include <errno.h>
#include "tclstuff.h"
#include "dedup.h"
#include "tip445.h"
#include "names.h"

#ifdef __builtin_expect
#	define likely(exp)		__builtin_expect(!!(exp), 1)
#	define unlikely(exp)	__builtin_expect(!!(exp), 0)
#else
#	define likely(exp)		(exp)
#	define unlikely(exp)	(exp)
#endif

// Taken from tclInt.h:
#if !defined(INT2PTR) && !defined(PTR2INT)
#   define INT2PTR(p) ((void *)(intptr_t)(p))
#   define PTR2INT(p) ((int)(intptr_t)(p))
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#   define UINT2PTR(p) ((void *)(uintptr_t)(p))
#   define PTR2UINT(p) ((unsigned int)(uintptr_t)(p))
#endif


enum node_type {
	ELEMENT = 0,
	ATTRIBUTE,
	TEXT,
	COMMENT,
	PI,
	NAMESPACE,
	NODE_TYPE_END
};


struct doc {
	Tcl_Obj*			docname;		// name(doc), thing(docname) retrieves this struct
	uint32_t			nodelist_len;	// The allocated space, not all slots are necessarily used
	int32_t				refcount;
	Tcl_HashTable		name_ids;		// Map of name -> numeric id
	Tcl_Obj*			names;			// reverse of name_ids, id'th element is the name
	uint32_t			slot_next;		// Next index into nodes[] to scan for free nodes when allocating new ones
	struct node*		root;
	struct node*		nodes;			// Array of nodes
	struct dedup_pool*	dedup_pool;
};


struct node {
	enum node_type	type;
	struct doc*		doc;
	int32_t			refcount;	// <= 0 for deleted nodes
	uint32_t		epoch;		// 0 initially for all nodes, when a node slot is freed and later reassigned, this is incremented
	uint32_t		parent;
	uint32_t		first_child;
	uint32_t		last_child;
	uint32_t		prior_sibling;
	uint32_t		next_sibling;
	uint32_t		name_id;
	Tcl_Obj*		value;		// ATTRIBUTE: attribute value
								// TEXT: text value
};


// a reference to an entry in doc->nodes, with epoch info.  Fits within Tcl_ObjIntRep
#pragma pack(push, 1)
struct node_slot {
	struct doc*		doc;
	uint32_t		slot;
	uint32_t		epoch;
};
#pragma pack(pop)


struct thread_data {
	int				initialized;
	Tcl_Obj*		name_valid;
};


struct parse_context {
	Tcl_Interp*			interp;
	XML_Parser			parser;
	int					rc;
	struct node_slot	cx;
	Tcl_Obj*			text_node_name;
	Tcl_Obj*			comment_node_name;
	Tcl_Obj*			pi_node_name;
};


void free_internal_rep_hyprdom_node(Tcl_Obj* obj);
void update_string_rep_hyprdom_node(Tcl_Obj* obj);
void free_doc(struct doc** doc);
int Hyprdom_GetDocByName(Tcl_Interp* interp /* may be NULL */, const char* docname, struct doc** doc);
int get_name_id(Tcl_Interp* interp /* may be NULL */, struct doc* doc, const char* name, uint32_t* name_id);
void detach_node(struct node* node);
void DecrRefNode(struct node* node);

#endif

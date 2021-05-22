#ifndef _HYPRDOM_INT_H
#define _HYPRDOM_INT_H

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <expat.h>
#include <errno.h>
#include <limits.h>
#include "tclstuff.h"
#include "dedup.h"
#include "tip445.h"
#include "names.h"
#include "xpath1.h"
#include "radish.h"
//#include "xml.h"

// Taken from tclInt.h:
#if !defined(INT2PTR) && !defined(PTR2INT)
#   define INT2PTR(p) ((void *)(intptr_t)(p))
#   define PTR2INT(p) ((int)(intptr_t)(p))
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#   define UINT2PTR(p) ((void *)(uintptr_t)(p))
#   define PTR2UINT(p) ((unsigned int)(uintptr_t)(p))
#endif

#define MAX_CHAR_LEN_DECIMAL_INTEGER(type) (10*sizeof(type)*CHAR_BIT/33 + 2)

#define TEXT_NODE_NAME		"_text_"
#define PI_NODE_NAME		"_pi_"
#define COMMENT_NODE_NAME	"_comment_"

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
	Tcl_Obj*			names;			// reverse of name_ids, id'th element is the name
	struct radish		radish_names;	// A radix-like trie mapping name -> name_id
	Tcl_HashTable		name_ids;		// Map of name -> numeric id
	uint32_t			slot_next;		// Next index into nodes[] to scan for free nodes when allocating new ones
	uint32_t			root;
	struct node*		nodes;			// Array of nodes
	struct dedup_pool*	dedup_pool;
};


struct node {
	struct doc*		doc;
	Tcl_Obj*		value;		// ATTRIBUTE: attribute value, TEXT: text value
	int32_t			refcount;	// <= 0 for deleted nodes
	uint32_t		epoch;		// 0 initially for all nodes, when a node slot is freed and later reassigned, this is incremented
	uint32_t		parent;
	uint32_t		first_child;
	uint32_t		last_child;
	uint32_t		prior_sibling;
	uint32_t		next_sibling;
	uint32_t		name_id;
	enum node_type	type;
	void*			padding;	// Pad struct node out to 64 bytes (2**6)
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

#include "parse_xml.h"

void breakpoint();

void free_internal_rep_hyprdom_node(Tcl_Obj* obj);
void update_string_rep_hyprdom_node(Tcl_Obj* obj);
void free_doc(struct doc** doc);
void free_node_fast(struct node* node);
int Hyprdom_GetDocByName(Tcl_Interp* interp /* may be NULL */, const char* docname, struct doc** doc);
int get_name_id(Tcl_Interp* interp /* may be NULL */, struct doc* doc, const char* name, uint32_t* name_id);
void detach_node(struct node* node);
void DecrRefNode(struct node* node);
int new_doc(Tcl_Interp* interp /* can be NULL */, int initial_slots, struct doc** doc);
struct node* new_node(struct doc* doc, struct node_slot* slot_ref);
int Hyprdom_GetNameFromObj(Tcl_Interp* interp, struct doc* doc, Tcl_Obj* obj, uint32_t* name_id);
int attach_node(Tcl_Interp* interp /* can be NULL */, struct node_slot* new, struct node_slot* parent, struct node_slot* before);
void free_node(struct node* node);
int alloc_name(Tcl_Interp* interp, struct doc* doc, const char* restrict name, uint32_t len, uint32_t* new_id);
int get_node_slot(Tcl_Interp* interp /* can be NULL */, struct node* node, struct node_slot* slot_ref);
int set_node_name(Tcl_Interp* interp /* may be NULL */, struct node* node, const char* name);

#endif

// TODO: Get rid of the refcount on nodes (they can't be shared between docs since they contain a reference to their doc

#include "hyprdomInt.h"

int g_pagesize = 4096;		/* This will be updated to the real value in Hyprdom_Init, but it's initialized
							 * here to a safe value to prevent accidents if it's used before then */

// Fake node names for node types that don't have them
/*
#define TEXT_NODE_NAME		"<#text>"
#define PI_NODE_NAME		"<#pi>"
#define COMMENT_NODE_NAME	"<#comment>"
*/

#define NAME_START_CHAR	":A-Z_a-z\\xC0-\\xF6\\xF8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD\\u10000-\\uEFFFF"
#define NAME_CHAR		".0-9\\xB7\\u0300-\\u036F\\u203F-\\u2040-"

const char* type_names[] = {
	"ELEMENT",
	"ATTRIBUTE",
	"TEXT",
	"COMMENT",
	"PI",
	"NAMESPACE",
	(const char*)NULL
};

Tcl_ThreadDataKey thread_data_key;

void breakpoint() {}

Tcl_ObjType hyprdom_node = {
	"Hyprdom_node",
	free_internal_rep_hyprdom_node,
	NULL,	/* node references aren't mutable, so should never need dup */
	update_string_rep_hyprdom_node,
	NULL
};

void free_internal_rep_hyprdom_node(Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &hyprdom_node);

	if (ir) {
		struct node_slot*		slot_ref = (struct node_slot*)ir;

		slot_ref->doc = NULL;
		slot_ref->slot = 0;
		slot_ref->epoch = 0;
	}
}

//>>>
void update_string_rep_hyprdom_node(Tcl_Obj* obj) //<<<
{
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &hyprdom_node);
	struct node_slot*	slot_ref = (struct node_slot*)ir;
	Tcl_DString			ds;
	char				numbuf[MAX_CHAR_LEN_DECIMAL_INTEGER(uint32_t)+1];		// Needs to be big enough to hold the maximum string length of %d from a uint32_t, plus the null terminator

	// String rep for a hyprdom node is a 3 element list: docname slot_num epoch
	Tcl_DStringInit(&ds);

	if (slot_ref->doc) {
		Tcl_DStringAppendElement(&ds, Tcl_GetString(slot_ref->doc->docname));
	} else {
		Tcl_DStringAppendElement(&ds, "<deleted>");
	}
	sprintf(numbuf, "%d", slot_ref->slot);
	Tcl_DStringAppendElement(&ds, numbuf);
	sprintf(numbuf, "%d", slot_ref->epoch);
	Tcl_DStringAppendElement(&ds, numbuf);

	obj->length = Tcl_DStringLength(&ds);
	obj->bytes = ckalloc(obj->length+1);
	memcpy(obj->bytes, Tcl_DStringValue(&ds), obj->length);
	obj->bytes[obj->length] = 0;

	Tcl_DStringFree(&ds);
}

//>>>

int Hyprdom_GetNodeSlotFromObj(Tcl_Interp* interp, Tcl_Obj* obj, struct node_slot** slot_ref) //<<<
{
	int					rc = TCL_OK;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &hyprdom_node);

	if (ir == NULL) {
		struct node_slot	new_slot_ref;
		Tcl_Obj**			ov;
		int					oc;
		int					tmp;

		TEST_OK_LABEL(done, rc, Tcl_ListObjGetElements(interp, obj, &oc, &ov));

		if (oc != 3) {
			rc = TCL_ERROR;
			if (interp)
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("node references must be lists of 3 elements"));
			goto done;
		}

		TEST_OK_LABEL(done, rc, Hyprdom_GetDocByName(interp, Tcl_GetString(ov[0]), &new_slot_ref.doc));

		TEST_OK_LABEL(done, rc, Tcl_GetIntFromObj(interp, ov[1], &tmp));

		new_slot_ref.slot = tmp;

		TEST_OK_LABEL(done, rc, Tcl_GetIntFromObj(interp, ov[2], &tmp));

		new_slot_ref.epoch = tmp;

		Tcl_FreeInternalRep(obj);
		Tcl_StoreInternalRep(obj, &hyprdom_node, (Tcl_ObjInternalRep*)&new_slot_ref);
		ir = Tcl_FetchInternalRep(obj, &hyprdom_node);
	}

	*slot_ref = (struct node_slot*)ir;

	if ((*slot_ref)->epoch != (*slot_ref)->doc->nodes[(*slot_ref)->slot].epoch) {
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Node was deleted"));
		goto done;
	}

done:
	return rc;
}

//>>>
Tcl_Obj* Hyprdom_NewSlotRef(const struct node_slot* slot_ref) //<<<
{
	Tcl_Obj*			res = Tcl_NewObj();
	Tcl_ObjInternalRep	ir;

	memcpy(&ir, slot_ref, sizeof(*slot_ref));
	Tcl_FreeInternalRep(res);
	Tcl_StoreInternalRep(res, &hyprdom_node, &ir);
	Tcl_InvalidateStringRep(res);

	return res;
}

//>>>

// Object that refers to a node name (ie. element name, attrib name)
Tcl_ObjType hyprdom_name = {
	"Hyprdom_name",
	NULL,
	NULL,	/* names aren't mutable, so should never need dup */
	NULL,	/* we never invalidate our stringrep */
	NULL
};

int Hyprdom_GetNameFromObj(Tcl_Interp* interp, struct doc* doc, Tcl_Obj* obj, uint32_t* name_id) //<<<
{
	int					rc = TCL_OK;
	Tcl_ObjInternalRep*	ir = Tcl_FetchInternalRep(obj, &hyprdom_name);

	if (ir == NULL) {
		const char*			name = Tcl_GetString(obj);
		Tcl_ObjInternalRep	newir;
		uint32_t			id;

		TEST_OK_LABEL(done, rc, get_name_id(interp, doc, name, &id));

		newir.ptrAndLongRep.ptr = doc;
		newir.ptrAndLongRep.value = id;

		//Tcl_FreeInternalRep(obj);
		Tcl_StoreInternalRep(obj, &hyprdom_name, &newir);
		ir = Tcl_FetchInternalRep(obj, &hyprdom_name);
	} else if (ir->ptrAndLongRep.ptr != doc) {
		const char*	name = Tcl_GetString(obj);
		uint32_t	id;

		// Name reference is stale, refers to a different doc
		TEST_OK_LABEL(done, rc, get_name_id(interp, doc, name, &id));

		ir->ptrAndLongRep.ptr = doc;
		ir->ptrAndLongRep.value = id;
	}

	*name_id = ir->ptrAndLongRep.value;

done:
	return rc;
}

//>>>

void free_thread_data(ClientData cdata) //<<<
{
	struct thread_data*	td = (struct thread_data*)cdata;

	DBG("Cleaning up thread data for %s\n", name("td:", Tcl_GetCurrentThread()));
	replace_tclobj(&td->name_valid, NULL);
	td->initialized = 0;
}

//>>>
struct thread_data* get_thread_data() //<<<
{
	struct thread_data*	res = NULL;
	res = Tcl_GetThreadData(&thread_data_key, sizeof(struct thread_data));

	if (!res->initialized) {
		DBG("Initializing thread data for %s\n", name("td:", Tcl_GetCurrentThread()));
		// TODO: Is it safe to share these regexp Tcl_Objs between interps in a thread?
		replace_tclobj(&res->name_valid, Tcl_NewStringObj("^[" NAME_START_CHAR "][" NAME_START_CHAR NAME_CHAR "]*$", -1));
		Tcl_CreateThreadExitHandler(free_thread_data, res);
		res->initialized = 1;
	}

	return res;
}

//>>>
int Hyprdom_GetDocByName(Tcl_Interp* interp /* may be NULL */, const char* docname, struct doc** doc) //<<<
{
	int					rc = TCL_OK;

	*doc = thing(docname);
	if (*doc == NULL) {
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Document %s does not exist in this thread", docname));
		goto done;
	}

done:
	return rc;
}

//>>>
int alloc_name(Tcl_Interp* interp, struct doc* doc, const char* restrict name, uint32_t len, uint32_t* new_id) //<<<
{
	int			rc = TCL_OK;
	Tcl_Obj*	nameobj = NULL;

	replace_tclobj(&nameobj, get_string(doc->dedup_pool, (const char*)name, len));
	TEST_OK_LABEL(done, rc, Tcl_ListObjLength(interp, doc->names, (int*)new_id));
	TEST_OK_LABEL(done, rc, Tcl_ListObjAppendElement(interp, doc->names, nameobj));

done:
	replace_tclobj(&nameobj, NULL);
	return rc;
}

//>>>
int get_name_id(Tcl_Interp* interp /* may be NULL */, struct doc* doc, const char* name, uint32_t* name_id) //<<<
{
	int				rc = TCL_OK;
	Tcl_HashEntry*	he = NULL;
	int				isnew=0;
	uint32_t		id;
	Tcl_Obj*		nameObj = NULL;

	he = Tcl_CreateHashEntry(&doc->name_ids, name, &isnew);

	if (isnew) {
		struct thread_data*	td = get_thread_data();
		int			valid;
		size_t		tmp;

		replace_tclobj(&nameObj, Tcl_NewStringObj((const char*)name, -1));

		// Verify that the name is valid for tag and attribute names
		if (-1 == (valid = Tcl_RegExpMatchObj(interp, nameObj, td->name_valid))) {
			rc = TCL_ERROR;
			goto done;
		}
		if (!valid) {
			rc = TCL_ERROR;
			if (interp)
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("XML name is not valid: \"%s\"", name));
			goto done;
		}

		TEST_OK_LABEL(done, rc, alloc_name(interp, doc, name, -1, &id));
		tmp = id;
		Tcl_SetHashValue(he, UINT2PTR(tmp));
	} else {
		size_t			tmp;
		tmp = PTR2UINT(Tcl_GetHashValue(he));
		id = tmp;
	}

	*name_id = id;

done:
	replace_tclobj(&nameObj, NULL);

	if (he && isnew && rc != TCL_OK) {
		// Partially allocated new entry, clean it up
		Tcl_DeleteHashEntry(he);
	}

	return rc;
}

//>>>
int set_node_name(Tcl_Interp* interp /* may be NULL */, struct node* node, const char* name) // interp can be NULL <<<
{
	int				rc = TCL_OK;
	uint32_t		id;

	TEST_OK_LABEL(done, rc, get_name_id(interp, node->doc, name, &id));

	node->name_id = id;

done:
	return rc;
}

//>>>
int new_doc(Tcl_Interp* interp /* can be NULL */, int initial_slots, struct doc** doc) //<<<
{
	int					rc = TCL_OK;
	size_t				memsize;
	int					alloc_rc;
	int					i;

	*doc = ckalloc(sizeof(**doc));
	if (unlikely(doc == NULL)) {
		alloc_rc = errno;
		goto alloc_failed;
	}
	memset(*doc, 0, sizeof(**doc));

	replace_tclobj(&(*doc)->docname, Tcl_NewStringObj(name("doc:", *doc), -1));

	if (initial_slots == 0) {
		// Work out how many slots to allocate to fill a page with the initial doc struct
		initial_slots = (g_pagesize - sizeof(*doc)) / sizeof(struct node);
	}

	memsize = sizeof(struct node)*initial_slots;
	//DBG("Allocating %ld bytes for node storage, accomodating %d slots\n", memsize, initial_slots);

#if 0
	alloc_rc = posix_memalign((void*)&(*doc)->nodes, g_pagesize, memsize);
	if (unlikely(alloc_rc != 0)) goto alloc_failed;
#else
	(*doc)->nodes = ckalloc(memsize);
#endif

	memset((*doc)->nodes, 0, memsize);
	for (i=0; i<initial_slots; i++)
		(*doc)->nodes[i].doc = *doc;

	radish_init(&(*doc)->radish_names, 128);
	Radish_IncrRef(&(*doc)->radish_names);

	Tcl_InitHashTable(&(*doc)->name_ids, TCL_STRING_KEYS);
	replace_tclobj(&(*doc)->names, Tcl_NewListObj(0, NULL));

	(*doc)->nodelist_len = initial_slots;
	(*doc)->root = 1;				// slot number 0 is reserved to mean NULL
	//TEST_OK_LABEL(done, rc, set_node_name(interp, &(*doc)->nodes[(*doc)->root], root_name));
	(*doc)->slot_next = 2;
#if DEDUP
	(*doc)->dedup_pool = new_dedup_pool(interp);
#else
	(*doc)->dedup_pool = NULL;
#endif

	return rc;

alloc_failed:
	if (interp) {
		switch (alloc_rc) {
			case ENOMEM:
				// TODO: save a pre-allocated Tcl_Obj for this, we may not have enough to allocate even this?
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Out of memory"));
				break;
			case EINVAL:
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Failure to allocate memory: Requested alignment invalid"));
				break;
			default:
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Failure to allocate memory: %d", alloc_rc));
		}
	}
	return TCL_ERROR;
}

//>>>
int check_node_slot(Tcl_Interp* interp /* can be NULL */, struct node_slot* slot_ref) //<<<
{
	int				rc = TCL_OK;
	struct doc*		doc = slot_ref->doc;
	struct node*	node = &doc->nodes[slot_ref->slot];	// May not be safe to dereference yet

	if (unlikely(slot_ref->slot >= doc->nodelist_len)) {
		// off the end of the allocated node slots
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("before node id is invalid: (%d, %d)", slot_ref->slot, slot_ref->epoch));
		goto done;
	}

	// node is now safe to dereference

	if (unlikely(node->epoch != slot_ref->epoch)) {
		// the before node reference is stale (refers to a deleted node)
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("before node no longer exists: (%d, %d)", slot_ref->slot, slot_ref->epoch));
		goto done;
	}

done:
	return rc;
}

//>>>
int get_node_slot(Tcl_Interp* interp /* can be NULL */, struct node* node, struct node_slot* slot_ref) //<<<
{
	int				rc = TCL_OK;
	struct doc*		doc = node->doc;
	int				myslot = node - doc->nodes;

	if (myslot > doc->nodelist_len) {
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Node does not belong to its document!"));
		goto done;
	}

	slot_ref->doc   = doc;
	slot_ref->slot  = node - doc->nodes;
	slot_ref->epoch = node->epoch;

done:
	return rc;
}

//>>>
int GetNodeFromSlotRef(Tcl_Interp* interp /* can be NULL */, struct node_slot* slot_ref, struct node** node) //<<<
{
	int				rc = TCL_OK;
	struct node*	tmp = NULL;

	TEST_OK_LABEL(done, rc, check_node_slot(interp, slot_ref));

	tmp = &slot_ref->doc->nodes[slot_ref->slot];

	if (tmp->epoch != slot_ref->epoch) {
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Node %p %d/%d has been deleted",
						slot_ref->doc, slot_ref->slot, slot_ref->epoch));
		goto done;
	}

	*node = tmp;
done:
	return rc;
}

//>>>
int attach_node(Tcl_Interp* interp /* can be NULL */, struct node_slot* new, struct node_slot* parent, struct node_slot* before) // before==NULL - append to children <<<
{
	int				rc = TCL_OK;
	const int		myslot = new->slot;
	struct node*	node = NULL;
	struct node*	parent_node = NULL;
	struct node*	nodes = parent->doc->nodes;

	if (0 && unlikely(parent->slot == 0)) {
		Tcl_Obj*	name = NULL;
		const char*	namestr = NULL;

		rc = TCL_ERROR;
		if (TCL_OK == Tcl_ListObjIndex(NULL, new->doc->names, nodes[new->slot].name_id, &name)) {
			namestr = Tcl_GetString(name);
		} else {
			namestr = "<error>";
		}
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Cannot attach %s node \"%s\" to the root",
					type_names[nodes[new->slot].type], namestr));
		goto done;
	}

	//TEST_OK_LABEL(done, rc, GetNodeFromSlotRef(interp, new,    &node       ));
	//TEST_OK_LABEL(done, rc, GetNodeFromSlotRef(interp, parent, &parent_node));
	node = &nodes[new->slot];
	parent_node = &nodes[parent->slot];

	if (unlikely(node->doc != parent_node->doc)) {
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("node does not belong to the same document as parent"));
		goto done;
	}

	if (unlikely(parent_node->type != ELEMENT)) {
		rc = TCL_ERROR;
		if (interp) {
			if (parent_node->type >= NODE_TYPE_END) {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Parent node type is invalid %d, memory corruption?",
							parent_node->type));
			} else {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Cannot attach children to %s nodes",
							type_names[parent_node->type]));
			}
		}
		goto done;
	}

	if (unlikely(node->type >= NODE_TYPE_END)) {
		rc = TCL_ERROR;
		if (interp)
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("Invalid node type %d, memory corruption?", node->type));
		goto done;
	}

	if (likely(before == NULL)) { // Append to children
		uint32_t		tail;

		if (unlikely(node->parent)) detach_node(node);

		tail = parent_node->last_child;

		if (likely(tail)) {
			parent_node->doc->nodes[tail].next_sibling = myslot;
			node->prior_sibling = tail;
		} else {
			// This is the first child
			node->prior_sibling = 0;
			parent_node->first_child = myslot;
		}

		node->next_sibling = 0;
		parent_node->last_child = myslot;
		node->parent = parent->slot;
	} else {
		//struct node*	before_node = NULL;
		//TEST_OK_LABEL(done, rc, GetNodeFromSlotRef(interp, before, &before_node));

		struct node*	before_node = &nodes[before->slot];

		if (before_node->doc != parent->doc) {
			rc = TCL_ERROR;
			if (interp)
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("before node does not belong to the same doc as parent"));
			goto done;
		}

		if (before_node->parent != parent->slot) {
			rc = TCL_ERROR;
			if (interp)
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("before node is not a child of parent"));
			goto done;
		}

		if (node->parent) detach_node(node);
		node->next_sibling  = before->slot;
		node->prior_sibling = before_node->prior_sibling;
		node->parent        = parent->slot;
		before_node->prior_sibling = myslot;
		if (parent_node->first_child == before->slot)
			parent_node->first_child = myslot;
	}

done:
	return rc;
}

//>>>
void detach_node(struct node* node) //<<<
{
	struct node*	nodes = node->doc->nodes;

	if (node->parent) {
		struct node*	parent = &nodes[node->parent];
		const uint32_t	myslot = node - nodes;
		if (parent->first_child == myslot)	parent->first_child	= node->next_sibling;
		if (parent->last_child  == myslot)	parent->last_child	= node->prior_sibling;
	}

	if (node->prior_sibling)	nodes[node->prior_sibling].next_sibling = node->next_sibling;
	if (node->next_sibling)		nodes[node->next_sibling].prior_sibling = node->prior_sibling;
}

//>>>
void free_node(struct node* node) //<<<
{
	struct node*	nodes = node->doc->nodes;
	const uint32_t	myslot = node - nodes;
	uint32_t		childslot;

	detach_node(node);

	childslot = node->first_child;
	while (likely(childslot)) {
		struct node*	child = &nodes[childslot];
		childslot = child->next_sibling;
		free_node_fast(child);
	}
	node->first_child = 0;
	node->last_child = 0;

	replace_tclobj(&node->value, NULL);
	node->epoch++;
	node->refcount = -1;

	if (node->doc->slot_next > myslot)
		node->doc->slot_next = myslot;
}

//>>>
void free_node_fast(struct node* node) // Special case: our parent and its whole sub-tree are going away, no need to detach and adjust all the links <<<
{
	struct node*	nodes = node->doc->nodes;
	const uint32_t	myslot = node - nodes;
	uint32_t		childslot;

	childslot = node->first_child;
	while (likely(childslot)) {
		struct node*	child = &nodes[childslot];
		childslot = child->next_sibling;
		free_node_fast(child);
	}
	node->first_child = 0;
	node->last_child = 0;

	replace_tclobj(&node->value, NULL);
	node->epoch++;
	node->refcount = -1;

	if (node->doc->slot_next > myslot)
		node->doc->slot_next = myslot;
}

//>>>
inline void DecrRefNode(struct node* node) //<<<
{
	if (node->refcount > 0) {
		node->refcount--;
		if (node->refcount <= 0)
			free_node(node);
	}
}

//>>>
static inline void IncrRefNode(struct node* node) //<<<
{
	if (node->refcount < 0)
		Tcl_Panic("Trying to increment the reference on a hyprdom node with a negative refcount: %d", node->refcount);

	node->refcount++;
}

//>>>
void free_doc(struct doc** doc) //<<<
{
	uint32_t		i;
	struct node*	nodes = (*doc)->nodes;
	const uint32_t	nodelist_len = (*doc)->nodelist_len;

	/* Special case: the whole doc is being freed, all nodes (which can't be
	 * shared with other docs) will be freed, so no need to walk the tree,
	 * just blast through the array in a memory-prefetch friendly way and
	 * decref the node->value if refcount > 0
	 */
	for (i=1; i<nodelist_len; i++)
		if (nodes[i].value && nodes[i].refcount > 0 && nodes[i].value->refCount > 0)
			Tcl_DecrRefCount(nodes[i].value);

	ckfree((*doc)->nodes);
	(*doc)->nodes = NULL;

	free_thing(*doc);
	Radish_DecrRef(&(*doc)->radish_names);
	replace_tclobj(&(*doc)->docname, NULL);
	Tcl_DeleteHashTable(&(*doc)->name_ids);
	replace_tclobj(&(*doc)->names, NULL);

	(*doc)->nodelist_len = 0;
	(*doc)->root = 0;
	(*doc)->refcount = -1;
	if ((*doc)->dedup_pool) {
		free_dedup_pool((*doc)->dedup_pool); (*doc)->dedup_pool = NULL;
	}
	ckfree(*doc); *doc = NULL;
}

//>>>
struct node* new_node(struct doc* doc, struct node_slot* slot_ref) //<<<
{
	struct node*	new = NULL;
	uint32_t		i;
	//int				alloc_rc;

	for (i=doc->slot_next; i<doc->nodelist_len; i++) {
		if (doc->nodes[i].refcount <= 0) {
			new = &doc->nodes[i];
			break;
		}
	}

	if (unlikely(i == doc->nodelist_len)) { // Ran out of slots, need to grow
		// +1: compensate for integer division, otherwise we're initially truncated to 0
		int				pages = ((sizeof(struct node) * doc->nodelist_len) / g_pagesize) + 1;
		size_t			memsize;
		//struct node*	newnodes = NULL;
		uint32_t		new_slots;
		uint32_t		j;

		// Grow by doubling until we hit 10 MiB, then grow linearly
		if (pages * g_pagesize < 10485760) {
			pages *= 2;
		} else {
			pages += 10485760 / g_pagesize;
		}
		new_slots = (pages*g_pagesize) / sizeof(struct node);
		memsize = sizeof(struct node)*new_slots;
		//DBG("Doc %s hit nodelimit %d, growing to %d (%ld bytes)\n", Tcl_GetString(doc->docname), doc->nodelist_len, new_slots, memsize);
#if 0
		alloc_rc = posix_memalign((void*)&newnodes, g_pagesize, memsize);
		if (unlikely(alloc_rc != 0)) goto alloc_failed;
		memcpy(newnodes, doc->nodes, doc->nodelist_len * sizeof(struct node));
		memset(newnodes+i, 0, sizeof(struct node)*(new_slots-i));
		free(doc->nodes); doc->nodes = newnodes;
#else
		doc->nodes = ckrealloc(doc->nodes, memsize);
		memset(doc->nodes+i, 0, sizeof(struct node)*(new_slots-i));
#endif
		doc->nodelist_len = new_slots;

		for (j=i; j<doc->nodelist_len; j++)
			doc->nodes[j].doc = doc;

		new = &doc->nodes[i];
	}

	new->refcount = 1;
	slot_ref->doc = doc;
	slot_ref->slot = i;
	slot_ref->epoch = new->epoch;
	doc->slot_next = i+1;

	return new;

	/*
alloc_failed:
	// TODO: Handle this better
	return NULL;
	*/
}

//>>>
static int new_doc_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct doc*			doc = NULL;
	struct node_slot	slot_ref;

	CHECK_ARGS(1, "rootname");

	TEST_OK_LABEL(done, rc, new_doc(interp, 0, &doc));
	TEST_OK_LABEL(done, rc, set_node_name(interp, &doc->nodes[doc->root], (const char*)Tcl_GetString(objv[1])));
	TEST_OK_LABEL(done, rc, get_node_slot(interp, &doc->nodes[doc->root], &slot_ref));
	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&slot_ref));

done:
	return rc;
}

//>>>
static int free_doc_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	node_slot = NULL;

	CHECK_ARGS(1, "node");

	TEST_OK_LABEL(done, rc, Hyprdom_GetNodeSlotFromObj(interp, objv[1], &node_slot));
	free_doc(&node_slot->doc);

done:
	return rc;
}

//>>>

/* Parsing from XML */
static int parse_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	struct node_slot		root_slot = {};
	const char*				xmldata = NULL;
	int						xmllen;
	int						use_expat = 0;
	int						i;
	struct parse_context	pc = {
		.interp					= interp,
		.parser					= NULL,
		.rc						= TCL_OK,
		.text_node_name_id		= 0,
		.comment_node_name_id	= 0,
		.pi_node_name_id		= 0,
		.slots_estimate			= 0,
		.cx	= {
			.doc	= NULL,
			.slot	= 0,
			.epoch	= 0		// epoch is always 0 for a new document's nodes
		}
	};
	static char*	opts[] = {
		"-expat",
		NULL
	};
	enum optval {
		USE_EXPAT
	};

	if ((pc.rc = setjmp(pc.on_error))) goto done;

	if (objc < 2 && objc > 3)
		CHECK_ARGS(1, "?options? xml");

	// Process the options
	for (i=1; i<objc-1; i++) {
		int		opt;

		TEST_OK_LABEL(done, pc.rc, Tcl_GetIndexFromObj(interp, objv[i], opts, "option", TCL_EXACT, &opt));

		switch (opt) {
			case USE_EXPAT:
				use_expat = 1;
				break;
			default:
				pc.rc = TCL_ERROR;
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Unhandled option index: %d", opt));
				goto done;
		}
	}

	xmldata = Tcl_GetStringFromObj(objv[objc-1], &xmllen);

	// Guess that we'll have about 1 node per 10 chars in the XML
	pc.slots_estimate = xmllen / 10;

	TEST_OK_LABEL(done, pc.rc, new_doc(pc.interp, pc.slots_estimate, &pc.cx.doc));

	TEST_OK_LABEL(done, pc.rc, alloc_name(interp, pc.cx.doc, TEXT_NODE_NAME, -1, &pc.text_node_name_id));
	TEST_OK_LABEL(done, pc.rc, alloc_name(interp, pc.cx.doc, COMMENT_NODE_NAME, -1, &pc.comment_node_name_id));
	TEST_OK_LABEL(done, pc.rc, alloc_name(interp, pc.cx.doc, PI_NODE_NAME, -1, &pc.pi_node_name_id));

	if (use_expat) { // Parse with expat <<<
		//DBG("parse_cmd\n");
		pc.parser = XML_ParserCreate(NULL);

		XML_SetUserData(pc.parser, &pc);

		// TODO: how to ensure that XML_Char is char and utf-8 encoded?
		XML_SetElementHandler              (pc.parser, pcb_start_element, pcb_end_element);
		XML_SetCharacterDataHandler        (pc.parser, pcb_text);
		XML_SetProcessingInstructionHandler(pc.parser, pcb_pi);
		XML_SetCommentHandler              (pc.parser, pcb_comment);

		if (XML_STATUS_ERROR == XML_Parse(pc.parser, xmldata, xmllen, 1)) {
			if (pc.rc == TCL_OK) {
				pc.rc = TCL_ERROR;
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s at line %lu",
						XML_ErrorString(XML_GetErrorCode(pc.parser)),
						XML_GetCurrentLineNumber(pc.parser)));
			} else {
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s: %s at line %lu",
						XML_ErrorString(XML_GetErrorCode(pc.parser)),
						Tcl_GetString(Tcl_GetObjResult(interp)),
						XML_GetCurrentLineNumber(pc.parser)));
			}
			goto done;
		}
		//>>>
	} else { // Parse using internal parse <<<
		TEST_OK_LABEL(done, pc.rc, parse_xml_re(&pc, (const char*)xmldata, xmllen));
		//>>>
	}

	if (likely(pc.rc == TCL_OK)) {
		if (unlikely(pc.cx.doc == NULL)) {
			pc.rc = TCL_ERROR;
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("No document created by parse"));
			goto done;
		}
		TEST_OK_LABEL(done, pc.rc, get_node_slot(interp, &pc.cx.doc->nodes[pc.cx.doc->root], &root_slot));
		Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&root_slot));
	} else {
		free_doc(&pc.cx.doc);
		pc.cx.doc = NULL;
	}

done:
	//DBG("Estimated %d slots to be required, used: %d\n", pc.slots_estimate, pc.cx.doc->slot_next);
	if (pc.parser) {
		XML_ParserFree(pc.parser);
		pc.parser = NULL;
	}
	pc.cx.doc = NULL;
	pc.interp = NULL;
	return pc.rc;
}

//>>>

/* Serializing to XML */
static int serialize_node_as_xml(Tcl_Interp* interp /* may be NULL */, Tcl_DString* xml, struct doc* doc, uint32_t slot) //<<<
{
	int				rc = TCL_OK;
	struct node*	nodes = doc->nodes;
	struct node*	node = &nodes[slot];
	const char*		data = NULL;
	int				datalen;

	/*
	{
		Tcl_Obj* nodenameobj = NULL;
		Tcl_ListObjIndex(interp, doc->names, node->name_id, &nodenameobj);
		DBG("--> serialize_node_as_xml: %s <%s>\n", type_names[node->type], Tcl_GetString(nodenameobj));
	}
	*/

	switch (node->type) {
		case ELEMENT:
			{
				const char*	tagstr = NULL;
				int			taglen;
				uint32_t	child;
				int			empty = 1;
				Tcl_Obj**	names = NULL;
				int			namesc;

				TEST_OK_LABEL(done, rc, Tcl_ListObjGetElements(interp, doc->names, &namesc, &names));

				tagstr = Tcl_GetStringFromObj(names[node->name_id], &taglen);

				Tcl_DStringAppend(xml, "<", 1);
				// All entries in names[] should have been checked at creation time
				Tcl_DStringAppend(xml, tagstr, taglen);

				child = node->first_child;
				while (child) {
					struct node*	child_node = &nodes[child];
					if (child_node->type == ATTRIBUTE) {
						const char*	s = NULL;
						int			slen;

						Tcl_DStringAppend(xml, " ", 1);
						s = Tcl_GetStringFromObj(names[child_node->name_id], &slen);
						// All entries in names[] should have been checked at creation time
						Tcl_DStringAppend(xml, s, slen);
						Tcl_DStringAppend(xml, "=\"", 2);
						s = Tcl_GetStringFromObj(child_node->value, &slen);
						{
							const char*			start = s;
							const char*			p = start;
							const char*const	end = start + slen;

							while (likely(p < end)) {
								const char c = *p;

								// Any byte higher than this we output verbatim, so avoid all the
								// tests below for the common case (letters)
								if (likely(c > 0x3c)) {
									p++;
									continue;
								}

								switch (c) {
									case '<':
										if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
										start = p+1;
										Tcl_DStringAppend(xml, "&lt;", 4);
										break;
									case '&':
										if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
										start = p+1;
										Tcl_DStringAppend(xml, "&amp;", 5);
										break;
									case '"':
										if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
										start = p+1;
										Tcl_DStringAppend(xml, "&quot;", 6);
										break;
									case 0x09:
									case 0x0A:
									case 0x0D:
										break;
									default:
										if (unlikely(c < 0x20)) {
											// Filter out other C0 control characters to comply with the spec
											// (which states that they may not occur in the document in any form)
											if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
											start = p;
										}
								}
								p++;
							}
							if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
						}
						Tcl_DStringAppend(xml, "\"", 1);
					} else {
						empty = 0;
					}
					child = child_node->next_sibling;
				}

				if (empty) {
					Tcl_DStringAppend(xml, "/>", 2);
				} else {
					Tcl_DStringAppend(xml, ">", 1);

					child = node->first_child;
					while (child) {
						struct node*	child_node = &nodes[child];
						if (child_node->type != ATTRIBUTE)
							TEST_OK_LABEL(done, rc, serialize_node_as_xml(interp, xml, doc, child));
						child = child_node->next_sibling;
					}

					Tcl_DStringAppend(xml, "</", 2);
					Tcl_DStringAppend(xml, tagstr, taglen);
					Tcl_DStringAppend(xml, ">", 1);
				}
			}
			break;
		case TEXT:
			{
				const char*			start = Tcl_GetStringFromObj(node->value, &datalen);
				const char*			p = start;
				const char*const	end = start + datalen;

				// TODO: pre-scan the text to see if there is a high concentration of < or &,
				// and if so output as a CDATA section instead?

				while (likely(p < end)) {
					const char c = *p;

					// Any byte higher than this we output verbatim, so avoid all the
					// tests below for the common case (letters)
					if (likely(c > 0x3c)) {
						p++;
						continue;
					}

					switch (c) {
						case '<':
							if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
							start = p+1;
							Tcl_DStringAppend(xml, "&lt;", 4);
							break;
						case '&':
							if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
							start = p+1;
							Tcl_DStringAppend(xml, "&amp;", 5);
							break;
						case 0x09:
						case 0x0A:
						case 0x0D:
							break;
						default:
							if (unlikely(c < 0x20)) {
								// Filter out other C0 control characters to comply with the spec
								// (which states that they may not occur in the document in any form)
								if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
								start = p;
							}
					}
					p++;
				}
				if (likely(p > start)) Tcl_DStringAppend(xml, start, p-start);
			}
			break;
		case COMMENT:
			data = Tcl_GetStringFromObj(node->value, &datalen);
			Tcl_DStringAppend(xml, "<!--", 4);
			// TODO: filter invalid C0 chars
			// comment content data is checked at creation time to not contain --
			Tcl_DStringAppend(xml, data, datalen);
			Tcl_DStringAppend(xml, "-->", 3);
			break;
		case PI:
			{
				Tcl_Obj**	ov = NULL;
				int			oc;

				TEST_OK_LABEL(done, rc, Tcl_ListObjGetElements(interp, node->value, &oc, &ov));
				Tcl_DStringAppend(xml, "<?", 4);
				// TODO: filter invalid C0 chars
				data = Tcl_GetStringFromObj(ov[0], &datalen);
				Tcl_DStringAppend(xml, data, datalen);
				Tcl_DStringAppend(xml, " ", 1);
				// TODO: filter invalid C0 chars
				data = Tcl_GetStringFromObj(ov[1], &datalen);
				Tcl_DStringAppend(xml, data, datalen);
				Tcl_DStringAppend(xml, "?>", 3);
			}
			break;
		case ATTRIBUTE:
		case NAMESPACE:
			// Do nothing
			break;
		default:
			rc = TCL_ERROR;
			if (interp)
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Unexpected node type while serializing hyprdom node as XML: %d", node->type));
			goto done;
	}

done:
	/*
	{
		Tcl_Obj* nodenameobj = NULL;
		Tcl_ListObjIndex(interp, doc->names, node->name_id, &nodenameobj);
		DBG("<-- serialize_node_as_xml: %s </%s>, rc: %d\n", type_names[node->type], Tcl_GetString(nodenameobj), rc);
	}
	*/
	return rc;
}

//>>>
static int asXML_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	myslot = NULL;
	Tcl_DString			xml;

	CHECK_ARGS(1, "node");

	TEST_OK_LABEL(done, rc, Hyprdom_GetNodeSlotFromObj(interp, objv[1], &myslot));

	TIME("serialize",
	Tcl_DStringInit(&xml);

	TEST_OK_LABEL(done, rc, serialize_node_as_xml(interp, &xml, myslot->doc, myslot->slot));

	Tcl_DStringResult(interp, &xml);

done:
	Tcl_DStringFree(&xml);
	);
	return rc;
}

//>>>

/* Construction by script commands */
static void free_autonode_nodename(ClientData cdata) //<<<
{
	replace_tclobj((Tcl_Obj**)&cdata, NULL);
}

//>>>
static int autonode_tag_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	Tcl_Obj*			tagname = (Tcl_Obj*)cdata;
	struct node_slot*	parent = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	struct node*		node = NULL;
	uint32_t			name_id;
	struct node_slot	myslot;
	int					i, attribc;
	struct node_slot*	hold_slot = NULL;

	if (parent == NULL) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No parent node context"));
		goto done;
	}

	TEST_OK_LABEL(done, rc, Hyprdom_GetNameFromObj(interp, parent->doc, tagname, &name_id));

	node = new_node(parent->doc, &myslot);
	node->name_id = name_id;
	TEST_OK_LABEL(done, rc, attach_node(interp, &myslot, parent, NULL));

	attribc = (objc-1) / 2;
	for (i=1; i<attribc*2; i+=2) {
		Tcl_Obj*			attrib_name  = objv[i];
		Tcl_Obj*			attrib_value = objv[i+1];
		uint32_t			attrib_name_id;
		struct node*		attrib_node = NULL;
		struct node_slot	attrib_slot;

		TEST_OK_LABEL(done, rc, Hyprdom_GetNameFromObj(interp, parent->doc, attrib_name, &attrib_name_id));

		attrib_node = new_node(parent->doc, &attrib_slot);
		attrib_node->name_id = attrib_name_id;
		replace_tclobj(&attrib_node->value, attrib_value);
		TEST_OK_LABEL(done, rc, attach_node(interp, &attrib_slot, &myslot, NULL));
	}

	hold_slot = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	Tcl_SetAssocData(interp, "hyprdom_current_node", NULL, &myslot);

	if (i < objc) {
		// A script arg remains
		// TODO: implement as NR command
		TEST_OK_LABEL(done, rc, Tcl_EvalObjEx(interp, objv[objc-1], 0));
	}

	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&myslot));

done:
	if (hold_slot)
		Tcl_SetAssocData(interp, "hyprdom_current_node", NULL, hold_slot);
	return rc;
}

//>>>
static int autonode_entity_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	Tcl_Obj*			entity = (Tcl_Obj*)cdata;	// entity is like "&lt;", null terminated
	struct node_slot*	parent = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	struct node*		node = NULL;
	uint32_t			name_id;
	struct node_slot	myslot;

	if (parent == NULL) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No parent node context"));
		goto done;
	}

	TEST_OK_LABEL(done, rc, Hyprdom_GetNameFromObj(interp, parent->doc, (Tcl_Obj*)cdata, &name_id));

	node = new_node(parent->doc, &myslot);
	node->type = TEXT;
	TEST_OK_LABEL(done, rc, set_node_name(interp, node, TEXT_NODE_NAME));
	replace_tclobj(&node->value, entity);
	TEST_OK_LABEL(done, rc, attach_node(interp, &myslot, parent, NULL));

	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&myslot));

done:
	return rc;
}

//>>>
static int autonode_txt_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	parent = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	struct node*		node = NULL;
	uint32_t			name_id;
	struct node_slot	myslot;

	CHECK_ARGS(1, "text");

	if (parent == NULL) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No parent node context"));
		goto done;
	}

	TEST_OK_LABEL(done, rc, Hyprdom_GetNameFromObj(interp, parent->doc, (Tcl_Obj*)cdata, &name_id));

	node = new_node(parent->doc, &myslot);
	node->type = TEXT;
	TEST_OK_LABEL(done, rc, set_node_name(interp, node, TEXT_NODE_NAME));
	replace_tclobj(&node->value, objv[1]);
	TEST_OK_LABEL(done, rc, attach_node(interp, &myslot, parent, NULL));

	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&myslot));

done:
	return rc;
}

//>>>
static int autonode_comment_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	parent = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	struct node*		node = NULL;
	uint32_t			name_id;
	struct node_slot	myslot;
	int					i;
	const char*			commentstr = NULL;
	int					commentlen;

	CHECK_ARGS(1, "comment");

	// Comment contents cannot contain the sequence "--"
	commentstr = Tcl_GetStringFromObj(objv[1], &commentlen);
	for (i=1; i<commentlen; i+=2) {
		if (commentstr[i] == '-') {
			if (
					commentstr[i-1] == '-' ||
					(i < commentlen-1 && commentstr[i+1] == '-')
			   ) {
				rc = TCL_ERROR;
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Comments cannot contain the sequence --"));
			}
		}
	}

	if (parent == NULL) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No parent node context"));
		goto done;
	}

	TEST_OK_LABEL(done, rc, Hyprdom_GetNameFromObj(interp, parent->doc, (Tcl_Obj*)cdata, &name_id));

	node = new_node(parent->doc, &myslot);
	node->type = COMMENT;
	TEST_OK_LABEL(done, rc, set_node_name(interp, node, TEXT_NODE_NAME));
	replace_tclobj(&node->value, objv[1]);
	TEST_OK_LABEL(done, rc, attach_node(interp, &myslot, parent, NULL));

	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&myslot));

done:
	return rc;
}

//>>>
static int autonode_pi_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	parent = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	struct node*		node = NULL;
	uint32_t			name_id;
	struct node_slot	myslot;

	CHECK_ARGS(2, "target data");

	if (parent == NULL) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No parent node context"));
		goto done;
	}

	TEST_OK_LABEL(done, rc, Hyprdom_GetNameFromObj(interp, parent->doc, (Tcl_Obj*)cdata, &name_id));

	node = new_node(parent->doc, &myslot);
	node->type = PI;
	TEST_OK_LABEL(done, rc, set_node_name(interp, node, PI_NODE_NAME));
	replace_tclobj(&node->value, Tcl_NewListObj(objc-1, objv+1));
	TEST_OK_LABEL(done, rc, attach_node(interp, &myslot, parent, NULL));

	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(&myslot));

done:
	return rc;
}

//>>>
static int autonode_parse_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	parent = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);

	if (parent == NULL) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No parent node context"));
		goto done;
	}

	CHECK_ARGS(1, "xml");

	// TODO: implement
	rc = TCL_ERROR;
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Not implemented yet"));
	goto done;

done:
	return rc;
}

//>>>
static int autonode_unknown_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	// Auto node unknown handler: <foo bar="baz" {...script...} adds an
	// element node <foo> with attribs and evaluates script.  Any nodes added
	// by script are added as children of <foo>.  Unknown matches <* and adds
	// a command to handle the case directly.
	int				rc = TCL_OK;
	const char*		cmd = NULL;
	Tcl_Namespace*	ns = Tcl_GetCurrentNamespace(interp);
	Tcl_Obj*		fqcmd = NULL;
	const char*		ns_str = ns->fullName;
	int				cmd_len;
	int				handled = 1;

	CHECK_ARGS(1, "cmd");

	cmd = Tcl_GetStringFromObj(objv[0], &cmd_len);
	if (cmd[0] == '<') {
		Tcl_Obj*	nodename = NULL;
		replace_tclobj(&nodename, Tcl_NewStringObj(cmd+1, cmd_len-1));
		replace_tclobj(&fqcmd, Tcl_ObjPrintf("%s::%s", ns_str, cmd));
		Tcl_CreateObjCommand(interp, Tcl_GetString(fqcmd), autonode_tag_cmd, nodename, free_autonode_nodename);
		nodename = NULL;	// Hand off our reference to the command cdata
	} else if (cmd[0] == '&') {
		Tcl_Obj*	entity = NULL;
		replace_tclobj(&entity, Tcl_ObjPrintf("%s;", cmd));
		replace_tclobj(&fqcmd, Tcl_ObjPrintf("%s::%s", ns_str, cmd));
		Tcl_CreateObjCommand(interp, Tcl_GetString(fqcmd), autonode_entity_cmd, entity, free_autonode_nodename);
		entity = NULL;		// Hand off our reference to the command cdata
	} else if (strcmp("txt", cmd)) {
		Tcl_Obj*	nodename = NULL;
		replace_tclobj(&nodename, Tcl_NewStringObj(TEXT_NODE_NAME, -1));
		replace_tclobj(&fqcmd, Tcl_ObjPrintf("%s::%s", ns_str, cmd));
		Tcl_CreateObjCommand(interp, Tcl_GetString(fqcmd), autonode_txt_cmd, nodename, free_autonode_nodename);
		nodename = NULL;	// Hand off our reference to the command cdata
	} else if (strcmp("<!--", cmd)) {
		Tcl_Obj*	nodename = NULL;
		replace_tclobj(&nodename, Tcl_NewStringObj(COMMENT_NODE_NAME, -1));
		replace_tclobj(&fqcmd, Tcl_ObjPrintf("%s::%s", ns_str, cmd));
		Tcl_CreateObjCommand(interp, Tcl_GetString(fqcmd), autonode_comment_cmd, nodename, free_autonode_nodename);
		nodename = NULL;	// Hand off our reference to the command cdata
	} else if (strcmp("<?", cmd)) {
		Tcl_Obj*	nodename = NULL;
		replace_tclobj(&nodename, Tcl_NewStringObj(PI_NODE_NAME, -1));
		replace_tclobj(&fqcmd, Tcl_ObjPrintf("%s::%s", ns_str, cmd));
		Tcl_CreateObjCommand(interp, Tcl_GetString(fqcmd), autonode_pi_cmd, nodename, free_autonode_nodename);
		nodename = NULL;	// Hand off our reference to the command cdata
	} else if (strcmp("<", cmd)) {
		replace_tclobj(&fqcmd, Tcl_ObjPrintf("%s::%s", ns_str, cmd));
		Tcl_CreateObjCommand(interp, Tcl_GetString(fqcmd), autonode_parse_cmd, NULL, NULL);
	} else {
		handled = 0;
	}

	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(handled));

	replace_tclobj(&fqcmd, NULL);
	return rc;
}

//>>>
static int add_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	node_slot = NULL;
	Tcl_Obj*			cxnode = NULL;
	Tcl_Obj*			script = NULL;
	struct node_slot*	hold_slot = NULL;

	CHECK_ARGS(2, "node script");

	replace_tclobj(&cxnode, objv[1]);	// Take an additional ref to this so that the intrep we borrow doesn't go away before we're done with it
	TEST_OK_LABEL(done, rc, Hyprdom_GetNodeSlotFromObj(interp, objv[1], &node_slot));
	replace_tclobj(&script, objv[2]);

	hold_slot = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);
	Tcl_SetAssocData(interp, "hyprdom_current_node", NULL, node_slot);

	// TODO: implement as NR command
	TEST_OK_LABEL(done, rc, Tcl_EvalObjEx(interp, objv[objc-1], 0));

done:
	if (hold_slot) {
		Tcl_SetAssocData(interp, "hyprdom_current_node", NULL, hold_slot);
	} else {
		Tcl_DeleteAssocData(interp, "hyprdom_current_node");
	}
	replace_tclobj(&cxnode, NULL);
	replace_tclobj(&script, NULL);
	return rc;
}

//>>>
static int current_node_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	current_node = Tcl_GetAssocData(interp, "hyprdom_current_node", NULL);

	CHECK_ARGS(0, "");

	if (current_node == NULL) {
		rc = TCL_ERROR;
		Tcl_SetErrorCode(interp, "HYPRDOM", "NO_CURRENT_NODE", NULL);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("No current node context"));
		goto done;
	}

	Tcl_SetObjResult(interp, Hyprdom_NewSlotRef(current_node));

done:
	return rc;
}

//>>>
static int xpath_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	struct node_slot*		node_slot = NULL;
	xpath1_context_t*		parser = NULL;
	int						remain;
	int						buflen;
	struct xpath1_parse_cx	xpath_cx = {
		.interp	= interp,
		.buf	= NULL,
		.ofs	= 0,
		.len	= 0,
#if DEDUP
		.p		= new_dedup_pool(interp),
#endif
		.rc		= TCL_OK
	};
	Tcl_Obj*				ops = NULL;

	CHECK_ARGS(2, "node xpath");

	TEST_OK_LABEL(done, xpath_cx.rc, Hyprdom_GetNodeSlotFromObj(interp, objv[1], &node_slot));

	xpath_cx.buf = Tcl_GetStringFromObj(objv[2], &buflen);

	xpath_cx.len = buflen;
	parser = xpath1_create(&xpath_cx);
	//remain = xpath1_parse(parser, &ops);
	{
		int ans;
		remain = xpath1_parse(parser, &ans);
		replace_tclobj(&ops, Tcl_NewIntObj(ans));
	}
	if (remain && xpath_cx.rc == TCL_OK) {
		xpath_cx.rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Garbage after xpath"));
		goto done;
	}
	xpath1_destroy(parser);

	if (xpath_cx.rc == TCL_OK)
		Tcl_SetObjResult(interp, ops);

done:
	replace_tclobj(&ops, NULL);

	if (xpath_cx.p) {
		free_dedup_pool(xpath_cx.p);
		xpath_cx.p = NULL;
	}

	return xpath_cx.rc;
}

//>>>
static int stats_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj *const objv[]) //<<<
{
	int					rc = TCL_OK;
	struct node_slot*	node_slot = NULL;
	struct doc*			doc = NULL;
	Tcl_DString			out;
	Tcl_Obj*			radishobj = NULL;
	char				numbuf[MAX_CHAR_LEN_DECIMAL_INTEGER(intptr_t)+2];
	int					names_len;

	CHECK_ARGS(1, "node");

	TEST_OK_LABEL(done, rc, Hyprdom_GetNodeSlotFromObj(interp, objv[1], &node_slot));

	doc = node_slot->doc;

	Tcl_DStringInit(&out);

	radishobj = Radish_NewRadishObj(&doc->radish_names);

	Tcl_DStringAppend(&out, Tcl_GetString(radishobj), -1);
	Tcl_DStringAppend(&out, "\n\nRadish Names: -----------------------------\n", -1);
	radish_to_dot(&out, &doc->radish_names);

	Tcl_DStringAppend(&out, "\n\nNames ", -1);
	TEST_OK_LABEL(done, rc, Tcl_ListObjLength(interp, doc->names, &names_len));
	sprintf(numbuf, "%d", names_len);
	Tcl_DStringAppend(&out, numbuf, -1);
	Tcl_DStringAppend(&out, ": -----------------------------\n", -1);
	Tcl_DStringAppend(&out, Tcl_GetString(doc->names), -1);

	Tcl_DStringAppend(&out, "\n\nDedup: -----------------------------\n", -1);
	dedup_stats(&out, doc->dedup_pool);

	Tcl_DStringResult(interp, &out);

done:
	return rc;
}

//>>>

#ifdef __cplusplus
extern "C" {
#endif
DLLEXPORT int Hyprdom_Init(Tcl_Interp* interp) //<<<
{
	int				rc = TCL_OK;
	Tcl_Namespace*	ns = NULL;

#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
		return TCL_ERROR;
#endif // USE_TCL_STUBS

	g_pagesize = sysconf(_SC_PAGESIZE);

	// Check that our custom node_slot intrep fits within Tcl_ObjInternalRep
	if (sizeof(struct node_slot) > sizeof(Tcl_ObjInternalRep)) {
		rc = TCL_ERROR;
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("On this platform, our node_slot struct doesn't fit into a Tcl_ObjInternalRep, sorry"));
		goto done;
	}

#define NS "::hyprdom"

	ns = Tcl_CreateNamespace(interp, NS, NULL, NULL);
	TEST_OK_LABEL(done, rc, Tcl_Export(interp, ns, "*", 0));
	Tcl_CreateEnsemble(interp, NS, ns, 0);

	Tcl_CreateObjCommand(interp, NS "::new",				new_doc_cmd,			NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::parse",				parse_cmd,				NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::free",				free_doc_cmd,			NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::asXML",				asXML_cmd,				NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::autonode_unknown",	autonode_unknown_cmd,	NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::add",				add_cmd,				NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::current_node",		current_node_cmd,		NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::xpath",				xpath_cmd,				NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::stats",				stats_cmd,				NULL, NULL);

	TEST_OK_LABEL(done, rc, Radish_Init(interp));

	TEST_OK_LABEL(done, rc, Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION));

done:
	return rc;
}

//>>>
DLLEXPORT int Hyprdom_SafeInit(Tcl_Interp* interp) //<<<
{
	// No unsafe features
	return Hyprdom_Init(interp);
}

//>>>
#ifdef __cplusplus
}
#endif

/* Local Variables: */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* End: */
// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4 ft=c

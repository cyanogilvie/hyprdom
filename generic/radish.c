#include "hyprdomInt.h"

#define RADISH_HOTEDGE_TUNE	0
#define RADISH_UTF8_SLOTS	0

#define RADISH_FOREACH_SLOT(radish, block) \
	do { \
		uint32_t			i; \
		for (i=0; i<r->next_free_slot; i++) { \
			Tcl_DString*			node = &r->nodes[i]; \
			const char*				radish_node = Tcl_DStringValue(node); \
			enum radish_node_type	node_type; \
			size_t					prefix_len; \
			intptr_t				value; \
			uint32_t				children; /* by the time block runs, points at the first child */ \
			\
			node_type = *radish_node++; \
			prefix_len = strlen(radish_node); \
			children = prefix_len+1; \
			\
			value = *(intptr_t*)(radish_node+children); \
			children += sizeof(ClientData); \
			\
			block \
		} \
	} while(0);

#if RADISH_HOTEDGE_TUNE
#define RADISH_FOREACH_CHILD(block) \
	if (Tcl_DStringLength(node) > 0)  { \
		switch (node_type) { \
			case RADISH_PACKED: \
				{ \
					uint32_t	rp = children; \
					while (radish_node[rp]) { \
						const char			child_char = radish_node[rp++]; \
						const unsigned char child_heat = radish_node[rp++]; \
						uint32_t			child_slot; \
						rp += read_radish_slot(radish_node+rp, &child_slot); \
						\
						block \
					} \
					break; \
				} \
			case RADISH_LUT8: \
				{ \
					while (radish_node[children]) { \
						const unsigned char	min = radish_node[children++]; \
						const unsigned char	max = radish_node[children++]; \
						unsigned char i; \
						for (i=min; i<=max; i++) { \
							const char			child_char = i; \
							uint32_t			child_slot = (unsigned char)radish_node[children+i-min]; \
							\
							block \
						} \
					} \
					break; \
				} \
			default: \
				Tcl_Panic("Corrupt radish, node type is not valid: \"%d\"", node_type); \
		} \
	};
#else
#define RADISH_FOREACH_CHILD(block) \
	if (Tcl_DStringLength(node) > 0)  { \
		switch (node_type) { \
			case RADISH_PACKED: \
				{ \
					uint32_t	rp = children; \
					while (radish_node[rp]) { \
						const char			child_char = radish_node[rp++]; \
						uint32_t			child_slot; \
						rp += read_radish_slot(radish_node+rp, &child_slot); \
						\
						block \
					} \
					break; \
				} \
			case RADISH_LUT8: \
				{ \
					while (radish_node[children]) { \
						const unsigned char	min = radish_node[children++]; \
						const unsigned char	max = radish_node[children++]; \
						unsigned char i; \
						for (i=min; i<=max; i++) { \
							const char			child_char = i; \
							uint32_t			child_slot = (unsigned char)radish_node[children+i-min]; \
							\
							block \
						} \
					} \
					break; \
				} \
			default: \
				Tcl_Panic("Corrupt radish, node type is not valid: \"%d\"", node_type); \
		} \
	};
#endif

const char* radish_node_type_str[] = {
	"RADISH_EMPTY",
	"RADISH_PACKED",
	"RADISH_LUT8"
};

static inline int read_radish_slot(const char* restrict bytes, uint32_t* slot) { // Sneaky abuse of UTF-8 encoding to store radish slot references on the tail of the slots (after the prefixes) <<<
#if RADISH_UTF8_SLOTS
	uint32_t	cp = 0;
	int			p = 0;

	//if (unlikely(bytes[p] == 0))
	//	return 0;

	if (likely((bytes[p] & 0x80) == 0)) {
		*slot = bytes[p++];
		return p;
	}

	if ((bytes[p] & 0xE0) == 0xC0) { // Two byte encoding
		cp  = (bytes[p++] & 0x1F) << 16;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |=  bytes[p++] & 0x3F;
	} else if ((bytes[p] & 0xF0) == 0xE0) { // Three byte encoding
		cp  = (bytes[p++] & 0x0F) << 12;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 6;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |=  bytes[p++] & 0x3F;
	} else if ((bytes[p] & 0xF8) == 0xF0) { // Four byte encoding
		cp  = (bytes[p++] & 0x07) << 18;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 12;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 6;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |=  bytes[p++] & 0x3F;
	} else if ((bytes[p] & 0xFC) == 0xF8) { // Five byte encoding
		cp  = (bytes[p++] & 0x03) << 24;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 18;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 12;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 6;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |=  bytes[p++] & 0x3F;
	} else if ((bytes[p] & 0xFE) == 0xFC) { // Six byte encoding
		cp  = (bytes[p++] & 0x01) << 30;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 24;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 18;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 12;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |= (bytes[p++] & 0x3F) << 6;	// if (bytes[p] & 0xC0 != 0x80) goto err;
		cp |=  bytes[p++] & 0x3F;
	} else {
		return 0;
	}

	*slot = cp;

	return p;
#else
	*slot = *(uint32_t*)bytes;
	return sizeof(uint32_t);
#endif
}

//>>>
static inline int write_radish_slot(char* bytes, uint32_t slot) { // Sneaky abuse of UTF-8 encoding to store radish slot references on the tail of the slots (after the prefixes) <<<
#if RADISH_UTF8_SLOTS
	unsigned char* restrict	o = (unsigned char*)bytes;
	const uint32_t			codepoint = slot;

	if (codepoint <= 0x7f) { // Identity encoding
		*o++ = codepoint;
	} else if (codepoint <= 0x7FF) { // Two byte encoding
		*o++ = 0b11000000 | ((codepoint >>  6) & 0b00011111);
		*o++ = 0b10000000 | ( codepoint        & 0b00111111);
	} else if (codepoint <= 0xFFFF) { // Three byte encoding
		*o++ = 0b11100000 | ((codepoint >> 12) & 0b00001111);
		*o++ = 0b10000000 | ((codepoint >>  6) & 0b00111111);
		*o++ = 0b10000000 | (codepoint & 0b00111111);
	} else if (codepoint <= 0x1FFFFF) { // Four byte encoding
		*o++ = 0b11110000 | ((codepoint >> 18) & 0b00000111);
		*o++ = 0b10000000 | ((codepoint >> 12) & 0b00111111);
		*o++ = 0b10000000 | ((codepoint >>  6) & 0b00111111);
		*o++ = 0b10000000 | ( codepoint        & 0b00111111);
	} else if (codepoint <= 0x3FFFFFF) { // Five byte encoding
		*o++ = 0b11111000 | ((codepoint >> 24) & 0b00000011);
		*o++ = 0b10000000 | ((codepoint >> 18) & 0b00111111);
		*o++ = 0b10000000 | ((codepoint >> 12) & 0b00111111);
		*o++ = 0b10000000 | ((codepoint >>  6) & 0b00111111);
		*o++ = 0b10000000 | ( codepoint        & 0b00111111);
	} else if (codepoint <= 0x7FFFFFFF) { // Six byte encoding
		*o++ = 0b11111100 | ((codepoint >> 30) & 0b00000001);
		*o++ = 0b10000000 | ((codepoint >> 24) & 0b00111111);
		*o++ = 0b10000000 | ((codepoint >> 18) & 0b00111111);
		*o++ = 0b10000000 | ((codepoint >> 12) & 0b00111111);
		*o++ = 0b10000000 | ((codepoint >>  6) & 0b00111111);
		*o++ = 0b10000000 | ( codepoint        & 0b00111111);
	} else {
		// Invalid codepoint
		return 0;
	}

	return o-(unsigned char*)bytes;
#else
	*(uint32_t*)bytes = slot;
	return sizeof(uint32_t);
#endif
}

//>>>

void free_internal_rep_radish(Tcl_Obj*);
void dup_internal_rep_radish(Tcl_Obj* src, Tcl_Obj* dup);
void update_string_rep_radish(Tcl_Obj* obj);

Tcl_ObjType radish_objtype = {
	"RadishTrie",
	free_internal_rep_radish,
	dup_internal_rep_radish,
	update_string_rep_radish,
	NULL
};

void free_internal_rep_radish(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &radish_objtype);
	struct radish*		r = ir->ptrAndLongRep.ptr;

	if (r) {
		Radish_DecrRef(r);
		r = NULL;
	}
}

//>>>
void dup_internal_rep_radish(Tcl_Obj* src, Tcl_Obj* dup) //<<<
{
	Tcl_ObjIntRep*	ir = Tcl_FetchIntRep(src, &radish_objtype);
	Tcl_ObjIntRep	newir;
	size_t			nodes_size;

	struct radish*	src_r = ir->ptrAndLongRep.ptr;
	struct radish*	dst_r = ckalloc(sizeof(*dst_r));

	memcpy(dst_r, src_r, sizeof(*dst_r));
	nodes_size = dst_r->slots * sizeof(Tcl_DString);
	dst_r->nodes = ckalloc(nodes_size);
	dst_r->refcount = 0;
	memcpy(dst_r->nodes, src_r->nodes, nodes_size);

	Radish_IncrRef(newir.ptrAndLongRep.ptr = dst_r);
	newir.ptrAndLongRep.value = 0;

	Tcl_StoreIntRep(dup, &radish_objtype, &newir);
}

//>>>
static inline void update_string_rep_child_body(Tcl_DString* ds, const char child_char, uint32_t child_slot) //<<<
{
	char	numbuf[MAX_CHAR_LEN_DECIMAL_INTEGER(uint32_t)+1];
	char	charbuf[2];

	charbuf[0] = child_char;
	charbuf[1] = 0;

	Tcl_DStringAppendElement(ds, charbuf);
	sprintf(numbuf, "%d", child_slot);
	Tcl_DStringAppendElement(ds, numbuf);
}

//>>>
static inline void update_string_rep_slot_body(Tcl_DString* ds, Tcl_DString* node, enum radish_node_type node_type, const char* radish_node, intptr_t value, uint32_t children) //<<<
{
	char		valuestr[MAX_CHAR_LEN_DECIMAL_INTEGER(intptr_t)+2]; /* +1: null term, +1 sign */ \

	Tcl_DStringStartSublist(ds);
	if (node_type != RADISH_EMPTY) {
		Tcl_DStringAppendElement(ds, radish_node);	// prefix

		sprintf(valuestr, "%ld", value);
		Tcl_DStringAppendElement(ds, valuestr);	// value

		// children
		if (radish_node[children]) {
			Tcl_DStringStartSublist(ds);
			RADISH_FOREACH_CHILD({ // Sets child_char, child_slot
				update_string_rep_child_body(ds, child_char, child_slot);
			});
			Tcl_DStringEndSublist(ds);
		}
	}
	Tcl_DStringEndSublist(ds);
}

//>>>
void update_string_rep_radish(Tcl_Obj* obj) //<<<
{
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &radish_objtype);
	struct radish*		r = ir->ptrAndLongRep.ptr;
	Tcl_DString			ds;

	Tcl_DStringInit(&ds);

	RADISH_FOREACH_SLOT(r, { // Sets node, radish_node, prefix_len, value and children
		update_string_rep_slot_body(&ds, node, node_type, radish_node, value, children);
	});	

	obj->length = Tcl_DStringLength(&ds);
	obj->bytes = ckalloc(obj->length+1);
	memcpy(obj->bytes, Tcl_DStringValue(&ds), obj->length);
	obj->bytes[obj->length] = 0;

	Tcl_DStringFree(&ds);
}

//>>>
int Radish_GetRadishFromObj(Tcl_Interp* interp, Tcl_Obj* obj, struct radish** radish) //<<<
{
	int					rc = TCL_OK;
	Tcl_ObjIntRep*		ir = Tcl_FetchIntRep(obj, &radish_objtype);
	struct radish*		r = NULL;

	if (ir == NULL) {
		Tcl_Obj**		slotv;
		int				slotc;
		int				i;
		Tcl_ObjIntRep	newir;

		TEST_OK_LABEL(done, rc, Tcl_ListObjGetElements(interp, obj, &slotc, &slotv));

		if (slotc < 1) {
			rc = TCL_ERROR;
			if (interp)
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Radishes need at least 1 slot"));
			goto done;
		}

		r = ckalloc(sizeof(*r));
		radish_init(r, (int)slotc*1.5);
		Radish_IncrRef(r);

		for (i=0; i<slotc; i++) {
			Tcl_Obj**	nodev;
			int			nodec;
			const char*	str;
			int			str_len;
			long		value;
			Tcl_Obj**	childv;
			int			childc;
			int			child;
			uint32_t	max_slot_ref = 0;

			TEST_OK_LABEL(done, rc, Tcl_ListObjGetElements(interp, slotv[i], &nodec, &nodev));

			Tcl_DStringInit(&r->nodes[i]);

			if (nodec != 2 && nodec != 3)
				THROW_ERROR_LABEL(done, rc, interp, "Radish slots must be empty or lists of 2/3 items: prefix, value, ?children?");

			str = Tcl_GetStringFromObj(nodev[0], &str_len);
			TEST_OK_LABEL(done, rc, Tcl_GetLongFromObj(interp, nodev[1], &value));

			if (childc % 2 != 0)
				THROW_ERROR_LABEL(done, rc, interp, "Radish children must be an dict-style list of branch_byte -> slot");

			str = Tcl_GetStringFromObj(nodev[0], &str_len);
			Tcl_DStringAppend(&r->nodes[i], str, str_len);
			Tcl_DStringAppend(&r->nodes[i], "\0", 1);
			Tcl_DStringAppend(&r->nodes[i], (char*)INT2PTR(value), sizeof(ClientData));

			if (nodec > 2) {
				TEST_OK_LABEL(done, rc, Tcl_ListObjGetElements(interp, nodev[2], &childc, &childv));

				for (child=0; child<childc; child+=2) {
					int			branch_char_len;
					const char*	branch_char	 = Tcl_GetStringFromObj(childv[child], &branch_char_len);
					uint32_t	slot;
					char		buf[6];
					int			buf_len;

					if (branch_char_len > 1)
						THROW_ERROR_LABEL(done, rc, interp, "Radish children branch_byte longer than a single byte: %d: %s", branch_char_len, branch_char);

					TEST_OK_LABEL(done, rc, Tcl_GetIntFromObj(interp, childv[i+1], (int*)&slot));

					buf_len = write_radish_slot(buf, slot);

					if (slot > max_slot_ref)
						max_slot_ref = slot;

					if (buf_len == 0)
						THROW_ERROR_LABEL(done, rc, interp, "Radish child %c slot reference %d could not be encoded", branch_char[0], slot);

					Tcl_DStringAppend(&r->nodes[i], buf, buf_len);
				}
			}
			Tcl_DStringAppend(&r->nodes[i], "\0", 1);

			r->next_free_slot = max_slot_ref+1;

			newir.ptrAndLongRep.ptr = r;
			newir.ptrAndLongRep.value = 0;

			Tcl_FreeIntRep(obj);
			Tcl_StoreIntRep(obj, &radish_objtype, &newir);
			ir = Tcl_FetchIntRep(obj, &radish_objtype);
		}
	}

	*radish = (struct radish*)ir->ptrAndLongRep.ptr;

done:
	if (r) {
		Radish_DecrRef(r);
		r = NULL;
	}
	return rc;
}

//>>>
void Radish_IncrRef(struct radish* radish) //<<<
{
	if (radish->refcount < 0)
		Tcl_Panic("Trying to increment a negative refcount: %d", radish->refcount);

	radish->refcount++;
}

//>>>
void Radish_DecrRef(struct radish* radish) //<<<
{
	if (--radish->refcount <= 0) {
		radish_free(radish);
		//ckfree(radish);
	}
}

//>>>
Tcl_Obj* Radish_NewRadishObj(struct radish* radish) //<<<
{
	Tcl_Obj*		res = Tcl_NewObj();
	Tcl_ObjIntRep	ir;

	Radish_IncrRef(ir.ptrAndLongRep.ptr = radish);

	Tcl_FreeIntRep(res);
	Tcl_StoreIntRep(res, &radish_objtype, &ir);
	Tcl_InvalidateStringRep(res);

	return res;
}

//>>>

void radish_init(struct radish* radish, uint32_t initial_slots) //<<<
{
	static int	pagesize = 0;

	if (initial_slots == 0) {
		if (pagesize == 0) pagesize = sysconf(_SC_PAGESIZE);

		initial_slots = pagesize / sizeof(Tcl_DString);
	}

	if (initial_slots < 2)
		// Avoid having to cater for degenerate cases
		initial_slots = 2;

	radish->nodes = ckalloc(initial_slots * sizeof(Tcl_DString));
	DBG("Allocated radish->nodes: %s\n", name("radishnodes:", radish->nodes));
	radish->slots = initial_slots;
	Tcl_DStringInit(radish->nodes);
	radish->next_free_slot = 1;
	radish->refcount = 0;
	radish->valtype = RADISH_VALTYPE_INT;
}

//>>>
void radish_free(struct radish* radish) //<<<
{
	uint32_t	i;

	for (i=0; i<radish->next_free_slot; i++)
		Tcl_DStringFree(&radish->nodes[i]);

	free_thing(radish->nodes);
	ckfree(radish->nodes);
	radish->nodes = NULL;
	radish->slots = 0;
	radish->next_free_slot = 0;
	radish->refcount = 0;
}

//>>>
void dump_radish_node(const char* prefix, Tcl_DString* node) //<<<
{
	const char*				radish_node = Tcl_DStringValue(node);
	enum radish_node_type	node_type;
	size_t					prefix_len;
	intptr_t				value;

	node_type = *radish_node++;
	prefix_len = strlen(radish_node);
	value = *(intptr_t*)(radish_node+prefix_len+1);

	fprintf(stderr, "%s: type: %s, prefix: \"%s\" (%ld bytes) -> %ld\n", prefix, radish_node_type_str[node_type], radish_node, prefix_len, value);
	{
		const char* restrict	p = radish_node+prefix_len+1+sizeof(ClientData);
		while (*p) {
			const char	byte = *p++;
#if RADISH_HOTEDGE_TUNE
			const unsigned char	heat = *p++;
#endif
			uint32_t	slot;

			p += read_radish_slot(p, &slot);
#if RADISH_HOTEDGE_TUNE
			fprintf(stderr, "\t'%c' 0x%02x (h: %d) -> %d\n", byte, byte, heat, slot);
#else
			fprintf(stderr, "\t'%c' 0x%02x -> %d\n", byte, byte, slot);
#endif
		}
	}
}

//>>>
void set_radish(struct radish_search* search, const char* tail, uint32_t tail_len, ClientData value) //<<<
{
	struct radish*			radish = search->radish;
	uint32_t				rp = search->divergence;
	Tcl_DString*			gp =  &radish->nodes[search->slot];
	char* restrict			radish_node = Tcl_DStringValue(gp);
	enum radish_node_type	node_type = *radish_node;

	/*
	 * If gp is an empty node, then set it to a leaf node with tail -> value
	 * (happens when the radish tree is empty and the first name is
	 * added to it).
	 */
	if (unlikely(node_type == RADISH_EMPTY)) {
		const char node_type_byte = RADISH_PACKED;
		Tcl_DStringAppend(gp, &node_type_byte, 1);
		Tcl_DStringAppend(gp, tail, tail_len);
		Tcl_DStringAppend(gp, "\0", 1);
		Tcl_DStringAppend(gp, (char *)&value, sizeof(ClientData));
		D(dump_radish_node("After set of RADISH_EMPTY", gp));
		return;
	}

	if (unlikely(radish->next_free_slot >= radish->slots-1)) { // -1: need to make (up to) two new nodes
		/*
		 * Out of space for new nodes, grow the allocation
		 */
		uint32_t	inc = radish->slots * 2;
		size_t		newmem;

		// Grow by doubling until the increment reaches 1 MiB, then
		// grow by 1MiB chunks at a time (roughly 4800 slots worth)
		if (inc * sizeof(Tcl_DString) > 1048576) {
			inc = 1048576 / sizeof(Tcl_DString);
		}

		newmem = (radish->slots + inc) * sizeof(Tcl_DString);
		radish->nodes = ckrealloc(radish->nodes, newmem);
		radish->slots += inc;
	}

	if (rp < Tcl_DStringLength(gp)) {
		/*
		 * Split the current radish node at rp,
		 * and add a new branch for xml[unique_name_suffix] -> new radish slot above
		 */
		const uint32_t	new_slot = radish->next_free_slot++;
		Tcl_DString*	new_node = &radish->nodes[new_slot];
		char			buf[6];
		const int		buf_len = write_radish_slot(buf, new_slot);
		const int		len = Tcl_DStringLength(gp);
		int				i;
		uint32_t		old_child_start;
		const char		old_branch_char = radish_node[rp];
		intptr_t		nullval = -1;

		// Find the null after the prefix
		for (i=0; i<len; i++)
			if (radish_node[i] == 0)
				break;

		i++;						// points at the value
		old_child_start = i + sizeof(ClientData);	// points at the first child link (if any)

		if (unlikely(old_child_start > len))
			Tcl_Panic("Corrupt radish node");

		// add new node as a child
		Tcl_DStringInit(new_node);
		Tcl_DStringAppend(new_node, radish_node, 1);			// Copy the node_type to the new branch
		Tcl_DStringAppend(new_node, radish_node+rp+1, len-rp-1);
		Tcl_DStringAppend(new_node, "\0", 1);
		Tcl_DStringAppend(new_node, radish_node+i, sizeof(ClientData));	// Copy the value to the new branch
		if (len > old_child_start)
			Tcl_DStringAppend(new_node, radish_node+old_child_start, len-old_child_start);

		// Truncate the prefix at the divergence point, clear the value for
		// this branch node, move the children up and truncate this dstring
		radish_node[0] = node_type = RADISH_PACKED;
		Tcl_DStringSetLength(gp, rp);
		Tcl_DStringAppend(gp, "\0", 1);
		Tcl_DStringAppend(gp, (char*)&nullval, sizeof(ClientData));	// Copy the value to the new branch

		Tcl_DStringAppend(gp, &old_branch_char, 1);	// Link the divergent char
#if RADISH_HOTEDGE_TUNE
		Tcl_DStringAppend(gp, "\0", 1);				// Initialize heat to 0.  TODO: inherit initial heat from parent branch?
#endif
		Tcl_DStringAppend(gp, buf, buf_len);		// Write our new slot

		// gp is now remodelled as a branch node at the divergence point, and
		// ready to receive the new child the tail prefix
	}

	if (tail_len == 0) {
		// GP (or the newly split version) is the target node for the value: no tail 
		char*	radish_node = Tcl_DStringValue(gp);
		int		rp = strlen(radish_node);

		if (rp == Tcl_DStringLength(gp)) {
			Tcl_DStringAppend(gp, "\0", 1);
			Tcl_DStringAppend(gp, (char*)&value, sizeof(ClientData));
		} else {
			rp++;
			*(intptr_t*)(radish_node+rp) = (intptr_t)value;
		}

	} else {
		const uint32_t	new_slot = radish->next_free_slot++;
		Tcl_DString*	new_node = &radish->nodes[new_slot];
		char			buf[6];
		const int		buf_len = write_radish_slot(buf, new_slot);
		const char		node_type_byte = RADISH_PACKED;

		// add new node as a child
		Tcl_DStringInit(new_node);
		Tcl_DStringAppend(new_node, &node_type_byte, 1);
		Tcl_DStringAppend(new_node, tail+1, tail_len-1);
		Tcl_DStringAppend(new_node, "\0", 1);
		Tcl_DStringAppend(new_node, (char *)&value, sizeof(ClientData));

		switch (node_type) {
			case RADISH_PACKED:
				// TODO: if the number of children exceeds some threshold (probably 7 or so), convert gp to a RADISH_LUT8 (if radish->slots < 256)
				Tcl_DStringAppend(gp, tail, 1);		// Link the divergent char
#if RADISH_HOTEDGE_TUNE
				Tcl_DStringAppend(gp, "\0", 1);		// Initialize heat to 0
#endif
				Tcl_DStringAppend(gp, buf, buf_len);	// Write our new slot in UTF-8
				break;

			case RADISH_LUT8:
				// TODO: Implement
				break;

			default:
				Tcl_Panic("Corrupt radish, node type is not valid: \"%d\"", node_type);
		}
	}
	D(dump_radish_node("After set", gp));
}

//>>>
void radish_search_init(Tcl_Interp* interp, struct radish* radish, jmp_buf* on_error, struct radish_search* search) //<<<
{
	__builtin_prefetch(radish->nodes, 0, 3);
	search->radish		= radish;
	search->slot		= 0;
	search->divergence	= 0;
	search->value_ofs	= 0;
	search->state		= BUSY;
	search->on_error	= on_error;
	search->interp		= interp;
	search->rc			= TCL_OK;
}

//>>>
uint32_t radish_search(struct radish_search* search, const char* bytes) //<<<
{
	Tcl_DString*			node = &search->radish->nodes[0];
	char*					radish_node = Tcl_DStringValue(node);
	const char* restrict	p = bytes;
	char*					rp = radish_node;
	uint32_t				slot = 0;
	uint32_t				value_ofs;
	enum radish_node_type	node_type;

walk_node:
	node_type = *rp++;
	if (node_type == RADISH_EMPTY) {
		// We're pointing at an empty node, nothing to compare against.  set_radish will
		// write our entire string as the prefix for this node.  (Happens with the first
		// string searched for in an empty radish trie.
		search->state = SET_ROOT;
		return p-bytes;
	}

	// *p implied by the loop conditions
	while (*rp && *p == *rp) {
		p++;
		rp++;
	}

	if (*rp == 0) {
		if (*p == 0) {
			search->state = MATCHED_LEAF;
			search->divergence = rp - radish_node;
			search->value_ofs = search->divergence+1;
			search->slot = slot;
			return p-bytes;
		} else {
			/*
			 * Ran to the end of this radish node prefix, but more name
			 * remains.  Search the child radish nodes for the node
			 * matching the next byte of the name
			 */
			char			next_byte = *p;
#if RADISH_HOTEDGE_TUNE
			char*			first_child = rp+1+sizeof(ClientData);
			char			last_child_byte = 0;
			uint32_t		last_child_slot;
			char*			last_child_rp;
			unsigned char	last_child_heat;
#endif

			rp++;
			value_ofs = rp-radish_node;
			rp += sizeof(ClientData);	// Skip over the prefix null terminator and value

			while (1) {
				const char		child_firstchar = *rp;
#if RADISH_HOTEDGE_TUNE
				char*			save_rp = rp;
				unsigned char	heat;
#endif
				int				adv;

				if (child_firstchar == 0) {
					// No more child nodes for this radish_node.  From xml[p] name is unique
					search->state = DIVERGE_NEW_CHILD;
					// leave rp pointing at the end of radish_node, where we will
					// Tcl_DStringAppend the new child for this node suffix (if
					// it validates).
					search->value_ofs = value_ofs;
					search->divergence = rp-radish_node;
					return p-bytes;
				}

				rp++;
#if RADISH_HOTEDGE_TUNE
				heat = (unsigned char)*rp++;
#endif
				adv = read_radish_slot(rp, &slot);
				/*
				if (unlikely(adv == 0))
					THROW_ERROR_JMP(*search->on_error, search->rc, search->interp, "Corrupted slot encoding for radish node");
				*/
				rp += adv;

				if (child_firstchar == next_byte) {
#if RADISH_HOTEDGE_TUNE
					__builtin_prefetch(&search->radish->nodes[slot], 0, 2);
					if (last_child_byte && heat > last_child_heat + 2) {
						// If this child isn't the first, swap it with its previous sibling (auto-tune hot edges)
						char*		swap_rp = last_child_rp;
						//DBG("Bumping %c (%d) up above %c (%d)\n", next_byte, heat, last_child_byte, last_child_heat);
						//dump_radish_node("Before swap", node);
						*swap_rp++ = next_byte;
						*swap_rp++ = (char)heat;
						swap_rp += write_radish_slot(swap_rp, slot);
						*swap_rp++ = last_child_byte;
						*swap_rp++ = (char)last_child_heat;
						write_radish_slot(swap_rp, last_child_slot);
						//dump_radish_node("After swap", node);
					}
					// Increment the heat
					if (++*(unsigned char*)(save_rp+1) == 0xFF) {
						// Hit heat cap, rescale
#if RADISH_UTF8_SLOTS
#error Not implemented yet
#else
						//DBG("next_byte %c hit the heat cap\n", next_byte);
						//dump_radish_node("Before rescale", node);
						unsigned char* restrict	sp = (unsigned char*)first_child;
						while (*sp++) {
							*sp >>= 3;
							sp += 1+sizeof(uint32_t);
						}
						//dump_radish_node("After rescale", node);
#endif
					}
#endif
					// Found next radish node to walk
					search->slot = slot;
					node = &search->radish->nodes[slot];
					radish_node = Tcl_DStringValue(node);

					p++;
					next_byte = *p;
					rp = radish_node;
					goto walk_node;
				}

				// Loop back around to check the next child node in sequence, looking
				// for the first char that matches bytes[p]
#if RADISH_HOTEDGE_TUNE
				last_child_byte = child_firstchar;
				last_child_slot = slot;
				last_child_rp = save_rp;
				last_child_heat = heat;
#endif
			}
		}
	} else {
		/*
		 * More chars remain on this radish prefix node, leave radish_node[rp] pointing
		 * to the first divergent character in the existing prefix.  If the tail of
		 * the name survives the valid chars checks below and is properly terminated,
		 * we'll split this radish node at rp and allocate a new branch for make from
		 * unique_name_suffix.
		 */
		search->state = DIVERGE_SPLIT;
		search->slot = slot;
		search->divergence = rp-radish_node;
		return p-bytes;
	}
}

//>>>
void radish_to_dot(Tcl_DString* ds, struct radish* r) //<<<
{
	Tcl_DStringAppend(ds, "digraph \"Radish\" {\n", -1);
	RADISH_FOREACH_SLOT(r, { // Sets i, node, radish_node, prefix_len, value and children
		char	slotnum[MAX_CHAR_LEN_DECIMAL_INTEGER(uint32_t)+1];
		int		slotnum_len = sprintf(slotnum, "%d", i);
		(void)value;

		Tcl_DStringAppend(ds, "\t", 1);
		Tcl_DStringAppend(ds, slotnum, slotnum_len);
		Tcl_DStringAppend(ds, " [label=\"", -1);
		Tcl_DStringAppend(ds, radish_node, prefix_len);
		Tcl_DStringAppend(ds, "\"];\n", -1);

		RADISH_FOREACH_CHILD({ // Sets child_char, child_slot
			char	numbuf[MAX_CHAR_LEN_DECIMAL_INTEGER(uint32_t)+1];
			int		numbuf_len = sprintf(numbuf, "%d", child_slot);
			char	hexbuf[3];

			Tcl_DStringAppend(ds, "\t", 1);
			Tcl_DStringAppend(ds, slotnum, slotnum_len);
			Tcl_DStringAppend(ds, " -> ", -1);
			Tcl_DStringAppend(ds, numbuf, numbuf_len);
			Tcl_DStringAppend(ds, " [label=\"", -1);
			Tcl_DStringAppend(ds, &child_char, 1);
			Tcl_DStringAppend(ds, ": 0x", -1);
			sprintf(hexbuf, "%02X", child_char);
			Tcl_DStringAppend(ds, hexbuf, 2);
#if RADISH_HOTEDGE_TUNE
			numbuf_len = sprintf(numbuf, "%d", child_heat);
			Tcl_DStringAppend(ds, ", h:", -1);
			Tcl_DStringAppend(ds, numbuf, numbuf_len);
#endif
			Tcl_DStringAppend(ds, "\"];\n", -1);
		});
	});
	Tcl_DStringAppend(ds, "}\n", -1);
}

//>>>

static int new_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int				rc = TCL_OK;
	int				initial_slots = 0;
	struct radish*	r = NULL;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "?initial_slots?");
		rc = TCL_ERROR;
		goto done;
	}

	if (objc > 1)
		TEST_OK_LABEL(done, rc, Tcl_GetIntFromObj(interp, objv[1], &initial_slots));

	r = ckalloc(sizeof(*r));
	radish_init(r, initial_slots);
	Radish_IncrRef(r);

	Tcl_SetObjResult(interp, Radish_NewRadishObj(r));

done:
	if (r) {
		Radish_DecrRef(r);
		r = NULL;
	}
	return rc;
}

//>>>
static int set_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int						rc = TCL_OK;
	struct radish*			r = NULL;
	struct radish_search	search;
	jmp_buf					on_error;
	const char*				str = NULL;
	int						str_len;
	long					value;
	uint32_t				unique_suffix;
	Tcl_Obj*				src = NULL;

	if ((rc = setjmp(on_error))) goto done;

	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "radishvar string value");
		rc = TCL_ERROR;
		goto done;
	}

	src = Tcl_ObjGetVar2(interp, objv[1], NULL, 0);
	if (src == NULL) {
		Tcl_Obj*		newval = NULL;
		struct radish*	r = ckalloc(sizeof(*r));

		radish_init(r, 0);
		replace_tclobj(&newval, Radish_NewRadishObj(r));
		src = Tcl_ObjSetVar2(interp, objv[1], NULL, newval, TCL_LEAVE_ERR_MSG);
		replace_tclobj(&newval, NULL);
		if (src == NULL) {
			rc = TCL_ERROR;
			goto done;
		}
	}

	if (Tcl_IsShared(src)) {
		Tcl_Obj*	newval = NULL;

		replace_tclobj(&newval, Tcl_DuplicateObj(src));
		src = Tcl_ObjSetVar2(interp, objv[1], NULL, newval, TCL_LEAVE_ERR_MSG);
		replace_tclobj(&newval, NULL);
		if (src == NULL) {
			rc = TCL_ERROR;
			goto done;
		}
	}

	TEST_OK_LABEL(done, rc, Radish_GetRadishFromObj(interp, src, &r));
	str = Tcl_GetStringFromObj(objv[2], &str_len);
	TEST_OK_LABEL(done, rc, Tcl_GetLongFromObj(interp, objv[3], &value));

	radish_search_init(interp, r, &on_error, &search);
	unique_suffix = radish_search(&search, str);
	set_radish(&search, str+unique_suffix, str_len-unique_suffix, INT2PTR(value));

	Tcl_SetObjResult(interp, src);

done:
	return rc;
}

//>>>
static int get_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int						rc = TCL_OK;
	struct radish*			r = NULL;
	struct radish_search	search;
	jmp_buf					on_error;
	const char*				str = NULL;
	int						str_len;
	intptr_t				value;
	uint32_t				unique_suffix;

	if ((rc = setjmp(on_error))) goto done;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "radish string");
		rc = TCL_ERROR;
		goto done;
	}

	TEST_OK_LABEL(done, rc, Radish_GetRadishFromObj(interp, objv[1], &r));
	str = Tcl_GetStringFromObj(objv[2], &str_len);

	radish_search_init(interp, r, &on_error, &search);
	unique_suffix = radish_search(&search, str);

	if (unique_suffix < str_len)
		THROW_ERROR_LABEL(done, rc, interp, "No entry for \"%s\", unique tail: \"%s\"", str, str+unique_suffix);

	value = (intptr_t)radish_get_value(&search);

	Tcl_SetObjResult(interp, Tcl_NewLongObj(value));

done:
	return rc;
}

//>>>
static int dot_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int				rc = TCL_OK;
	struct radish*	r = NULL;
	Tcl_DString		out;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "radish");
		rc = TCL_ERROR;
		goto done;
	}

	TEST_OK_LABEL(done, rc, Radish_GetRadishFromObj(interp, objv[1], &r));

	Tcl_DStringInit(&out);

	radish_to_dot(&out, r);

	Tcl_DStringResult(interp, &out);
	Tcl_DStringFree(&out);

done:
	return rc;
}

//>>>

#ifdef __cplusplus
extern "C" {
#endif
DLLEXPORT int Radish_Init(Tcl_Interp* interp) //<<<
{
	int				rc = TCL_OK;
	Tcl_Namespace*	ns = NULL;

#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
		return TCL_ERROR;
#endif // USE_TCL_STUBS

#define NS "::radish"

	ns = Tcl_CreateNamespace(interp, NS, NULL, NULL);
	TEST_OK_LABEL(done, rc, Tcl_Export(interp, ns, "*", 0));
	Tcl_CreateEnsemble(interp, NS, ns, 0);

	Tcl_CreateObjCommand(interp, NS "::new",			new_cmd,			NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::set",			set_cmd,			NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::get",			get_cmd,			NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::dot",			dot_cmd,			NULL, NULL);

	//TEST_OK_LABEL(done, rc, Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION));

done:
	return rc;
}

//>>>
DLLEXPORT int Radish_SafeInit(Tcl_Interp* interp) //<<<
{
	// No unsafe features
	return Radish_Init(interp);
}

//>>>
#ifdef __cplusplus
}
#endif

//>>>

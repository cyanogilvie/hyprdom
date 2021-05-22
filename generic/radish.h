enum radish_valtype {
	RADISH_VALTYPE_INT,		/* Values are intptr_t */
	RADISH_VALTYPE_TCLOBJ	/* Values are Tcl_Obj* */
};

struct radish {
	Tcl_DString*		nodes;	
	int					refcount;
	uint32_t			slots;			/* The number of slots available */
	uint32_t			next_free_slot;	/* Next free slot, no slots allocated above this */
	enum radish_valtype	valtype;
};

enum radish_state {
	BUSY,
	MATCHED_LEAF,
	DIVERGE_NEW_CHILD,
	DIVERGE_SPLIT,
	INPUT_EOF,
	SET_ROOT
};

struct radish_search {
	struct radish*		radish;
	enum radish_state	state;
	uint32_t			slot;
	uint32_t			divergence;
	uint32_t			value_ofs;		/* offset into this node for the payload value */
	jmp_buf*			on_error;
	Tcl_Interp*			interp;
	int					rc;
};

void radish_init(struct radish* radish, uint32_t initial_slots);
void radish_free(struct radish* radish);
//int write_radish_slot(char* bytes, uint32_t slot);
void set_radish(struct radish_search* search, const char* tail, uint32_t tail_len, ClientData value);
//void add_radish_node(struct radish* radish, uint32_t graft_point, uint32_t rp, const char* restrict tail, uint32_t tail_len, ClientData value);
void radish_search_init(Tcl_Interp* interp, struct radish* radish, jmp_buf* on_error, struct radish_search* search);
uint32_t radish_search(struct radish_search* search, const char* restrict bytes);
void radish_to_dot(Tcl_DString* ds, struct radish* r);

int Radish_GetRadishFromObj(Tcl_Interp* interp, Tcl_Obj* obj, struct radish** radish);
void Radish_IncrRef(struct radish* radish);
void Radish_DecrRef(struct radish* radish);
Tcl_Obj* Radish_NewRadishObj(struct radish* radish);
int Radish_Init(Tcl_Interp* interp);

static inline ClientData radish_get_value(struct radish_search* search) //<<<
{
	Tcl_DString*	node = &search->radish->nodes[search->slot];
	const char*		radish_node = Tcl_DStringValue(node);

	if (search->value_ofs == 0) {
		search->rc = TCL_ERROR;
		if (search->interp)
			Tcl_SetObjResult(search->interp, Tcl_ObjPrintf("Radish search did not resolve to a value"));
		longjmp(*search->on_error, 1);
	}

	return *(ClientData*)(radish_node + search->value_ofs);
}

//>>>

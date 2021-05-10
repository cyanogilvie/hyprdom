#if DEDUP

#include "hyprdomInt.h"

#if FFS == ffsll
#define _GNU_SOURCE		// For glibc extension ffsll
#endif

static int first_free(FREEMAP_TYPE* freemap) //<<<
{
	int	i=0, bit, res;
	FFS_TMP_STORAGE;

	while ((bit = FFS(freemap[i])) == 0) i++;
	res = i * (sizeof(FREEMAP_TYPE)*8) + (bit-1);
	return res;
}

//>>>
static void mark_used(FREEMAP_TYPE* freemap, int idx) //<<<
{
	int	i = idx / (sizeof(FREEMAP_TYPE)*8);
	int bit = idx - (i * (sizeof(FREEMAP_TYPE)*8));
	freemap[i] &= ~(1LL << bit);
}

//>>>
static void mark_free(FREEMAP_TYPE* freemap, int idx) //<<<
{
	int	i = idx / (sizeof(FREEMAP_TYPE)*8);
	int bit = idx - (i * (sizeof(FREEMAP_TYPE)*8));
	freemap[i] |= 1LL << bit;
}

//>>>
void free_hyprdom_dedup_tclempty(ClientData cdata, Tcl_Interp* interp) //<<<
{
	Tcl_DecrRefCount((Tcl_Obj*)cdata);	// AssocData ref
	cdata = NULL;
}

//>>>
struct dedup_pool* new_dedup_pool(Tcl_Interp* interp /* may be NULL */) //<<<
{
	struct dedup_pool*	new_pool = NULL;

	new_pool = ckalloc(sizeof(*new_pool));
	Tcl_InitHashTable(&new_pool->kc, TCL_STRING_KEYS);
	new_pool->kc_count = 0;
	memset(&new_pool->freemap, 0xFF, sizeof(new_pool->freemap));
	new_pool->tcl_empty = NULL;

	if (interp)
		replace_tclobj(&new_pool->tcl_empty, Tcl_GetAssocData(interp, "hyprdom_dedup_tclempty", NULL));

	if (new_pool->tcl_empty == NULL) {
		replace_tclobj(&new_pool->tcl_empty, Tcl_NewObj());

		if (interp) {
			Tcl_IncrRefCount(new_pool->tcl_empty);	// AssocData ref
			Tcl_SetAssocData(interp, "hyprdom_dedup_tclempty", free_hyprdom_dedup_tclempty, new_pool->tcl_empty);
		}
	}

	return new_pool;
}

//>>>
void free_dedup_pool(struct dedup_pool* p) //<<<
{
	Tcl_HashEntry*		he;
	Tcl_HashSearch		search;
	struct kc_entry*	e;

	he = Tcl_FirstHashEntry(&p->kc, &search);
	while (he) {
		ptrdiff_t	idx = (ptrdiff_t)Tcl_GetHashValue(he);

		//if (idx >= KC_ENTRIES) Tcl_Panic("age_cache: idx (%ld) is out of bounds, KC_ENTRIES: %d", idx, KC_ENTRIES);
		//printf("age_cache: kc_count: %d", p->kc_count);
		e = &p->kc_entries[idx];

		Tcl_DeleteHashEntry(he);
		Tcl_DecrRefCount(e->val);
		Tcl_DecrRefCount(e->val);	// Two references - one for the cache table and one on loan to callers' interim processing
		mark_free(p->freemap, idx);
		e->val = NULL;
		he = Tcl_NextHashEntry(&search);
	}
	p->kc_count = 0;
	replace_tclobj(&p->tcl_empty, NULL);

	ckfree(p); p = NULL;
}

//>>>
static void age_cache(struct dedup_pool* p) //<<<
{
	Tcl_HashEntry*		he;
	Tcl_HashSearch		search;
	struct kc_entry*	e;

	he = Tcl_FirstHashEntry(&p->kc, &search);
	while (he) {
		ptrdiff_t	idx = (ptrdiff_t)Tcl_GetHashValue(he);

		//if (idx >= KC_ENTRIES) Tcl_Panic("age_cache: idx (%ld) is out of bounds, KC_ENTRIES: %d", idx, KC_ENTRIES);
		//printf("age_cache: kc_count: %d", p->kc_count);
		e = &p->kc_entries[idx];

		if (e->hits < 1) {
			Tcl_DeleteHashEntry(he);
			Tcl_DecrRefCount(e->val);
			Tcl_DecrRefCount(e->val);	// Two references - one for the cache table and one on loan to callers' interim processing
			mark_free(p->freemap, idx);
			e->val = NULL;
		} else {
			e->hits >>= 1;
		}
		he = Tcl_NextHashEntry(&search);
	}
	p->kc_count = 0;
}

//>>>
Tcl_Obj* new_stringobj_dedup(struct dedup_pool* p, const char* bytes, int length) //<<<
{
	char				buf[STRING_DEDUP_MAX + 1];
	const char			*keyname;
	int					is_new;
	struct kc_entry*	kce;
	Tcl_Obj*			out;
	Tcl_HashEntry*		entry = NULL;

	if (p == NULL)
		return Tcl_NewStringObj(bytes, length);

	if (length == 0) {
		return p->tcl_empty;
	} else if (length < 0) {
		length = strlen(bytes);
	}

	if (length > STRING_DEDUP_MAX)
		return Tcl_NewStringObj(bytes, length);

	if (likely(bytes[length] == 0)) {
		keyname = bytes;
	} else {
		memcpy(buf, bytes, length);
		buf[length] = 0;
		keyname = buf;
	}
	entry = Tcl_CreateHashEntry(&p->kc, keyname, &is_new);

	if (is_new) {
		ptrdiff_t	idx = first_free(p->freemap);

		if (unlikely(idx >= KC_ENTRIES)) {
			// Cache overflow
			Tcl_DeleteHashEntry(entry);
			age_cache(p);
			return Tcl_NewStringObj(bytes, length);
		}

		kce = &p->kc_entries[idx];
		kce->hits = 0;
		out = kce->val = Tcl_NewStringObj(bytes, length);
		Tcl_IncrRefCount(out);	// Two references - one for the cache table and one on loan to callers' interim processing.
		Tcl_IncrRefCount(out);	// Without this, values not referenced elsewhere could reach callers with refCount 1, allowing
								// the value to be mutated in place and corrupt the state of the cache (hash key not matching obj value)

		mark_used(p->freemap, idx);

		Tcl_SetHashValue(entry, (void*)idx);
		p->kc_count++;

		if (unlikely(p->kc_count > (int)(KC_ENTRIES/2.5))) {
			kce->hits++; // Prevent the just-created entry from being pruned
			age_cache(p);
		}
	} else {
		ptrdiff_t	idx = (ptrdiff_t)Tcl_GetHashValue(entry);

		kce = &p->kc_entries[idx];
		out = kce->val;
		if (kce->hits < 255) kce->hits++;
	}

	return out;
}

//>>>

#endif

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

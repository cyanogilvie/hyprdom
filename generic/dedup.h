#ifndef _DEDUP_H
#define _DEDUP_H

#if DEDUP
#define STRING_DEDUP_MAX	16

struct dedup_pool* new_dedup_pool(Tcl_Interp* interp /* may be NULL */);
void free_dedup_pool(struct dedup_pool* p);
Tcl_Obj* new_stringobj_dedup(struct dedup_pool* p, const char* bytes, int length);
void dedup_stats(Tcl_DString* ds, struct dedup_pool* p);
#	define get_string(p, bytes, length)	new_stringobj_dedup(p, bytes, length)
#else
#	define free_dedup_pool(p)	// nop
#	define get_string(p, bytes, length) Tcl_NewStringObj(bytes, length)
#	define dedup_stats(ds, p)
#endif

struct kc_entry {
	Tcl_Obj			*val;
	unsigned int	hits;
};

/* Find the best BSF (bit-scan-forward) implementation available:
 * In order of preference:
 *    - __builtin_ffsll     - provided by gcc >= 3.4 and clang >= 5.x
 *    - ffsll               - glibc extension, freebsd libc >= 7.1
 *    - ffs                 - POSIX, but only on int
 * TODO: possibly provide _BitScanForward implementation for Visual Studio >= 2005?
 */
#if defined(HAVE___BUILTIN_FFSLL) || defined(HAVE_FFSLL)
#	define FFS_TMP_STORAGE	/* nothing to declare */
#	if defined(HAVE___BUILTIN_FFSLL)
#		define FFS				__builtin_ffsll
#	else
#		define FFS				ffsll
#	endif
#	define FREEMAP_TYPE		long long
#	define KC_ENTRIES		6*8*sizeof(FREEMAP_TYPE)	// Must be an integer multiple of 8*sizeof(FREEMAP_TYPE)
#elif defined(_MSC_VER) && defined(_WIN64) && _MSC_VER >= 1400
#	define FFS_TMP_STORAGE	unsigned long ix;
/* _BitScanForward64 numbers bits starting with 0, ffsll starts with 1 */
#	define FFS(x)			(_BitScanForward64(&ix, x) ? ix+1 : 0)
#	define FREEMAP_TYPE		long long
#	define KC_ENTRIES		6*8*sizeof(FREEMAP_TYPE)	// Must be an integer multiple of 8*sizeof(FREEMAP_TYPE)
#else
#	define FFS_TMP_STORAGE	/* nothing to declare */
#	define FFS				ffs
#	define FREEMAP_TYPE		int
#	define KC_ENTRIES		12*8*sizeof(FREEMAP_TYPE)	// Must be an integer multiple of 8*sizeof(FREEMAP_TYPE)
#endif

struct dedup_pool {
	Tcl_HashTable	kc;
	int				kc_count;
	FREEMAP_TYPE	freemap[(KC_ENTRIES / (8*sizeof(FREEMAP_TYPE)))+1];	// long long for ffsll
	struct kc_entry	kc_entries[KC_ENTRIES];

	// TODO: copy these in from an interp cx
	Tcl_Obj*		tcl_empty;
};

#endif

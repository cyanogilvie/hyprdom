#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)
builtin(include,ax_gcc_builtin.m4)

#
# Add here whatever m4 macros you want to define for your package
#

AC_DEFUN([ENABLE_DEDUP], [
	#trap 'echo "val: (${enable_dedup+set}), dedup_ok: ($dedup_ok), DEDUP: ($DEDUP)"' DEBUG
	AC_MSG_CHECKING([whether to use a string deduplication mechanism for short strings])
	AC_ARG_ENABLE(dedup,
		AC_HELP_STRING([--enable-dedup], [Parsing JSON involves allocating a lot of small string Tcl_Objs, many of which are duplicates.  This mechanism helps reduce that duplication (default: yes)]),
		[dedup_ok=$enableval], [dedup_ok=yes])

	if test "$dedup_ok" = "yes" -o "${DEDUP}" = 1; then
		DEDUP=1
		AC_MSG_RESULT([yes])
	else
		DEDUP=0
		AC_MSG_RESULT([no])
	fi

	AC_DEFINE_UNQUOTED([DEDUP], [$DEDUP], [Dedup enabled?])
	#trap '' DEBUG
])


AC_DEFUN([TIP445], [
	AC_MSG_CHECKING([whether we need to polyfill TIP 445])
	saved_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $TCL_INCLUDE_SPEC"
	AC_TRY_COMPILE([#include <tcl.h>], [Tcl_ObjIntRep ir;],
	    have_tcl_objintrep=yes, have_tcl_objintrep=no)
	CFLAGS="$saved_CFLAGS"

	if test "$have_tcl_objintrep" = yes; then
		AC_DEFINE(TIP445_SHIM, 0, [Do we need to polyfill TIP 445?])
		AC_MSG_RESULT([no])
	else
		AC_DEFINE(TIP445_SHIM, 1, [Do we need to polyfill TIP 445?])
		AC_MSG_RESULT([yes])
	fi
])


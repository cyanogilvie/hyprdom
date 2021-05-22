#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)

#
# Add here whatever m4 macros you want to define for your package
#
builtin(include,ax_gcc_builtin.m4)
builtin(include,ax_cc_for_build.m4)
builtin(include,expat.m4)

# We need a modified version of TEA_ADD_SOURCES because some of those files will be
# generated *after* the macro runs, so it can't test for existence:
AC_DEFUN([HYPRDOM_ADD_SOURCES], [
    vars="$@"
    for i in $vars; do
	case $i in
	    [\$]*)
		# allow $-var names
		PKG_SOURCES="$PKG_SOURCES $i"
		PKG_OBJECTS="$PKG_OBJECTS $i"
		;;
	    *)
		PKG_SOURCES="$PKG_SOURCES $i"
		# this assumes it is in a VPATH dir
		i=`basename $i`
		# handle user calling this before or after TEA_SETUP_COMPILER
		if test x"${OBJEXT}" != x ; then
		    j="`echo $i | sed -e 's/\.[[^.]]*$//'`.${OBJEXT}"
		else
		    j="`echo $i | sed -e 's/\.[[^.]]*$//'`.\${OBJEXT}"
		fi
		PKG_OBJECTS="$PKG_OBJECTS $j"
		;;
	esac
    done
    AC_SUBST(PKG_SOURCES)
    AC_SUBST(PKG_OBJECTS)
])


AC_DEFUN([ENABLE_DEDUP], [
	#trap 'echo "val: (${enable_dedup+set}), dedup_ok: ($dedup_ok), DEDUP: ($DEDUP)"' DEBUG
	AC_MSG_CHECKING([whether to use a string deduplication mechanism for short strings])
	AC_ARG_ENABLE(dedup,
		AC_HELP_STRING([--enable-dedup], [Parsing XML involves allocating a lot of small string Tcl_Objs, many of which are duplicates.  This mechanism helps reduce that duplication (default: yes)]),
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


AC_DEFUN([RE2C], [
	AC_MSG_CHECKING([For re2c])
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


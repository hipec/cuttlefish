# check required autoconf version
AC_PREREQ([2.59])

# define distribution properties
AC_INIT([cuttlefish], [0.1], [HiPEC@IIITD])

# check if srcdir is correct
AC_CONFIG_SRCDIR([inc/cuttlefish.h])

AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.9 tar-ustar no-define foreign dist-bzip2])

AC_COPYRIGHT([])

# configure script revision
AC_REVISION([$Revision: 0.1 $])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG(C++)

CPPFLAGS=$CPPFLAGS_BACKUP

###################################################################
########### GET LOW LEVEL DETAILS IN THE INSTALLATION #############
###################################################################

### ENABLE MOTIVATION
AC_ARG_ENABLE(motivation,
    AS_HELP_STRING([--enable-motivation],
    [turn on HC motivation policy (Default is false)]),
    [with_motivation=$enableval],
    [with_motivation=no;])

AS_IF([test "x$with_motivation" != xno],
      [ AC_MSG_NOTICE([Enabled HC motivation policy]) ],
      [ AC_MSG_NOTICE([Disabled HC motivation policy]) ])

AM_CONDITIONAL(MOTIVATION, test "x$with_motivation" != xno)
### End MOTIVATION

### ENABLE CORE
AC_ARG_ENABLE(core,
    AS_HELP_STRING([--enable-core],
    [turn on HC core policy (Default is false)]),
    [with_core=$enableval],
    [with_core=no;])

AS_IF([test "x$with_core" != xno],
      [ AC_MSG_NOTICE([Enabled HC core policy]) ],
      [ AC_MSG_NOTICE([Disabled HC core policy]) ])

AM_CONDITIONAL(CORE, test "x$with_core" != xno)
### End CORE

### ENABLE UNCORE
AC_ARG_ENABLE(uncore,
    AS_HELP_STRING([--enable-uncore],
    [turn on HC uncore policy (Default is false)]),
    [with_uncore=$enableval],
    [with_uncore=no;])

AS_IF([test "x$with_uncore" != xno],
      [ AC_MSG_NOTICE([Enabled HC uncore policy]) ],
      [ AC_MSG_NOTICE([Disabled HC uncore policy]) ])

AM_CONDITIONAL(UNCORE, test "x$with_uncore" != xno)
### End UNCORE

### ENABLE COMBINED
AC_ARG_ENABLE(combined,
    AS_HELP_STRING([--enable-combined],
    [turn on HC combined policy (Default is false)]),
    [with_combined=$enableval],
    [with_combined=no;])

AS_IF([test "x$with_combined" != xno],
      [ AC_MSG_NOTICE([Enabled HC combined policy]) ],
      [ AC_MSG_NOTICE([Disabled HC combined policy]) ])

AM_CONDITIONAL(COMBINED, test "x$with_combined" != xno)
### End COMBINED

# Variable substitutions.
AC_SUBST([ac_aux_dir])
AC_SUBST([abs_top_srcdir])

#
# Config file to process
#
AC_CONFIG_FILES([
Makefile
src/Makefile
])

# Generates and runs config.status, which in turn creates the makefiles 
# and any other files resulting from configuration
AC_OUTPUT

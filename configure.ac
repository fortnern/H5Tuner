## Process this file with autoconf to produce a configure script.
##
## This file is part of h5tuner. The full h5tuner copyright notice,
##  including terms governing use, modification, and redistribution, is
## contained in the file COPYING, which can be found at the root of the
## source code distribution tree.  If you do not have access to this file,
## you may request a copy from help@hdfgroup.org.
##
##
##
## Initialize configure.
##
AC_PREREQ(2.69)

## AC_INIT takes the name of the package, the version number, and an
## email address to report bugs. AC_CONFIG_SRCDIR takes a unique file
## as its argument.
##
## NOTE: Do not forget to change the version number here when we do a
## release!!!
##
AC_INIT(autotuner, 0.1.1, fede@hdfgroup.org)

AC_CONFIG_SRCDIR([src/autotuner_hdf5.c])
AC_CONFIG_HEADERS([src/autotuner.h])

AC_CONFIG_MACRO_DIR([m4])

## AM_INIT_AUTOMAKE takes a list of options that should be applied to
## every Makefile.am when automake is run.
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

## AM_MAINTAINER_MODE turns off "rebuild rules" that contain dependencies
## for Makefiles, configure, src/H5config.h, etc. If AM_MAINTAINER_MODE
## is enabled, these files will be rebuilt if out of date. This is a
## problem because if users try to build on a machine with
## the wrong versions of autoconf and automake, these files will be
## rebuilt with the wrong versions and bad things can happen.
## Also, CVS doesn't preserve dependencies between timestamps, so
## Makefiles will often think rebuilding needs to occur when it doesn't.
##
## By default, it is enabled. Users can configure with
## --disable-maintainer-mode to prevent running the autotools.
AM_MAINTAINER_MODE([enable])

## ----------------------------------------------------------------------
## Set prefix default (install directory) to a directory in the build area.
## This allows multiple src-dir builds within one host.
AC_PREFIX_DEFAULT([`pwd`/h5tuner])

AC_PROG_CC

## AM_CFLAGS (and company) are for CFLAGS that should be used on h5tuner
AC_SUBST([AM_CFLAGS_SHARED])
AC_SUBST([AM_LDFLAGS_SHARED])
AC_SUBST([AM_ADDFLAGS])
AC_SUBST([AM_ADDFLAGS_SHARED])
AC_SUBST([AM_LIBS])


## Detect OS from shell
OS=`uname`

if test $OS = Darwin; then
  dnl On MacOS
  AM_LIBS="-lpthread -lz"
else
  dnl set for Linux
  AM_LIBS="-lpthread -lrt -lz"
fi

## ----------------------------------------------------------------------
## Dump all shell variables values.
##
AC_MSG_CHECKING([shell variables initial values])
set >&AS_MESSAGE_LOG_FD
AC_MSG_RESULT([done])

## set some flag values.  These may be moved to platform specific config
## files later.
AM_CFLAGS_SHARED="-g -shared -fpic -DPIC"
AM_LDFLAGS_SHARED=-ldl
AM_ADDFLAGS=-DDEBUG
AM_ADDFLAGS_SHARED=-DDEBUG


## ----------------------------------------------------------------------
## Check that the cache file was build on the same host as what we're
## running on now.
##
AC_CACHE_CHECK([for cached host], [hdf5_cv_host], [hdf5_cv_host="none"]);
if test $hdf5_cv_host = "none"; then
  hdf5_cv_host=$host
elif test $hdf5_cv_host != $host; then
  echo "The config.cache file was generated on $hdf5_cv_host but"
  echo "this is $host.  Please remove that file and try again."
  AC_MSG_ERROR([config.cache file is invalid])
fi

## ----------------------------------------------------------------------
## Source any special files that we need.  These files normally aren't
## present but can be used by the maintainers to fine tune things like
## turning on debug or profiling flags for the compiler.  The search order
## is:
##
##    CPU-VENDOR-OS
##    VENDOR-OS
##    CPU-OS
##    CPU-VENDOR
##    OS
##    VENDOR
##    CPU
##
## If the `OS' ends with a version number then remove it. For instance,
## `freebsd3.1' would become `freebsd'

case $host_os in
  aix*)
    host_os_novers=aix
    ;;
  freebsd*)
    host_os_novers=freebsd
    ;;
  solaris*)
    host_os_novers=solaris
    ;;
  *)
    host_os_novers=$host_os
    ;;
esac

host_config="none"
for f in $host_cpu-$host_vendor-$host_os \
         $host_cpu-$host_vendor-$host_os_novers \
         $host_vendor-$host_os \
         $host_vendor-$host_os_novers \
         $host_cpu-$host_os \
         $host_cpu-$host_os_novers \
         $host_cpu-$host_vendor \
         $host_os \
         $host_os_novers \
         $host_vendor \
         $host_cpu ; do
  AC_MSG_CHECKING([for config $f])
  if test -f "$srcdir/config/$f"; then
    host_config=$srcdir/config/$f
    AC_MSG_RESULT([found])
    break
  fi
  AC_MSG_RESULT([no])
done
if test "X$host_config" != "Xnone"; then
  CC_BASENAME="`echo $CC | cut -f1 -d' ' | xargs basename 2>/dev/null`"
  . $host_config
fi

## Source any special site-specific file
hname="`hostname`"
while test -n "$hname"; do
  file=$srcdir/config/site-specific/host-$hname
  AC_MSG_CHECKING([for config $file])
  if test -f "$file"; then
    . $file
    AC_MSG_RESULT([found])
    break
  fi
  AC_MSG_RESULT([no])
  hname_tmp=$hname
  hname="`echo $hname | cut -d. -f2-99`"
  test "$hname_tmp" = "$hname" && break
done

## ----------------------------------------------------------------------
## Enable dependency tracking unless the configure options or a
## site-specific file told us not to.  This prevents configure from
## silently disabling dependencies for some compilers.
##
if test -z "${enable_dependency_tracking}"; then
  enable_dependency_tracking="yes"
fi

## ----------------------------------------------------------------------
## Check for programs.
##
AC_PROG_CC
CC_BASENAME="`echo $CC | cut -f1 -d' ' | xargs basename 2>/dev/null`"

## Checkpoint the cache
AC_CACHE_SAVE

## Include the current directory
CPPFLAGS="$CPPFLAGS -I ."

# ----------------------------------------------------------------------
# Check for HDF5
AC_ARG_WITH([hdf5],
            [AC_HELP_STRING([--with-hdf5=DIR],
                            [Use hdf5 library [default=yes]])],,
            [withval=yes])

case "$withval" in
  yes)
    HAVE_HDF5="yes"
    AC_CHECK_HEADERS([hdf5.h],, [unset HAVE_HDF5])
    AC_CHECK_LIB([hdf5], [H5open],, [unset HAVE_HDF5])

    if test -z "$HAVE_HDF5"; then
      AC_MSG_ERROR([could not find hdf5 library])
    fi
    ;;
  no)
    AC_MSG_ERROR([hdf5 library required to build H5Tuner])
    ;;
  *)
    HAVE_HDF5="yes"
    case "$withval" in
      *,*)
        hdf5_inc="`echo $withval | cut -f1 -d,`"
        hdf5_lib="`echo $withval | cut -f2 -d, -s`"
        ;;
      *)
        if test -n "$withval"; then
          hdf5_inc="$withval/include"
          hdf5_lib="$withval/lib"
        fi
        ;;
    esac

    ## Trying to include -I/usr/include and -L/usr/lib is redundant and
    ## can mess some compilers up.
    if test "X$hdf5_inc" = "X/usr/include"; then
      hdf5_inc=""
    fi
    if test "X$hdf5_lib" = "X/usr/lib"; then
      hdf5_lib=""
    fi

    if test -n "$hdf5_inc"; then
      CPPFLAGS="$CPPFLAGS -I$hdf5_inc"
    fi

    AC_CHECK_HEADERS([hdf5.h],, [unset HAVE_HDF5])

    if test -n "$hdf5_lib"; then
      LDFLAGS="$LDFLAGS -L$hdf5_lib"
      AM_LDFLAGS_SHARED="$AM_LDFLAGS_SHARED -L$hdf5_lib"
    fi

    AC_CHECK_LIB([hdf5], [H5open],, [unset HAVE_HDF5])

    if test -z "$HAVE_HDF5"; then
      AC_MSG_ERROR([could not find hdf5 library])
    fi
    ;;
esac


# ----------------------------------------------------------------------
# Check for MXML
AC_ARG_WITH([mxml],
            [AC_HELP_STRING([--with-mxml=DIR],
                            [Use mxml library [default=yes]])],,
            [withval=yes])

case "$withval" in
  yes)
    HAVE_MXML="yes"
    AC_CHECK_HEADERS([mxml.h],, [unset HAVE_MXML])
    AC_CHECK_LIB([mxml], [mxmlLoadFile],, [unset HAVE_MXML])

    if test -z "$HAVE_MXML"; then
      AC_MSG_ERROR([could not find mxml library])
    fi
    ;;
  no)
    AC_MSG_ERROR([mxml library required to build H5Tuner])
    ;;
  *)
    HAVE_MXML="yes"
    case "$withval" in
      *,*)
        mxml_inc="`echo $withval | cut -f1 -d,`"
        mxml_lib="`echo $withval | cut -f2 -d, -s`"
        ;;
      *)
        if test -n "$withval"; then
          mxml_inc="$withval/include"
          mxml_lib="$withval/lib"
        fi
        ;;
    esac

    ## Trying to include -I/usr/include and -L/usr/lib is redundant and
    ## can mess some compilers up.
    if test "X$mxml_inc" = "X/usr/include"; then
      mxml_inc=""
    fi
    if test "X$mxml_lib" = "X/usr/lib"; then
      mxml_lib=""
    fi

    if test -n "$mxml_inc"; then
      CPPFLAGS="$CPPFLAGS -I$mxml_inc"
    fi

    AC_CHECK_HEADERS([mxml.h],, [unset HAVE_MXML])

    if test -n "$mxml_lib"; then
      LDFLAGS="$LDFLAGS -L$mxml_lib"
      AM_LDFLAGS_SHARED="$AM_LDFLAGS_SHARED -L$mxml_lib"
    fi

    AC_CHECK_LIB([mxml], [mxmlLoadFile],, [unset HAVE_MXML])

    if test -z "$HAVE_MXML"; then
      AC_MSG_ERROR([could not find mxml library])
    fi
    ;;
esac


##    RUNSERIAL            -- This is a command which will be prepended to
##               the executable name to run the executable using
##               a single process. For serial versions of the
##               library this will normally be empty. For parallel
##               versions it might be something like `mpiexec -n 1'.
##               The value of this variable is substituted in *.in
##               files.
##
##    RUNPARALLEL    -- This is a command which will be prepended to
##               the executable name to run the executable on
##               multiple processors. For the serial library the
##               value will normally be the empty string. For
##               parallel library it should be something like
##               "mpiexec -n \$\${NPROCS:=6}" where NPROCS will
##               eventually contain the number of processors on which
##               to run the executable (the double dollarsigns are to
##               protect the expansion until make executes the
##               command).  The value of this variable is
##               substituted in *.in files.
##
AC_SUBST([RUNSERIAL])
AC_SUBST([RUNPARALLEL])

## ----------------------------------------------------------------------
## Create libtool.  If shared/static libraries are going to be enabled
## or disabled, it should happen before these macros.
    LT_PREREQ([2.2])

LT_INIT([dlopen])


## Checkpoint the cache
AC_CACHE_SAVE

## Set RUNPARALLEL to mpiexec. 
if test -z "$RUNPARALLEL"; then
  RUNPARALLEL="mpiexec -n \$\${NPROCS:=3}"
fi

## Display what we found about running programs
AC_MSG_CHECKING([prefix for running on one processor])
AC_MSG_RESULT([$RUNSERIAL])
AC_MSG_CHECKING([prefix for running in parallel])
AC_MSG_RESULT([$RUNPARALLEL])

## There *must* be some way to run in parallel even if it's just the
## word `none'.
if test -z "$RUNPARALLEL"; then
  AC_MSG_ERROR([no way to run a parallel program])
fi

## If RUNSERIAL or RUNPARALLEL is the word `none' then replace it with
## the empty string. This means that no launch commands were requested,
## so we will not use any launch commands.
if test "X$RUNSERIAL" = "Xnone"; then
  RUNSERIAL=""
fi
if test "X$RUNPARALLEL" = "Xnone"; then
  RUNPARALLEL=""
fi


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 examples/Makefile
                 examples/config.xml
                 test/config.xml
                 evo/h5evolve_sample.py])

AC_OUTPUT

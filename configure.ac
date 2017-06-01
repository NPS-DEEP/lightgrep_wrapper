#                                  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Order is largely irrevellant, although it must start with AC_INIT and end with AC_OUTPUT
# See http://autotoolset.sourceforge.net/tutorial.html
# and http://www.openismus.com/documents/linux/automake/automake.shtml

AC_PREREQ(2.57)

AC_INIT(lightgrep_wrapper, 0.1.0, github.com/NPS-DEEP/lightgrep_wrapper/issues)
AC_CONFIG_MACRO_DIR(m4)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                ])

# define CC and CXX as desired for alternate compilers
# for example "./configure CC=clang CXX=clang++"
#CC=clang
#CXX=clang++

AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ENABLE_SHARED
AC_DISABLE_STATIC
LT_INIT
AC_PROG_LEX

# use C++11 mode if available; HAVE_CXX11 is defined in config.h if so.  Don't
# use the GNU C++11 extensions for portability's sake (noext).
# https://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_11.html
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
AX_CXX_COMPILE_STDCXX_11(noext, optional)

# Endian check is required for MD5 implementation
AC_C_BIGENDIAN

################################################################
## Includes
m4_include([m4/lightgrep_wrapper_cflags.m4])

################################################################
## establish default Python for tests and for SWIG
AM_PATH_PYTHON([2.7],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])
if test "$PYTHON" == : ; then
  AC_MSG_ERROR([Python is not available.])
fi

################################################################
## Linux
AC_DEFINE([__LINUX__],1,[Linux operating system functions])
CPPFLAGS="$CPPFLAGS -fPIC"

################################################################
# support for mallinfo for malloc usage reporting
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_LIB([stdc++],[mallinfo],
  [AC_DEFINE(HAVE_MALLINFO,1,[define 1 if stdc++ supports mallinfo])]
  ,[])

# Specific functions, may not be required
AC_TYPE_INT64_T
AC_SYS_LARGEFILE
AC_SYS_POSIX_TERMIOS

################################################################
# Should we disable optimization?
AC_ARG_WITH([opt], AC_HELP_STRING([--without-opt], [Drop all -O C flags]))
# Or maybe just tone it down a bit?
AC_ARG_WITH([o3], AC_HELP_STRING([--without-o3], [Do not force O3 optimization; use default level]))

################################################################
## libtool required for preparing the liblightgrep_wrapper.so library
AC_CHECK_PROG(has_libtool, libtool, true, false)
if test x"$has_libtool" == "xfalse" ; then
  AC_MSG_ERROR([libtool is required, please install libtool such as The GNU Portable Library Tool])
fi

################################################################
# do we have a broken strchr?
# This prevents errors in base64_forensics.cpp
AC_LANG_PUSH(C++)
AC_TRY_COMPILE([#include <string.h>],
               [static const char Base64[] = "ABCDEFGH";int ch=3;char *pos = strchr(Base64, ch);],
               conforming_strchr=yes,
               conforming_strchr=no)
if test "$conforming_strchr" = yes; then
  AC_MSG_NOTICE([strchr is conforming])
  AC_DEFINE(HAVE_CONFORMING_STRCHR, 1,[define to 1 if strchr takes const char*])
else
  AC_MSG_NOTICE([strchr is not conforming])
fi
AC_LANG_POP()

#################################################################
# Get the GIT commit into the GIT_COMMIT variable
AC_CHECK_PROG([git],[git],[yes],[no])
AM_CONDITIONAL([FOUND_GIT],[test "x$git" = xyes])
AM_COND_IF([FOUND_GIT],
        [GIT_COMMIT=`git describe --dirty --always`
         AC_MSG_NOTICE([git commit $GIT_COMMIT])],
        [AC_MSG_WARN([git not found])])

################################################################
############## drop optimization flags and add -g if requested ################
if test x"${AFF_NOOPT}" != "x" ; then
   with_noopt="yes";
fi

if test "${with_opt}" = "no" ; then
  CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O[[0-9]]//`             # note the double quoting!
  CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O[[0-9]]//`
else
  # If we are not stripping the optimizer, add fortify source
  CFLAGS="  $CFLAGS   -D_FORTIFY_SOURCE=2 "
  CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 "

  # and increase optimizer from -O2 to -O3 if not explicitly forbidden
  if test "${with_o3}" != "no" ; then
      CFLAGS=`echo   -g "$CFLAGS"   | sed s/-O2/-O3/`             # note the double quoting!
      CXXFLAGS=`echo -g "$CXXFLAGS" | sed s/-O2/-O3/`
  fi
fi

#################################################################
# We can enable code under development by defining HAVE_DEVEL,
# use "#ifdef HAVE_DEVEL"
AC_ARG_ENABLE([devel], AS_HELP_STRING([--enable-devel], \
              [Enable inclusion of code that is under development]), \
              AC_DEFINE(HAVE_DEVEL, 1, [Include code under development]))

#################################################################
# report some values
AC_MSG_NOTICE([
*************************************
*************************************
  PACKAGE_NAME:     $PACKAGE_NAME
  PACKAGE_VERSION:  $PACKAGE_VERSION
  CC:               $CC
  CXX:              $CXX
  CPPFLAGS:         $CPPFLAGS
  CFLAGS:           $CFLAGS
  CXXFLAGS:         $CXXFLAGS
  LIBS:             $LIBS
  LDFLAGS:          $LDFLAGS
])
  
AC_OUTPUT

## for git version
if test x"$GIT_COMMIT" != "x" ; then
  echo "#define GIT_COMMIT  \"$GIT_COMMIT\""  >> config.h
fi


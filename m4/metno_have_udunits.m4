##### http:///                                  -*- Autoconf -*-
#
# WARNING
#
#   This file is a copy of
#   'common_build_files/m4/metno_have_udunits.m4'.  The next time
#   'common_build_files/distribute.sh' is run with the appropriate
#   arguments, all changes to this file will disappear.  Please edit
#   the original.
#
# SYNOPSIS
#
#   METNO_HAVE_UDUNITS([ACTION-IF-TRUE], [ACTION-IF-FALSE])
#
# DESCRIPTION
#
#   This macro will check for the existence of the Unidata units
#   (udunits) library (http://www.unidata.ucar.edu/packages/udunits/).
#   The check is done by checking for the header file udunits.h and
#   the udunits library object file.  A --with-udunits option is
#   supported as well.  The following output variables are set with
#   AC_SUBST:
#
#     AC_SUBST(UDUNITS_CPPFLAGS)
#     AC_SUBST(UDUNITS_LDFLAGS)
#     AC_SUBST(UDUNITS_LIBS)
#
#   You can use them like this in Makefile.am:
#
#     AM_CPPFLAGS = $(UDUNITS_CPPFLAGS)
#     AM_LDFLAGS = $(UDUNITS_LDFLAGS)
#     program_LDADD = $(UDUNITS_LIBS)
#
#   Additionally, the C preprocessor symbol HAVE_UDUNITS will be
#   defined with AC_DEFINE([HAVE_UDUNITS]) if the library is
#   available.  Forthermore, the variable have_udunits will be set to
#   "yes" if the library is available.
#
# AUTHOR
#
#   Martin Thorsen Ranang <mtr@linpro.no>
#
# LAST MODIFICATION
#
#   $Date: 2007-12-07 23:53:49 +0100 (Fri, 07 Dec 2007) $
#
# ID
#
#   $Id: metno_have_udunits.m4 778 2007-12-07 22:53:49Z martinr $
#
# COPYLEFT
#
#   Copyright (c) 2007 Meteorologisk institutt <diana@met.no>
#
#   This program is free software: you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation, either version 3 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program. If not, see
#   <http://www.gnu.org/licenses/>.
#

# METNO_HAVE_UDUNITS([ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------------
AC_DEFUN([METNO_HAVE_UDUNITS], [
  AH_TEMPLATE([HAVE_UDUNITS], [Define if udunits is available])
  AC_ARG_WITH(udunits, 
              [  --with-udunits=DIR      prefix for Unidata units (udunits) library files and headers], 
              [if test "$withval" = "no"; then
                 ac_udunits_path=
                 $2
               elif test "$withval" = "yes"; then
                 ac_udunits_path=/usr
               else
                 ac_udunits_path="$withval"
               fi], 
              [ac_udunits_path=/usr])
  if test "$ac_udunits_path" != ""; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I$ac_udunits_path/include"
    AC_CHECK_HEADER([udunits.h], [
      AC_LANG_PUSH(C)
      saved_LDFLAGS="$LDFLAGS"
      LDFLAGS="$LDFLAGS -L$ac_udunits_path/lib"

      # Udunits needs a couple of (mathematical) functions.  The
      # following macro _will_ affect LIBS (before it is saved).
      AC_CHECK_LIB([m], [floor])
      AC_CHECK_FUNCS([fmod log10 ceil pow])
      
      saved_LIBS="$LIBS"
      AC_CHECK_LIB([udunits], [utInit],
        [AC_SUBST(UDUNITS_CPPFLAGS, [-I$ac_udunits_path/include])
         AC_SUBST(UDUNITS_LDFLAGS, [-L$ac_udunits_path/lib])
         AC_SUBST(UDUNITS_LIBS, [-ludunits])
         AC_DEFINE([HAVE_UDUNITS])
         have_udunits=yes
         $1
        ], [
        :
        $2
        ])
      AC_LANG_POP(C)
      LIBS="$saved_LIBS"
      LDFLAGS="$saved_LDFLAGS"
    ], [
      :
      $2
    ])
    CPPFLAGS="$saved_CPPFLAGS"
  fi
])

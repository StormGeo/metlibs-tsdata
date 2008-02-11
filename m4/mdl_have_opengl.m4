##### http://autoconf-archive.cryp.to/mdl_have_opengl.html
#
# SYNOPSIS
#
#   MDL_HAVE_OPENGL
#
# DESCRIPTION
#
#   Search for OpenGL. We search first for Mesa (a GPL'ed version of
#   Mesa) before a vendor's version of OpenGL, unless we were
#   specifically asked not to with `--with-Mesa=no' or
#   `--without-Mesa'.
#
#   The four "standard" OpenGL libraries are searched for: "-lGL",
#   "-lGLU", "-lGLX" (or "-lMesaGL", "-lMesaGLU" as the case may be)
#   and "-lglut".
#
#   All of the libraries that are found (since "-lglut" or "-lGLX"
#   might be missing) are added to the shell output variable "GL_LIBS",
#   along with any other libraries that are necessary to successfully
#   link an OpenGL application (e.g. the X11 libraries). Care has been
#   taken to make sure that all of the libraries in "GL_LIBS" are
#   listed in the proper order.
#
#   Additionally, the shell output variable "GL_CFLAGS" is set to any
#   flags (e.g. "-I" flags) that are necessary to successfully compile
#   an OpenGL application.
#
#   The following shell variable (which are not output variables) are
#   also set to either "yes" or "no" (depending on which libraries were
#   found) to help you determine exactly what was found.
#
#     have_GL
#     have_GLU
#     have_GLX
#     have_glut
#
# LAST MODIFICATION
#
#   2007-12-05 - Fixed the AC_ARG_{WITH,ENABLE} calls (<mtr@linpro.no>).
#
# COPYLEFT
#
#   Copyright (c) 2007 Matthew D. Langston
#   Copyright (c) 2007 Ahmet Inan <auto@ainan.org>
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#   02111-1307, USA.
#
#   As a special exception, the respective Autoconf Macro's copyright
#   owner gives unlimited permission to copy, distribute and modify the
#   configure scripts that are the output of Autoconf when processing
#   the Macro. You need not follow the terms of the GNU General Public
#   License when using or distributing such scripts, even though
#   portions of the text of the Macro appear in them. The GNU General
#   Public License (GPL) does govern all other use of the material that
#   constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the
#   Autoconf Macro released by the Autoconf Macro Archive. When you
#   make and distribute a modified version of the Autoconf Macro, you
#   may extend this special exception to the GPL to apply to your
#   modified version as well.

AC_DEFUN([MDL_HAVE_OPENGL],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])

  AC_CACHE_CHECK([for OpenGL], mdl_cv_have_OpenGL,
  [
dnl Check for Mesa first, unless we were asked not to.
    AC_ARG_WITH([mesa], [  --with-mesa=DIR         prefix for mesa library files and headers],
      [if test "$withval" = "no"; then
         ac_mesa_path=
         $2
       elif test "$withval" = "yes"; then
         ac_mesa_path=/usr
       else
         ac_mesa_path="$withval"
       fi],
      [ac_mesa_path=/usr])

if test "$ac_mesa_path" != ""; then
  AC_LANG_PUSH(C)  

  GL_save_CPPFLAGS="$CPPFLAGS"
  GL_save_LIBS="$LIBS"

  dnl If we are running under X11 then add in the appropriate libraries.
  if test x"$no_x" != xyes; then
  dnl Add everything we need to compile and link X programs to GL_X_CFLAGS
  dnl and GL_X_LIBS.
    GL_CFLAGS="$X_CFLAGS"
    GL_X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS"
  fi

  GL_CFLAGS="$GL_CFLAGS -I$ac_mesa_path/include"
  GL_X_LIBS="$GL_X_LIBS -L$ac_mesa_path/lib"

  CPPFLAGS="$GL_CFLAGS"

  LIBS="$GL_X_LIBS"

  # Save the "AC_MSG_RESULT file descriptor" to FD 8.
  exec 8>&AC_FD_MSG

  # Temporarily turn off AC_MSG_RESULT so that the user gets pretty
  # messages.
  exec AC_FD_MSG>/dev/null

  AC_CHECK_LIB([GL], [glAccum], [have_GL=yes], [have_GL=no])
  AC_CHECK_LIB([GLU], [gluBeginCurve], [have_GLU=yes], [have_GLU=no])
  AC_CHECK_LIB([GLX], [glXChooseVisual], [have_GLX=yes], [have_GLX=no])
  AC_CHECK_LIB([glut], [glutInit], [have_glut=yes], [have_glut=no])
  
  # Restore pretty messages.
  exec AC_FD_MSG>&8

  if test -n "$LIBS"; then
    mdl_cv_have_OpenGL=yes
    GL_LIBS="$LIBS"
    AC_SUBST(GL_CFLAGS)
    AC_SUBST(GL_LIBS)
  else
    mdl_cv_have_OpenGL=no
    GL_CFLAGS=
  fi

dnl Reset GL_X_LIBS regardless, since it was just a temporary variable
dnl and we don't want to be global namespace polluters.
  GL_X_LIBS=

  #AC_LANG_RESTORE
  AC_LANG_POP(C)

dnl bugfix: dont forget to cache this variables, too
    mdl_cv_GL_CFLAGS="$GL_CFLAGS"
    mdl_cv_GL_LIBS="$GL_LIBS"
    mdl_cv_have_GL="$have_GL"
    mdl_cv_have_GLU="$have_GLU"
    mdl_cv_have_GLX="$have_GLX"
    mdl_cv_have_glut="$have_glut"
  ])
  GL_CFLAGS="$mdl_cv_GL_CFLAGS"
  GL_LIBS="$mdl_cv_GL_LIBS"
  have_GL="$mdl_cv_have_GL"
  have_GLU="$mdl_cv_have_GLU"
  have_GLX="$mdl_cv_have_GLX"
  have_glut="$mdl_cv_have_glut"

  LIBS="$GL_save_LIBS"
  CPPFLAGS="$GL_save_CPPFLAGS"

fi
])
dnl endof bugfix -ainan

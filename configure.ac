AC_INIT([Test], [0.1], [Herp@Derp])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign silent-rules])
LT_INIT([disable-static])
LT_LANG([C])

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([GTK2], [gtk+-2.0 gthread-2.0 cairo cairo-png])
PKG_CHECK_MODULES([AL], [allegro-5.1 allegro_image-5.1])
PKG_CHECK_MODULES([AI], [assimp])
#PKG_CHECK_MODULES([COGL], [cogl-2.0])
#PKG_CHECK_MODULES([LUA52], [lua52])

#CPPFLAGS="$CPPFLAGS -DX_ERROR_STACK_TRACE_INSTEAD_OF_ASSERT"
CPPFLAGS="$CPPFLAGS -DX_ERROR_ASSERT_INSTEAD_OF_STACK_TRACE"
CFLAGS="$CFLAGS -std=gnu99"
CFLAGS="$CFLAGS -g -O0"

AC_SUBST([herp], ["derp"])

AC_CONFIG_FILES([hello Makefile])
AC_OUTPUT

#libdir = /lib, pkglibdir = /lib/@PACKAGE@/

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = main
main_SOURCES = src/main.c
main_LDADD = libhello.la

lib_LTLIBRARIES = libhello.la
libhello_la_SOURCES = src/hello.c

dist_data_DATA = help

bin_PROGRAMS += gl1
gl1_SOURCES = src/gl1.c src/error.c src/stage.c
gl1_CFLAGS = ${GTK2_CFLAGS} ${AL_CFLAGS} ${AI_CFLAGS} ${AM_CFLAGS}
gl1_LDADD = ${GTK2_LIBS} ${AL_LIBS} ${AI_LIBS} -lGL

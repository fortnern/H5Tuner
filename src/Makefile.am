#
# Copyright by The HDF Group.
# All rights reserved.
#
#
lib_LTLIBRARIES=libautotuner.la
#
libautotuner_la_SOURCES = autotuner_hdf5_static.c autotuner_hdf5.c autotuner_private.h

all: libautotuner_static.a libautotuner.so

autotuner_hdf5_static.o: autotuner_hdf5_static.c autotuner.h autotuner_private.h
				$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ $) @AM_ADDFLAGS@

autotuner_hdf5.po: autotuner_hdf5.c autotuner.h autotuner_private.h
				$(CC) $(CPPFLAGS) $(CFLAGS_SHARED) @AM_CFLAGS_SHARED@	$(LDFLAGS_SHARED) @AM_LDFLAGS_SHARED@ -c $< -o $@ @AM_ADDFLAGS_SHARED@

libautotuner_static.a: autotuner_hdf5_static.o
				ar rcs $@ $^

libautotuner.so: autotuner_hdf5.po
				$(CC) $(CFLAGS_SHARED) @AM_CFLAGS_SHARED@ $(LDFLAGS_SHARED) @AM_LDFLAGS_SHARED@ -o $@ $^ $(LIBS) @AM_LIBS@ @AM_ADDFLAGS_SHARED@

install: libautotuner_static.a libautotuner.so
				mkdir -p ${prefix}/lib
				cp libautotuner_static.a ${prefix}/lib/libautotuner_static.a
				cp libautotuner.so ${prefix}/lib/libautotuner.so

uninstall:
				rm -f ${prefix}/lib/libautotuner_static.a ${prefix}/lib/libautotuner.so

clean:
				rm -f *.o *.a *.so *.po *.la *.lo

include $(top_srcdir)/config/conclude.am

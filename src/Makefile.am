## Process this file with automake to produce Makefile.in.
##

include $(top_srcdir)/Makefile.common


pkglib_LTLIBRARIES =				\
	libGL.la


pkglib_LTLIBRARIES +=				\
	kroki-glxoffload-audit.la


kroki_glxoffload_audit_la_LDFLAGS =		\
	-module


nobase_pkglib_LTLIBRARIES =			\
	stub/libkroki-glxoffload.la


libGL_la_LDFLAGS =				\
	-version-info 1


libGL_la_LIBADD =				\
	stub/libkroki-glxoffload.la		\
	$(LIBADD_CLOCK_GETTIME)			\
	$(LIBADD_DLOPEN)			\
	$(LIBADD_CUCKOO_HASH)

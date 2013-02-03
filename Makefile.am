## Process this file with automake to produce Makefile.in.
##

include $(top_srcdir)/Makefile.common


ACLOCAL_AMFLAGS = -I m4


SUBDIRS =					\
        src/


bin_SCRIPTS =					\
	bin/kroki-glxoffload


EXTRA_DIST =					\
	version.sh				\
	kroki-glxoffload.spec			\
	README.md


rpm: dist
	HOME=$(abs_top_builddir) rpmbuild -tb $(distdir).tar.bz2
	mv $(top_builddir)/rpmbuild/RPMS/*/*.rpm $(top_builddir)
	rm -rf $(top_builddir)/rpmbuild
	rm $(distdir).tar.bz2

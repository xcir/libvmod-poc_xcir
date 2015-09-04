WAND_INCLUDES=`/usr/bin/Wand-config --cppflags`

AM_CPPFLAGS = @VMOD_INCLUDES@ $(WAND_INCLUDES) -Wall -Werror

vmoddir = @VMOD_DIR@
vmod_LTLIBRARIES = libvmod_poc_xcir.la

libvmod_poc_xcir_la_LDFLAGS = -module -export-dynamic -avoid-version -shared -lMagickWand

libvmod_poc_xcir_la_SOURCES = \
	vmod_vfp.c \
	vmod_smalllight_param.c \
	vmod_poc_xcir.c

nodist_libvmod_poc_xcir_la_SOURCES = \
	vcc_if.c \
	vcc_if.h

vmod_poc_xcir.lo: vcc_if.c vcc_if.h

vcc_if.c: vcc_if.h

vcc_if.h: @VMODTOOL@ $(top_srcdir)/src/vmod_poc_xcir.vcc
	@VMODTOOL@ $(top_srcdir)/src/vmod_poc_xcir.vcc

VMOD_TESTS = $(top_srcdir)/src/tests/*.vtc
.PHONY: $(VMOD_TESTS)

$(top_srcdir)/src/tests/*.vtc: libvmod_poc_xcir.la
	@VARNISHTEST@ -Dvarnishd=@VARNISHD@ -Dvmod_topbuild=$(abs_top_builddir) $@

check: $(VMOD_TESTS)

EXTRA_DIST = \
	vmod_poc_xcir.vcc \
	$(VMOD_TESTS)

CLEANFILES = \
	$(builddir)/vcc_if.c \
	$(builddir)/vcc_if.h \
	$(builddir)/vmod_poc_xcir.rst \
	$(builddir)/vmod_poc_xcir.man.rst

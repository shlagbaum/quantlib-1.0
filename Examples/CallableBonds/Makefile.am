
AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir}

if AUTO_EXAMPLES
bin_PROGRAMS = CallableBonds
TESTS = CallableBonds$(EXEEXT)
else
noinst_PROGRAMS = CallableBonds
endif
CallableBonds_SOURCES = CallableBonds.cpp
CallableBonds_LDADD = ../../ql/libQuantLib.la

EXTRA_DIST = \
    CallableBonds.dev \
    CallableBonds_vc7.vcproj \
    CallableBonds_vc8.vcproj \
    CallableBonds_vc9.vcproj \
    ReadMe.txt

.PHONY: examples check-examples

examples: CallableBonds$(EXEEXT)

check-examples: examples
	./CallableBonds$(EXEEXT)

dist-hook:
	mkdir -p $(distdir)/bin
	mkdir -p $(distdir)/build



AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir}

if AUTO_EXAMPLES
bin_PROGRAMS = Bonds
TESTS = Bonds$(EXEEXT)
else
noinst_PROGRAMS = Bonds
endif
Bonds_SOURCES = Bonds.cpp
Bonds_LDADD = ../../ql/libQuantLib.la

EXTRA_DIST = \
    Bonds.dev \
    Bonds_vc7.vcproj \
    Bonds_vc8.vcproj \
    Bonds_vc9.vcproj \
    ReadMe.txt

.PHONY: examples check-examples

examples: Bonds$(EXEEXT)

check-examples: examples
	./Bonds$(EXEEXT)

dist-hook:
	mkdir -p $(distdir)/bin
	mkdir -p $(distdir)/build


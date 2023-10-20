CXX = g++
MSVC = cl //nologo
MSASM = ml //nologo
MSCS = C:/WINDOWS/Microsoft.NET/Framework/v4.0.30319/csc //nologo
BOOSTDIR = C:/usr/boost
LIBDIR = C:/src

INCLUDES = -I$(BOOSTDIR) -I$(LIBDIR)/sys++ -I$(LIBDIR)/extlib
DEFS = -DEXT_USE_STD_STRING -DNOMINMAX -DUNICODE -D_UNICODE
CXXFLAGS = -Wall -Wno-unused-local-typedefs -pipe -std=c++11 -mtune=core2 -march=i686 -O2 $(DEFS) $(INCLUDES) -IC:/usr/include
LDFLAGS =
LDLIBS = -L$(LIBDIR)/sys++ -lsys++ -lpng -lz
MSVCFLAGS = //GF //EHsc //GS- //std:c++17 //O2 //W4 $(DEFS) $(INCLUDES) -IC:/usr/VC/include
MSVC_DEBUG_FLAGS = //EHsc //GS //Zi $(DEFS) $(INCLUDES) -IC:/usr/VC/include
MSVCLIBSx64 = $(LIBDIR)/sys++/sys++mt_x64.lib zlibstat_x64.lib libpng16_x64.lib
MSVCLIBSx86 = $(LIBDIR)/sys++/sys++.lib zlibstat_x86.lib libpng16_x86.lib

.SUFFIXES: .o .obj _x64.obj _x86.obj .cc .cpp .c .cs .asm .exe

depias: depias.obj pias_sjis.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

dextt: dextt.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

deuk2: deuk2.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

advb: advb.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

unadvc: unadvc.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

adviz: adviz.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

desdt: desdt.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

desda: desda.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

deadv: deadv.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

deism: deism.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

.cc.obj:
	$(MSVC) $(MSVCFLAGS) -c $<

.cc_x64.obj:
	$(MSVC) $(MSVCFLAGS) //Fo$@ -c $<

.cc_x86.obj:
	$(MSVC) $(MSVCFLAGS) //Fo$@ -c $<

.cpp.obj:
	$(MSVC) $(MSVCFLAGS) -c $<

.c.obj:
	$(MSVC) $(MSVCFLAGS) -c $<

.asm.obj:
	$(MSASM) //c $< //Fo$@

.cs.exe:
	$(MSCS) $(MSCSFLAGS) //out:$@ $^

clean:
	rm -f $(OBJECTS_GCC) $(OBJECTS_VC)

tags:
	ctags *.cc *.hpp

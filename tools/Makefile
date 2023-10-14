MSVC = cl //nologo
MSCS = C:/WINDOWS/Microsoft.NET/Framework/v4.0.30319/csc //nologo
BOOSTDIR = C:/usr/boost
LIBDIR = C:/src
INCLUDES = -I$(BOOSTDIR) -Isys++
Platform ?= x86
OBJDIR = ./$(Platform)
DEFS = -DEXT_USE_STD_STRING -DNOMINMAX -DUNICODE -D_UNICODE
MSVCFLAGS = //GF //EHsc //GS- //O2 $(DEFS) $(INCLUDES) -IC:/usr/VC/include
MSVCLIBSx64 = $(LIBDIR)/sys++/sys++mt_x64.lib zlibstat_x64.lib libpng16_x64.lib
MSVCLIBSx86 =  $(LIBDIR)/sys++/sys++.lib zlibstat_x86.lib libpng16_x86.lib

.SUFFIXES: .o .obj .cc .cpp .c .exe

setup:
	mkdir $(OBJDIR)

pngblend: $(OBJDIR)/pngblend.obj $(OBJDIR)/png-convert.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(MSVCLIBSx86) user32.lib

pngalpha: $(OBJDIR)/pngalpha.obj $(OBJDIR)/png-convert.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(MSVCLIBSx86)

pngmask: $(OBJDIR)/pngmask.obj $(OBJDIR)/png-convert.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(MSVCLIBSx86)

jpgmask: $(OBJDIR)/jpgmask.obj $(OBJDIR)/png-convert.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(MSVCLIBSx86) libjpeg.lib

u16jpstr: $(OBJDIR)/u16jpstr.obj $(OBJDIR)/sjis-table.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++mt.lib

jpstrings: $(OBJDIR)/jpstrings.obj $(OBJDIR)/sjis-table.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++.lib

rtfenc: $(OBJDIR)/rtfenc.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++.lib

unlzss: $(OBJDIR)/unlzss.obj $(OBJDIR)/lzss.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++.lib

unzlib: $(OBJDIR)/unzlib.obj
	$(MSVC) $(MSVCFLAGS) $^ //Fe$@.exe $(LIBDIR)/sys++/sys++.lib zlibstat.lib

$(OBJDIR)/%.obj: %.cc
	$(MSVC) $(MSVCFLAGS) -c $< //Fo$@

.cc.obj:
	$(MSVC) $(MSVCFLAGS) -c $<

.cpp.obj:
	$(MSVC) $(MSVCFLAGS) -c $<

.c.obj:
	$(MSVC) $(MSVCFLAGS) -c $<

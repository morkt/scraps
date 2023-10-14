CXX = g++
MSVC = cl //nologo
BOOSTDIR = C:/usr/boost
INCLUDES = -I$(BOOSTDIR) -I.. -I../../sys++
#CXXFLAGS = -Wall -W -Wno-parentheses -std=c++0x -pg -minline-all-stringops -finline-functions -finline-small-functions -DNOMINMAX $(INCLUDES) -march=i386 -mtune=core2
CXXFLAGS = -pg -Wall -W -Wno-parentheses -std=c++11 -O3 -minline-all-stringops $(DEFS) $(INCLUDES) -march=i686 -mtune=core2
DEFS = -DNOMINMAX
LDFLAGS = -pg
LIBS =
MSVCFLAGS = //MD //GF //EHsc //GR //O2 $(DEFS) $(INCLUDES)
VCLIBS = ../../sys++/sys++md.lib

timing.o: CXXFLAGS += -mthreads
timing_gw: LDFLAGS += -mthreads
timing_gw timing_vc: DEFS += -DEXT_MT -DEXT_FORMAT_USE_TABLE
timing_gw: LIBS += ../../sys++/libsys++mt.a -LC:/usr/boost_1_50_0/stage/lib -lboost_thread-mgw47-mt-1_50 -lboost_system-mgw47-mt-1_50 -lboost_timer-mgw47-mt-1_50 -lboost_chrono-mgw47-mt-1_50
timing_notable_gw: LIBS += ../../sys++/libsys++.a -LC:/usr/boost_1_50_0/stage/lib -lboost_timer-mgw47-1_50 -lboost_chrono-mgw47-1_50 -lboost_system-mgw47-1_50
timing_vc: VCFLAGS += //MD

.SUFFIXES: .o .obj _cyg.o _cyg.s .cc .s

timing: timing_vc timing_gw timing_notable_gw timing_notable_vc

timing.obj timing.o: timing.cc ../format.hpp ../lstring.hpp parsing.hpp

timing_notable_gw: timing.cc ../format.hpp ../lstring.hpp parsing.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

timing_notable_vc: timing.cc ../format.hpp ../lstring.hpp parsing.hpp
	$(MSVC) $(MSVCFLAGS) $(VCFLAGS) //Fe$@.exe $< $(VCLIBS) //link /LIBPATH:$(BOOSTDIR)/stage/lib

.cc.obj:
	$(MSVC) $(MSVCFLAGS) //c $<

.cc_cyg.o:
	g++ $(CXXFLAGS) -o $@ -c $<

%_vc: %.obj
	$(MSVC) $(VCFLAGS) //Fe$@.exe $< $(VCLIBS) //link /LIBPATH:$(BOOSTDIR)/stage/lib

%_gw: %.o
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o *.obj

.PHONY: timing

# cxx.mk	Copyright 1994 Z-Code Software Corp.

#
# Definitions for compiling C++ sources
#

.SUFFIXES:	.cc

.cc.$O:
	$(CXX) $(OPTIMIZE) $(CXXPPFLAGS) $(CXXFLAGS) $(ZCFLAGS) -c $*.cc

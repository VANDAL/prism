CUNIT:=/home/mdlui/Code/CUnit
CHECK:=/home/mdlui/Code/Check
SRCDIR:=..

SGLDIR:=/home/mdlui/Code/SigilNew/src/Sigil2
PRIMDIR:=$(SGLDIR)/primitives
TESTDIR:=$(SGLDIR)/test
BUILDDIR:=$(SGLDIR)/build


CC:=gcc
CXX:=g++
CXXFLAGS+= -std=c++11 -Wall -g

CXXFLAGS+= -I$(SGLDIR)
CXXFLAGS+= -I$(PRIMDIR)
CXXFLAGS+= -I$(TESTDIR)

default:

check:
	$(CXX) $(FLAGS) $(CHECK_FLAGS) -o CheckTest CheckTest.c

FrontEnd:
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/SglIface.o -c $(SGLDIR)/SglIface.cpp

BackEnd:
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/SglBackEnd.o -c $(SGLDIR)/SglBackEnd.cpp 

Primitives:
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/SglPrimitives.o -c $(PRIMDIR)/SglPrimitives.cpp 


tester: FrontEnd BackEnd Primitives
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/BEDummy.o -c $(TESTDIR)/BEDummy.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/randgen.o -c $(TESTDIR)/workload_gen/test.cc
	$(CXX) -o $(BUILDDIR)/tester \
		$(BUILDDIR)/randgen.o \
		$(BUILDDIR)/SglIface.o \
		$(BUILDDIR)/BEDummy.o \
		$(BUILDDIR)/SglPrimitives.o \
		$(BUILDDIR)/SglBackEnd.o

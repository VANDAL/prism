CUNIT:=/home/mdlui/Code/CUnit
CHECK:=/home/mdlui/Code/Check
SRCDIR:=..

SGLDIR:=/home/mdlui/Code/SigilNew/src/Sigil2
TESTDIR:=$(SGLDIR)/test
BUILDDIR:=$(SGLDIR)/../../obj

DUMMYDIR:=Dummy


CC:=gcc
CXX:=g++
CXXFLAGS+= -std=c++11 -Wall -g

CXXFLAGS+= -I$(SGLDIR)
CXXFLAGS+= -I$(TESTDIR)

CXXFLAGS+= -I$(DUMMYDIR)

.PHONY: Dummy

default:

check:
	$(CXX) $(FLAGS) $(CHECK_FLAGS) -o CheckTest CheckTest.c

Interface:
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/SglIface.o -c $(SGLDIR)/SglIface.cpp

Events:
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/SglEvents.o -c $(SGLDIR)/SglEvents.cpp 

Dummy:
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/Dummy.o -c $(DUMMYDIR)/Dummy.c 
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/DummyGlue.o -c $(DUMMYDIR)/DummyGlue.cpp


tester: FrontEnd BackEnd Primitives
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/BEDummy.o -c $(TESTDIR)/BEDummy.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILDDIR)/randgen.o -c $(TESTDIR)/workload_gen/test.cc
	$(CXX) -o $(BUILDDIR)/tester \
		$(BUILDDIR)/randgen.o \
		$(BUILDDIR)/SglIface.o \
		$(BUILDDIR)/BEDummy.o \
		$(BUILDDIR)/SglPrimitives.o \
		$(BUILDDIR)/SglBackEnd.o


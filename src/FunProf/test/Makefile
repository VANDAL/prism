CUNIT:=/home/mdlui/Code/CUnit
CHECK:=/home/mdlui/Code/Check
SRCDIR:=..

CC:=gcc
FLAGS+= -x c -std=c99 -Wall -g
FLAGS+= -I$(SRCDIR)

CUNIT_FLAGS+= -I$(CUNIT)/include
CUNIT_FLAGS+= -Wl,-rpath=$(CUNIT)/lib -L$(CUNIT)/lib -lcunit 

CHECK_FLAGS+= -I$(CHECK)/include
CHECK_FLAGS+= -Wl,-rpath=$(CHECK)/lib -L$(CHECK)/lib -lcheck 

default:

cunit:
	$(CC) $(FLAGS) $(CUNIT_FLAGS) -o CUnitTest CUnitTest.c

check:
	$(CC) $(FLAGS) $(CHECK_FLAGS) -o CheckTest CheckTest.c

shadow:
	rm -rf ./shad
	$(CC) $(FLAGS) $(CHECK_FLAGS) -o shad ../ReaderMemPool.c ../Common.c test_FnShadowMemory.c

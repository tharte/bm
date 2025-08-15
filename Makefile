# Defaults:
CPP_STD            =  --std=c++17
DEBUG              =  0
GDB                =  0

INCLUDE            =  -I.

BLANK_LINE         =  @echo
EXE                =

ifeq ($(DEBUG),1)
	OPTIMFLAGS = -g -D_DEBUG $(CPP_STD)
	EXECUTING  =   @echo debug version compiled
	ifeq ($(GDB),1)
		EXECUTE    =  gdb $@
	else
		EXECUTE    =  # don't execute - just create executable for debugging
	endif
else
	OPTIMFLAGS =  -DNDEBUG -Wall -O3  $(CPP_STD)
	EXECUTING  =  @echo "---***" executing "***---" "'"$@"'":
	EXECUTE    =  @./$@
endif

# ifdef out code anywhere
ifeq (0,1)
endif

bm$(EXE): bm.cpp utils.h
	g++ $(OPTIMFLAGS) $(INCLUDE) $< -o $@
	$(EXECUTING)
	$(EXECUTE)

scratch$(EXE): scratch.cpp
	g++ $(OPTIMFLAGS) $(INCLUDE) $< -o $@
	$(EXECUTING)
	$(EXECUTE)

clean:
	rm -rf *.o \

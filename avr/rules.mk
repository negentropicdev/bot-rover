#These are the command-line targets that are specialized for the non-recursive
#make used with this build system.

.PHONY: all

all:

#debug target adds debugger flags to compiler flags
.PHONY: debug
debug: CXXFLAGS+= -DDEBUG -ggdb
debug: CFLAGS+= -DDEBUG -ggdb
debug: all

#clean erases any *~ autosave files and empties the build folder. archived
#libraries and linked binaries are left
.PHONY: clean
clean: scratch=$(shell find . -name "*~")
clean:
	$(rm) $(BUILD_DIR)/* $(scratch)

#realclean also empties the lib and bin folders
.PHONY: realclean
realclean: clean
	$(rm) $(LIB_DIR)/* $(BIN_DIR)/*

#if any automatically generated dependency files have been created, include them
deps=$(wildcard $(BUILD_DIR)/*.d)
-include $(deps)

#generic rules for working on individual target files
%.o: %.c
	$(cc)

%.o: %.cpp
	$(cxx)

%: %.o
	$(ld)

%.a:
	$(ar)

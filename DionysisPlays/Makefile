#  FILE = "/src/mystation.c"
#  Created by Dionysis Taxiarchis Balaskas on 2/12/19.
#  Copyright © 2019 D.T.Balaskas. All rights reserved.
#####################################################################
.PHONY: clean default run
CXX= gcc
CXXFLAGS= -Wall -g3
LDLIBS= -lrt
RM= rm -f
GDBFLAGS = -ggdb3

BDIR= bin
SRCDIR= src
SRC= $(wildcard $(SRCDIR)/*.c)
IDIR= include
DEPS= $(wildcard $(IDIR)/*.h)
ODIR= build

default: cfs cfs_functions cfs_commands list
	@echo "Compiling Project..."
test:
	@echo "source directory: " $(SRCDIR)
	@echo "include directory: " $(IDIR)
	@echo "object directory: " $(ODIR)
	@echo "executable directory: " $(BDIR)
	@echo "At compilation we are adding the libraries: " $(LDLIBS)
$(ODIR)/%.o: $(SRCDIR)/%.c
	@echo "Creating object " $@ "..."
	$(CXX) -c -o $@ $< $(CXXFLAGS)
cfs: $(ODIR)/cfs.o $(ODIR)/cfs_functions.o $(ODIR)/cfs_commands.o $(ODIR)/list.o
	@echo "Creating cfs..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
cfs_commands: $(ODIR)/cfs_commands.o
	@echo "Creating cfs_commands..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
cfs_functions: $(ODIR)/cfs_functions.o
	@echo "Creating cfs_functions..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
list: $(ODIR)/list.o
	@echo "Creating list..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
clean:
	@echo "Cleaning up..."
	$(RM) $(ODIR)/*
	$(RM) $(BDIR)/*
debug:
	@echo "Debugging cfs..."
	$(CXX) $(GDBFLAGS) $(SRCDIR)/cfs.c $(SRCDIR)/cfs_commands.c $(SRCDIR)/cfs_functions.c $(SRCDIR)/list.c -o $(BDIR)/cfs
	gdb $(BDIR)/cfs
run:
	@echo "Executing cfs..."
	./$(BDIR)/cfs
commands:
	@echo "Executing cfs from command_list..."
	$(RM) myfiles.cfs
	$(RM) -r ./doc/myfiles.cfs
	./$(BDIR)/cfs < ./command_list

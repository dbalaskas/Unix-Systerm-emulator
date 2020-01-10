#  FILE = "/src/mystation.c"
#  Created by Dionysis Taxiarchis Balaskas on 2/12/19.
#  Copyright © 2019 D.T.Balaskas. All rights reserved.
#####################################################################
.PHONY: default clean run
CXX= gcc
CXXFLAGS= -Wall -g3
LDLIBS= -lpthread -lrt
RM= rm -f

BDIR= bin
SRCDIR= src
SRC= $(wildcard $(SRCDIR)/*.c)
IDIR= include
DEPS= $(wildcard $(IDIR)/*.h)
ODIR= build

default: mystation bus station-manager comptroller
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
mystation: $(ODIR)/mystation.o $(ODIR)/shm_functions.o $(ODIR)/bus_info.o
	@echo "Creating mystation..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
bus: $(ODIR)/bus.o $(ODIR)/shm_functions.o $(ODIR)/bus_info.o
	@echo "Creating bus..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
station-manager: $(ODIR)/station-manager.o $(ODIR)/shm_functions.o $(ODIR)/bus_info.o
	@echo "Creating station-manager..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
comptroller: $(ODIR)/comptroller.o $(ODIR)/shm_functions.o $(ODIR)/bus_info.o
	@echo "Creating comptroller..."
	$(CXX) -o $(BDIR)/$@ $^ $(LDLIBS) $(CXXFLAGS)
clean:
	@echo "Cleaning up..."
	$(RM) $(ODIR)/*
	$(RM) $(BDIR)/*

# source files.
SRC = deferred-shading.cpp
 
OBJ = $(SRC:.cpp=.o)
 
OUT = ../bin/deferred-shading
 
# include directories
INCLUDES = -I../../include
 
# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -Wall
 
# compiler
CCC = g++
 
# library paths
LIBS = -L../../third-party/assimp/lib  -L/usr/local/lib -L../../bin -ldodo -lpthread -lassimp -lGL -lX11 -lrt
 
# compile flags
LDFLAGS = -g
 
.SUFFIXES: .cpp
 
default: all

debug: CCFLAGS = -DDEBUG -g -Wall
debug: all

all : $(OUT)
 
.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
 
$(OUT): $(OBJ)
	$(CCC) $(OBJ) -o $(OUT) $(LIBS)
 
clean:
	rm -f $(OBJ) $(OUT)



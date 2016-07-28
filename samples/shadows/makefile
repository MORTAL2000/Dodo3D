# source files.
SRC = shadows.cpp
 
OBJ = $(SRC:.cpp=.o)
 
OUT = ../bin/shadows
 
# include directories
INCLUDES = -I../../dodo/include
 
# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -O2 -Wall
 
# compiler
CCC = g++
 
# library paths
LIBS = -L../../dodo/third-party/assimp/lib  -L/usr/local/lib -L../../dodo/bin -ldodo -lpthread -lassimp -lGL -lX11 -lrt
 
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



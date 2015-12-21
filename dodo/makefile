
# source files.
SRC = $(wildcard src/*.cpp)
OBJ = $(addprefix bin/,$(notdir $(SRC:.cpp=.o)))
OUT = bin/libdodo.a
 
# include directories
INCLUDES = -I./include -I./third-party/assimp/include
 
# C++ compiler flags
CCFLAGS = -g -O2 -Wall
 
# compiler
CCC = g++
 
# library paths
 
.SUFFIXES: .cpp
 
default: all

debug: CCFLAGS = -DDEBUG -g -Wall
debug: all

bin/%.o: src/%.cpp
	$(CCC) $(INCLUDES) $(CCFLAGS) -c -o $@ $<
 
$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

all: $(OUT)

clean:
	rm -f $(OBJ) $(OUT)



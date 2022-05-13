#c++ compile instructions

SRC=./src
EXE_FILE=TrueChain
OUT=./out
OUT_OBJ=./out/obj
OUT_EXE=./out/exe
LIBS=
EXE_FLAGS= -g
CXX = g++
CXXFLAGS = -g
RM = rm -rf

SRC_FILES=$(wildcard $(SRC)/*.cpp)
OBJ_FILES=$(patsubst $(SRC)/%.cpp,$(OUT_OBJ)/%.o,$(SRC_FILES))


all: base clean default test

base:
	mkdir $(OUT) -p
	mkdir $(OUT_EXE) -p
	mkdir $(OUT_OBJ) -p

default: compile_executable

release: CXXFLAGS += -O2
release: EXE_FLAGS += -O2
release: compile_executable

compile_objects: $(OBJ_FILES)

compile_executable: compile_objects
	$(CXX) $(EXE_FLAGS) $(OBJ_FILES) -o $(OUT_EXE)/$(EXE_FILE)
$(OUT_OBJ)/%.o:$(SRC)/%.cpp
	$(CXX) $(LIBS) $(CXXFLAGS) -c $< -o $@
test: compile_executable
	$(OUT_EXE)/$(EXE_FILE)
test_release: release compile_executable
	$(OUT_EXE)/$(EXE_FILE)
clean:
	$(RM) $(OUT_OBJ)/*
	$(RM) $(OUT_EXE)/*

#You must call 'make clean' between calls to 'make test' to 'make test_release'
#OR 'make default' to 'make release'


CXXFLAGS += -MMD 
-include $(OBJ_FILES:.o=.d)


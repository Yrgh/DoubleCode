SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

TARGET := $(BIN_DIR)/main

SRCS := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

FLAGS1 := -MMD -Iinclude -Isrc -Llib
FLAGS2 := -lSDL3

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	g++ $(FLAGS1) -o $@ $^ $(FLAGS2)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	g++ $(FLAGS1) -c $< -o $@ $(FLAGS2)

clean:
	del /f /q $(subst /,\,$(OBJS))
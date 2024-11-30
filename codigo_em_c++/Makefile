CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -L/opt/homebrew/lib -lsfml-graphics -lsfml-window -lsfml-system
INCLUDE = -Iinclude -I/opt/homebrew/include

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRC = $(wildcard $(SRC_DIR)/*.cc)
OBJ = $(patsubst $(SRC_DIR)/%.cc, $(OBJ_DIR)/%.o, $(SRC))
EXEC = $(BIN_DIR)/simplefs

all: $(BIN_DIR) $(OBJ_DIR) $(EXEC)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(EXEC): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -f $(EXEC)

dist-clean: clean
	rm -rf $(OBJ_DIR) $(BIN_DIR)

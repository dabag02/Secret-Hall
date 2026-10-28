# Build incrementale di La Sala Segreta (passo M2). Uso:  make [-jN] [BUILD=cartella]   |   make clean
# Equivalente a CMakeLists.txt; serve solo g++, make e sdl2-config (vedi INSTALL.md).
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
BUILD    ?= build

SRC := sala_segreta.cpp $(shell find src -name '*.cpp' | sort)
OBJ := $(SRC:%.cpp=$(BUILD)/obj/%.o)
DEP := $(OBJ:.o=.d)

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs) -lSDL2_ttf -lGL

$(BUILD)/sala_segreta: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(SDL_LIBS)

$(BUILD)/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Isrc $(SDL_CFLAGS) -MMD -MP -c $< -o $@

-include $(DEP)

clean:
	rm -rf $(BUILD)

# Test headless (nessun SDL/GL): `make test` (passo M4a; altri test si aggiungeranno in M12)
TEST_OBJ := $(BUILD)/obj/src/game/defs.o $(BUILD)/obj/src/items.o $(BUILD)/obj/src/state.o
$(BUILD)/test_defs: tests/unit/test_defs.cpp $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -Isrc $< $(TEST_OBJ) -o $@

test: $(BUILD)/test_defs
	$(BUILD)/test_defs

.PHONY: clean test

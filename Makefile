# LightEngine: build for macOS (system GLUT) and Linux (freeglut).
#
#   make            build build/LightEngine
#   make run        build and run from the repo root (data/map.txt is loaded relative to cwd)
#   make clean      remove build/
#
# Override as usual: make CXX=clang++ CXXFLAGS="-O0 -g"

TARGET := build/LightEngine
SRCS   := $(wildcard src/*.cpp src/le/*.cpp)
OBJS   := $(SRCS:src/%.cpp=build/obj/%.o)
DEPS   := $(OBJS:.o=.d)

CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++98
CPPFLAGS += -MMD -MP

ifeq ($(shell uname -s),Darwin)
  LDLIBS += -framework GLUT -framework OpenGL
else
  LDLIBS += -lglut -lGL -lm
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

-include $(DEPS)

# 2D Light Engine: build for macOS and Linux.
#
#   make            build build/LightEngine
#   make run        build and run from the repo root (data/ is loaded relative to cwd)
#   make clean      remove build/
#
# Requires GLFW 3 (found via pkg-config) and OpenGL 2.1. Dear ImGui is vendored in third_party/.
# Override as usual: make CXX=clang++ CXXFLAGS="-O0 -g"

TARGET := build/LightEngine
IMGUI  := third_party/imgui

SRCS := $(wildcard src/*.cpp src/*/*.cpp) \
        $(IMGUI)/imgui.cpp $(IMGUI)/imgui_draw.cpp $(IMGUI)/imgui_tables.cpp $(IMGUI)/imgui_widgets.cpp \
        $(IMGUI)/backends/imgui_impl_glfw.cpp $(IMGUI)/backends/imgui_impl_opengl2.cpp
OBJS := $(SRCS:%.cpp=build/obj/%.o)
DEPS := $(OBJS:.o=.d)

CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17
CPPFLAGS += -Isrc -I$(IMGUI) -I$(IMGUI)/backends -MMD -MP

ifneq ($(MAKECMDGOALS),clean)
  GLFW_CFLAGS := $(shell pkg-config --cflags glfw3 2>/dev/null)
  GLFW_LIBS   := $(shell pkg-config --libs glfw3 2>/dev/null)
  ifeq ($(GLFW_LIBS),)
    $(error GLFW 3 not found via pkg-config. macOS: brew install glfw pkg-config; Debian/Ubuntu: sudo apt install libglfw3-dev pkg-config)
  endif
endif
CPPFLAGS += $(GLFW_CFLAGS)
LDLIBS   += $(GLFW_LIBS)

ifeq ($(shell uname -s),Darwin)
  CPPFLAGS += -DGL_SILENCE_DEPRECATION
  LDLIBS   += -framework OpenGL
else
  LDLIBS   += -lGL -lm
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

-include $(DEPS)

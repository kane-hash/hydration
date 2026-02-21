CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -DGL_SILENCE_DEPRECATION
INCLUDES = -I/opt/homebrew/include -I.
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

TARGET = hydration
SRCDIR = src
SHADERDIR = shaders

SOURCES = main.cpp $(SRCDIR)/Shader.cpp $(SRCDIR)/Simulation.cpp $(SRCDIR)/Renderer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

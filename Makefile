CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I/usr/include/SDL2
LDFLAGS  = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

TARGET = pong

all: $(TARGET)

$(TARGET): pong.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean

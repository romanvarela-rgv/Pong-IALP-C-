CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I/usr/include/SDL2
LDFLAGS  = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

SRCS = src/main.cpp src/game.cpp src/render.cpp src/audio.cpp src/csv_manager.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = pong

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

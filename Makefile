CSRC = $(wildcard termbox/*.c) $(wildcard UTF8/*.c) $(wildcard sqlite/*.c)
CXXSRC = $(wildcard *.cpp)
COBJ = $(CSRC:.c=.o)
CXXOBJ = $(CXXSRC:.cpp=.o)
TARGET = juke

# CC = clang
# CXX = clang++
# LD = clang++

CC = gcc
CXX = g++
LD = g++

COMMONFLAGS = -c -W -Wall -Wextra -pedantic -Wno-unused-parameter -O3 \
			  -D_XOPEN_SOURCE=700
CFLAGS = $(COMMONFLAGS) -std=c11
CXXFLAGS = $(COMMONFLAGS) -std=c++20
LDFLAGS = -lpthread -ldl -lvlc

.PHONY: all clean cleanall run runv

all: debug

debug: CFLAGS += -g3 -fno-omit-frame-pointer -DDEBUG
debug: CXXFLAGS += -g3 -fno-omit-frame-pointer -DDEBUG
debug: LDFLAGS += -rdynamic
debug: $(TARGET)

ndebug: CFLAGS += -g0 -DNDEBUG
ndebug: CXXFLAGS += -g0 -DNDEBUG
ndebug: LDFLAGS += -s
ndebug: $(TARGET)

$(TARGET): $(COBJ) $(CXXOBJ)
	$(LD) $(COBJ) $(CXXOBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(CXXOBJ) $(TARGET)

cleanall:
	rm -f $(COBJ) $(CXXOBJ) $(TARGET)

run: $(TARGET)
	@./$(TARGET)

runv: $(TARGET)
	valgrind ./$(TARGET)

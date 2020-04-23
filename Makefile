CXX=g++
#CXX=clang++

CXXFLAGS=-O3
#CXXFLAGS=-g3 -fsanitize=address -fsanitize=leak -fsanitize=undefined
CXXFLAGS+=-Wall -Wextra
CPPFLAGS=-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

OBJECTS=\
	src/ReadCharsets.o\
	src/ReadMasks.o\
	src/main.o

DEPS=$(OBJECTS:.o=.d)
-include $(DEPS)

all: maskgen

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MF $(@:.o=.d) $(TARGET_ARCH) -c -o $@ $<

maskgen: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) maskgen

.PHONY: all clean

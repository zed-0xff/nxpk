CXX=g++
CXXFLAGS=-std=c++11 -O2
LD=$(CXX)
LDFLAGS=
TARGET=keytool

SRCS=$(TARGET).cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $< -o $@

# Clean target for removing generated files
clean:
	rm -f $(SRCS) $(TARGET)

# Phony targets
.PHONY: all clean


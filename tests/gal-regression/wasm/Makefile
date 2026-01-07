# Makefile for GAL WebGL Test (WASM)
#
# Uses direct em++ calls like tests/apps/Makefile.wasm
# Avoids emcmake/cmake which require Python 3.10+
#
# Usage:
#   make                    # Build
#   make clean              # Clean build artifacts
#   make DEBUG=1            # Debug build with source maps

CXX = em++

# Output directory
OUTPUT_DIR = ../../apps/gal-webgl

# Debug or Release build
ifdef DEBUG
    CXXFLAGS = -g -O0
    DEBUG_LDFLAGS = -g -gsource-map
else
    CXXFLAGS = -O2
    DEBUG_LDFLAGS =
endif

# Emscripten flags for WebGL 2.0
EM_FLAGS = -sUSE_WEBGL2=1 \
           -sFULL_ES3=1 \
           -sALLOW_MEMORY_GROWTH=1 \
           -sEXPORTED_FUNCTIONS=['_main','_runScenario','_getTotalScenarios','_getCurrentScenario','_getCanvasWidth','_getCanvasHeight'] \
           -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
           -sMODULARIZE=1 \
           -sEXPORT_NAME='createGALTest' \
           -sENVIRONMENT=web

LDFLAGS = $(DEBUG_LDFLAGS) $(EM_FLAGS)

# Source files
SRCS = gal_webgl_test.cpp
OBJS = $(SRCS:.cpp=.o)

# Target
TARGET = $(OUTPUT_DIR)/gal_webgl_test.js

all: $(OUTPUT_DIR) $(TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	cp gal_webgl_test.html $(OUTPUT_DIR)/

clean:
	rm -f $(OBJS)
	rm -f $(OUTPUT_DIR)/gal_webgl_test.js
	rm -f $(OUTPUT_DIR)/gal_webgl_test.wasm
	rm -f $(OUTPUT_DIR)/gal_webgl_test.html

.PHONY: all clean

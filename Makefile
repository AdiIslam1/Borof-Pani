# Project
TARGET = borofpani
SRC_DIR = src
ASSETS = assets

# Collect all .cpp files in src/
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Compiler (forcing C mode even for .cpp)
CC = gcc
CFLAGS = -Wall -std=c99 -x c

# Detect platform
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Linux)
    LIBS = -lraylib -lm -lpthread -ldl -lrt -lX11
    COPY = cp -r
endif

ifeq ($(UNAME_S), Darwin) # macOS
    LIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
    COPY = cp -r
endif

ifeq ($(OS), Windows_NT) # Windows (MSYS2/MinGW)
    LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
    TARGET := $(TARGET).exe
    COPY = cp -r
endif

# Rules
all: $(TARGET) copy-assets

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

copy-assets:
	@if [ -d $(ASSETS) ]; then \
		$(COPY) $(ASSETS) ./; \
		echo "Assets copied."; \
	fi

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o


CC := gcc
CFLAGS := -ggdb3 -Wall -Wpedantic -Wextra -Wdeclaration-after-statement -Iinclude
LDFLAGS := -lm

# add SDL2
CFLAGS += $(shell pkg-config --cflags sdl2)
LDFLAGS += $(shell pkg-config --libs sdl2)
# add zlib
CFLAGS += $(shell pkg-config --cflags zlib)
LDFLAGS += $(shell pkg-config --libs zlib)

SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := build
TARGET := $(BUILD_DIR)/b173c

CFLAGS += -I$(SRC_DIR)

SRC_FILES := $(shell find $(SRC_DIR)/ -type f -name "*.c")
HDR_FILES := $(shell find $(SRC_DIR)/ -type f -name "*.h")
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

PY := python
SHADER_PREP := prepare_shader_sources.py
SHADER_SOURCES := $(OBJ_DIR)/gl_shader_sources.c
SHADER_SOURCES_OBJ := $(OBJ_DIR)/gl_shader_sources.o
SHADER_FILES := $(wildcard $(SRC_DIR)/vid/shaders/*.glsl)

OBJ_FILES += $(SHADER_SOURCES_OBJ)

all: $(OBJ_DIR) $(BUILD_DIR) $(TARGET)

clean:
	rm $(OBJ_DIR) -fr
	rm $(TARGET) -fr

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)


# dont care about warnings from ext
$(OBJ_DIR)/ext/%.o: $(SRC_DIR)/ext/%.c $(HDR_FILES)
	./mkdirs.sh $@
	$(CC) -c -Iinclude -o $@ $<

# project sources (c)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDR_FILES)
	./mkdirs.sh $@
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $<

# project sources (glsl)
$(SHADER_SOURCES): $(SHADER_PREP) $(SHADER_FILES)
	$(PY) $(SHADER_PREP) $(SHADER_SOURCES) $(SHADER_FILES)

$(SHADER_SOURCES_OBJ): $(SHADER_SOURCES)
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $<

# final binary
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

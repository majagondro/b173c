CC := gcc
CFLAGS := -g -Wall -Wpedantic -Wextra -Wdeclaration-after-statement
LDFLAGS :=

CFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs)

SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := build
TARGET := $(BUILD_DIR)/b173c

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
HDR_FILES := $(wildcard $(SRC_DIR)/*.h)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

all: $(OBJ_DIR) $(BUILD_DIR) $(TARGET)

clean:
	rm $(OBJ_DIR) -fr
	rm $(BUILD_DIR) -fr

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDR_FILES)
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $<

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

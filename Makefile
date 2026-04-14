CC = gcc
CFLAGS = -Wall -Wextra -O3 -arch arm64 -Iinclude
LDFLAGS = -dynamiclib -arch arm64

SRC_DIR = core
BUILD_DIR = build
GEN_DIR = gen
INCLUDE_DIR = include

SOURCES := $(shell find core -name '*.c')
OBJ_FILES = $(patsubst core/%.c, build/%.o, $(SOURCES))

TARGET = $(BUILD_DIR)/libnes_core.dylib

all: clean $(TARGET)

test:
	DYLD_LIBRARY_PATH=/opt/homebrew/lib python3 -m src.tests.test_smb

generate:
	@echo "Generating CPU instructions..."
	python3 $(GEN_DIR)/gen_cpu_instructions.py

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -g -c $< -o $@

$(TARGET): $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)/*
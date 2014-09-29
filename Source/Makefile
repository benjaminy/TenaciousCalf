BUILD_DIR = ../Build

all: $(BUILD_DIR)/foo $(BUILD_DIR)/hash

$(BUILD_DIR)/foo: array.c
	gcc -Wall -o $(BUILD_DIR)/foo -g array.c

$(BUILD_DIR)/hash: hpy_hash_map.c
	gcc -Wall -o $(BUILD_DIR)/hash -g hpy_hash_map.c

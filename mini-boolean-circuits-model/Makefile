# Makefile -- Boolean Circuit Model
# Builds the mini-boolean-circuits-model library and test executable.
# C99 standard, libc + libm.

CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -O2 -g
LDFLAGS  = -lm
SRC_DIR  = src
INC_DIR  = include
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(SRC_DIR)/circuit.c \
       $(SRC_DIR)/circuit_analysis.c \
       $(SRC_DIR)/circuit_builder.c \
       $(SRC_DIR)/circuit_complexity.c \
       $(SRC_DIR)/circuit_synthesis.c \
       $(SRC_DIR)/circuit_app.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_SRC = $(TEST_DIR)/test_circuit.c
TEST_EXE = test_circuit

all: $(TEST_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(TEST_EXE): $(OBJS) $(TEST_SRC)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(TEST_SRC) $(OBJS) $(LDFLAGS) -o $@

test: $(TEST_EXE)
	./$(TEST_EXE)

clean:
	rm -rf $(BUILD_DIR) $(TEST_EXE) *.o *.exe

distclean: clean
	rm -f *~

.PHONY: all test clean distclean

# Makefile -- ACC0 Circuit Complexity
# Builds mini-acc0-circuits library and test executable.
# C99 standard, libc + libm.

CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -O2 -g
LDFLAGS  = -lm
SRC_DIR  = src
INC_DIR  = include
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(SRC_DIR)/acc0.c \
       $(SRC_DIR)/acc0_builders.c \
       $(SRC_DIR)/acc0_gates.c \
       $(SRC_DIR)/acc0_lower_bounds.c \
       $(SRC_DIR)/acc0_polynomial.c \
       $(SRC_DIR)/acc0_sat.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TEST_SRC = $(TEST_DIR)/test_all.c
TEST_EXE = test_acc0

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

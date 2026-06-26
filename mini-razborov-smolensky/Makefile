# mini-razborov-smolensky Makefile
# Razborov-Smolensky polynomial method for AC0[p] circuit lower bounds
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -I./include -std=c99
LDFLAGS = -lm

SRC  = src/gf_poly.c src/rs_core.c src/rs_approx.c src/rs_bounds.c
OBJ  = $(SRC:.c=.o)
DEMO = src/razborov.c
TEST = tests/test_all.c
EXAM = examples/demo.c

.PHONY: all clean test demo check

all: test demo

# Compile library objects
src/%.o: src/%.c include/*.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Build and run test suite
test: $(OBJ)
	$(CC) $(CFLAGS) -o build/test_all $(TEST) $(OBJ) $(LDFLAGS)
	./build/test_all

# Build demo program
demo: $(OBJ)
	$(CC) $(CFLAGS) -o build/demo $(DEMO) $(OBJ) $(LDFLAGS)
	@echo "Run: ./build/demo"

# Build example program
example: $(OBJ)
	$(CC) $(CFLAGS) -o build/example $(EXAM) $(OBJ) $(LDFLAGS)
	@echo "Run: ./build/example"

# Check that code compiles (no linking)
check:
	$(CC) $(CFLAGS) -c src/gf_poly.c -o /dev/null
	$(CC) $(CFLAGS) -c src/rs_core.c -o /dev/null
	$(CC) $(CFLAGS) -c src/rs_approx.c -o /dev/null
	$(CC) $(CFLAGS) -c src/rs_bounds.c -o /dev/null
	@echo "All source files compile OK."

# Clean build artifacts
clean:
	rm -f src/*.o
	rm -rf build/

# Create build directory
build:
	mkdir -p build

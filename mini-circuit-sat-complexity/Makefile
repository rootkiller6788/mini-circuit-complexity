# Makefile for mini-circuit-sat-complexity
# C99, libc + libm. Depends on mini-boolean-circuits-model.
#
# Circuit-SAT Complexity: SAT algorithms, CDCL solver, circuit lower bounds,
# MCSP, Williams' algorithmic method, CNF tools, benchmark generation.

CC       = gcc
CFLAGS   = -std=c99 -Wall -Wextra -O2 -I./include -I../mini-boolean-circuits-model/include
LDFLAGS  = -lm

SRCDIR   = src
TSTDIR   = tests
EXMDIR   = examples
MODELDIR = ../mini-boolean-circuits-model/src

SRCS     = $(wildcard $(SRCDIR)/*.c)
MODELSRCS = $(filter-out %/circuit_app.c %/_test.c, $(wildcard $(MODELDIR)/*.c))
TESTS    = $(wildcard $(TSTDIR)/*.c)
EXAMPLES = $(wildcard $(EXMDIR)/*.c)

.PHONY: all clean test examples check

all: check

# Compile only (no link) — verifies all sources are valid C
check:
	$(CC) $(CFLAGS) -c $(SRCS)
	@echo "All source files compiled successfully."

# Full test: compile model + this module, link, and run
test: $(SRCS) $(MODELSRCS) $(TESTS)
	@for t in $(TESTS); do \
		echo "=== Running $$t ==="; \
		$(CC) $(CFLAGS) $$t $(SRCS) $(MODELSRCS) $(LDFLAGS) -o _test_runner; \
		./_test_runner; \
		rm -f _test_runner; \
	done

# Examples
examples: $(SRCS) $(MODELSRCS) $(EXAMPLES)
	@for e in $(EXAMPLES); do \
		echo "=== Running $$e ==="; \
		$(CC) $(CFLAGS) $$e $(SRCS) $(MODELSRCS) $(LDFLAGS) -o _example_runner; \
		./_example_runner; \
		rm -f _example_runner; \
	done

clean:
	rm -f $(SRCDIR)/*.o *.o *.exe _test_runner _example_runner

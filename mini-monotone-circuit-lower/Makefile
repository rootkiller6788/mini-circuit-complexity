CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include -std=c11
LDFLAGS = -lm
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/test_all.c
TEST_BIN = test_all
EXAMPLES = razborov_demo sunflower_lemma_demo clique_lowerbound_explore

.PHONY: all test clean examples

all: $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJ) $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) $(OBJ) -o $(TEST_BIN) $(LDFLAGS)
	./$(TEST_BIN)

examples: $(OBJ)
	$(CC) $(CFLAGS) examples/razborov_demo.c $(OBJ) -o razborov_demo $(LDFLAGS)
	$(CC) $(CFLAGS) examples/sunflower_lemma_demo.c $(OBJ) -o sunflower_lemma_demo $(LDFLAGS)
	$(CC) $(CFLAGS) examples/clique_lowerbound_explore.c $(OBJ) -o clique_lowerbound_explore $(LDFLAGS)

razborov_demo: $(OBJ)
	$(CC) $(CFLAGS) examples/razborov_demo.c $(OBJ) -o razborov_demo $(LDFLAGS)
	./razborov_demo

sunflower_demo: $(OBJ)
	$(CC) $(CFLAGS) examples/sunflower_lemma_demo.c $(OBJ) -o sunflower_lemma_demo $(LDFLAGS)
	./sunflower_lemma_demo

clique_demo: $(OBJ)
	$(CC) $(CFLAGS) examples/clique_lowerbound_explore.c $(OBJ) -o clique_lowerbound_explore $(LDFLAGS)
	./clique_lowerbound_explore

monotone_demo: $(OBJ)
	$(CC) $(CFLAGS) src/monotone.c $(OBJ) -o monotone_demo $(LDFLAGS)
	./monotone_demo

clean:
	rm -f src/*.o $(TEST_BIN) $(EXAMPLES) monotone_demo
	rm -f *.exe

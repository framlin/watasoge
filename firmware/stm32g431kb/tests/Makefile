CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Werror -O0 -g \
         -I../Core/Inc -I../fracanto/include -I../fracanto/drivers/pcm5102a
LDFLAGS = -lm

TESTS = test_adsr test_watasoge_module

.PHONY: all clean test

all: $(TESTS)

test: $(TESTS)
	@for t in $(TESTS); do echo "--- Running $$t ---"; ./$$t || exit 1; done

test_adsr: test_adsr.c ../Core/Src/synthesis.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_watasoge_module: test_watasoge_module.c ../Core/Src/watasoge_module.c ../Core/Src/synthesis.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TESTS)

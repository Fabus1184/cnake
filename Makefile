CC := gcc
LIBS := -lm -lncursesw
ARGS := -Wall -Wextra

build: main

run: main
	./main

main: main.c
	$(CC) $? $(LIBS) $(ARGS) -o $@

clean:
	rm main

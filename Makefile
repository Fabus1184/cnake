CC := gcc
LIBS := -lm -lncursesw

build: main

run: main
	./main

main: main.c
	$(CC) $(LIBS) $? -g -O2 -o $@

clean:
	rm main

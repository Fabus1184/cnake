CC := gcc
LIBS := -lm -lncursesw

main: main.c
	$(CC) $(LIBS) $? -o $@

clean:
	rm main

#include <locale.h>
#include <math.h>
#include <memory.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#define FIELD_COLS 6
#define FIELD_ROWS 14
#define FIELD_SIZE FIELD_COLS * FIELD_ROWS

typedef enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
} Direction;

typedef struct Position {
    int x;
    int y;
} Position;

typedef struct Snake {
    Position* path;
    int size;
    Direction direction;
    int stock;
} Snake;

typedef struct GameState {
    Snake snake;
    int width, height;
    Position apple;
} GameState;

static void drawState(GameState* state)
{
    // print head
    switch (state->snake.direction) {
    case UP:
        mvprintw(state->snake.path[0].y, state->snake.path[0].x, "🔼");
        break;
    case DOWN:
        mvprintw(state->snake.path[0].y, state->snake.path[0].x, "🔽");
        break;
    case LEFT:
        mvprintw(state->snake.path[0].y, state->snake.path[0].x, "◀️ ");
        break;
    case RIGHT:
        mvprintw(state->snake.path[0].y, state->snake.path[0].x, "▶️ ");
        break;
    }

    // print body
    for (int i = 1; i < state->snake.size; i++) {
        mvprintw(state->snake.path[i].y, state->snake.path[i].x, "🟩");
    }

    // print apple
    mvprintw(state->apple.y, state->apple.x, "🍎");
}

static bool snakeContains(Snake* snake, Position p)
{
    for (int i = 0; i < snake->size; i++) {
        if ((snake->path[i].x == p.x
                || snake->path[i].x + 1 == p.x
                || snake->path[i].x - 1 == p.x)
            && snake->path[i].y == p.y)
            return true;
    }
    return false;
}

static void genApple(GameState* state)
{
    state->apple = (Position) { rand() % state->width, rand() % (state->height + 1) };
    while (snakeContains(&state->snake, state->apple)) {
        state->apple = (Position) { rand() % state->width, rand() % (state->height + 1) };
    }
}

static void updateState(GameState* state)
{
    if (state->snake.stock > 0) {
        Position* new = malloc((state->snake.size + 1) * sizeof(Position));
        memcpy(new, state->snake.path, (state->snake.size) * sizeof(Position));
        free(state->snake.path);
        state->snake.path = new;
        state->snake.size++;
        state->snake.stock--;
    }

    memmove(state->snake.path + 1, state->snake.path, (state->snake.size - 1) * sizeof(Position));

    switch (state->snake.direction) {
    case UP:
        state->snake.path[0].y--;
        break;
    case DOWN:
        state->snake.path[0].y++;
        break;
    case LEFT:
        state->snake.path[0].x -= 2;
        break;
    case RIGHT:
        state->snake.path[0].x += 2;
        break;
    }

    if (state->snake.path[0].x < 0) {
        state->snake.path[0].x = state->width;
    }

    if (state->snake.path[0].y < 0) {
        state->snake.path[0].y = state->height;
    }

    if (state->snake.path[0].x > state->width) {
        state->snake.path[0].x = 0;
    }

    if (state->snake.path[0].y > state->height) {
        state->snake.path[0].y = 0;
    }

    Position head = state->snake.path[0];
    state->snake.path++;
    state->snake.size--;
    if (snakeContains(&state->snake, head)) {
        endwin();
        usleep(10000);
        printf("YOU LOSE!\n");
        exit(EXIT_SUCCESS);
    }
    state->snake.path--;
    state->snake.size++;

    if (snakeContains(&state->snake, state->apple)) {
        genApple(state);
        state->snake.stock++;
    }
}

static long micros()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * (int)1e6 + t.tv_usec;
}

int main()
{
    // initialize rng
    srand((unsigned int)micros());

    // curses init
    setlocale(LC_ALL, "");
    atexit((void (*)(void))endwin);
    initscr();
    if (nodelay(stdscr, true) == ERR) {
        endwin();
        printf("Error unblocking getch\n");
        exit(EXIT_FAILURE);
    }
    noecho();
    curs_set(false);

    // get dimensions
    int x, y;
    getmaxyx(stdscr, y, x);
    x -= 2;
    y--;

    // init game state
    bool field[FIELD_SIZE];
    memset(field, (int)true, FIELD_SIZE);

    Position snakePath[5];
    for (int i = 0; i < 5; i++) {
        snakePath[i] = (Position) { x / 2 + (2 * i), y / 2 };
    }
    Snake snake = { snakePath, 5, LEFT, 0 };
    GameState state = { snake, x, y, { 0, 0 } };
    genApple(&state);

    long time = micros();

    // game loop
    while (true) {
        int c = getch();
        if (c != ERR) {
            switch (c) {
            case 'w':
                state.snake.direction = UP;
                break;
            case 'a':
                state.snake.direction = LEFT;
                break;
            case 's':
                state.snake.direction = DOWN;
                break;
            case 'd':
                state.snake.direction = RIGHT;
                break;
            case 'q':
                exit(EXIT_SUCCESS);
            default:
                break;
            }
        }

        if (micros() - time >= 100 * 1000) {
            updateState(&state);
            clear();
            drawState(&state);
            refresh();

            time = micros();
        }
    }
}

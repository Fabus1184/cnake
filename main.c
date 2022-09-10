#include <locale.h>
#include <memory.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define FIELD_COLS 6
#define FIELD_ROWS 14
#define FIELD_SIZE FIELD_COLS * FIELD_ROWS
#define SNAKE_INITIAL_SIZE 5

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
    Position *path;
    int size;
    Direction direction;
    int stock;
} Snake;

typedef struct GameState {
    Snake snake;
    int width, height;
    Position apple;
} GameState;

void exitCnake(char *format, int exitCode, ...) {
    endwin();
    while (!isendwin());
    usleep(250 * 1000);
    if (format) {
        va_list ap;
        va_start(ap, exitCode);
        vprintf(format, ap);
    }
    exit(exitCode);
}

static void drawState(GameState *state) {
    // print score
    attron(A_BOLD);
    mvprintw(0, 0, "Score: %d", state->snake.size - SNAKE_INITIAL_SIZE);
    attroff(A_BOLD);

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

static bool snakeContains(Snake *snake, Position p) {
    // test if snake path contains position
    for (int i = 0; i < snake->size; i++) {
        if ((snake->path[i].x == p.x
             || snake->path[i].x + 1 == p.x
             || snake->path[i].x - 1 == p.x)
            && snake->path[i].y == p.y)
            return true;
    }
    return false;
}

static void genApple(GameState *state) {
    // generate random apple not inside the snake
    state->apple = (Position) {rand() % state->width, rand() % (state->height + 1)};
    while (snakeContains(&state->snake, state->apple)) {
        state->apple = (Position) {rand() % state->width, rand() % (state->height + 1)};
    }
}

static void updateState(GameState *state) {

    // extend snake
    if (state->snake.stock > 0) {
        // new path array
        Position *new = malloc((state->snake.size + 1) * sizeof(Position));
        // copy old path (last element still uninitialized)
        memcpy(new, state->snake.path, (state->snake.size) * sizeof(Position));
        free(state->snake.path);
        state->snake.path = new;
        state->snake.size++;
        state->snake.stock--;
    }

    // shift left, overwriting last
    memmove(state->snake.path + 1, state->snake.path, (state->snake.size - 1) * sizeof(Position));

    // move head
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

    // wrap around window borders
    {
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
    }

    // check if snake collides with itself
    {
        Position head = state->snake.path[0];
        state->snake.path++;
        state->snake.size--;

        if (snakeContains(&state->snake, head)) {
            exitCnake("YOU LOSE!\nScore: %d\n", EXIT_SUCCESS, ++state->snake.size - SNAKE_INITIAL_SIZE);
        }

        state->snake.path--;
        state->snake.size++;
    }

    // check if snake contains apple
    if (snakeContains(&state->snake, state->apple)) {
        genApple(state);
        state->snake.stock++;
    }
}

// get time in microseconds
static long micros() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * (int) 1e6 + t.tv_usec;
}

int main() {
    // initialize rng
    srand((unsigned int) micros());

    // curses init
    setlocale(LC_ALL, "");
    atexit((void (*)(void)) endwin);
    initscr();
    start_color();
    use_default_colors();
    noecho();
    curs_set(false);
    if (nodelay(stdscr, true) == ERR) {
        exitCnake("Error unblocking getch()\n", EXIT_FAILURE);
    }

    // get dimensions
    int x, y;
    getmaxyx(stdscr, y, x);
    x -= 2; // emojis are 2 wide characters
    y--;

    // init game state
    Position *snakePath = malloc(SNAKE_INITIAL_SIZE * sizeof (Position));
    for (int i = 0; i < SNAKE_INITIAL_SIZE; i++) {
        snakePath[i] = (Position) {x / 2 + (2 * i), y / 2};
    }
    Snake snake = {snakePath, SNAKE_INITIAL_SIZE, LEFT, 0};
    GameState state = {snake, x, y, {0, 0}};
    genApple(&state);

    // game loop
    long time = micros();
    while (!isendwin()) {
        // get input (non blocking)
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
                    exitCnake("YOU LOSE!\nScore: %d\n", EXIT_SUCCESS, state.snake.size - SNAKE_INITIAL_SIZE);
                default:
                    break;
            }
        }

        // update & redraw game every 100 ms
        if (micros() - time >= 100 * 1000) {
            updateState(&state);

            clear();
            drawState(&state);
            refresh();

            time = micros();
        }
    }
}

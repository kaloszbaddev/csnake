#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

typedef unsigned int U32;
typedef signed int I32;
typedef unsigned char U8;

#define COLS 15 
#define ROWS 15
#define START_SIZE 3

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void mmemset(void *dst, I32 c, U32 n) {
    U8 *d = dst;
    while (n--) *d++ = (U8)c; 
}

typedef enum Type {
    AIR = 0,
    SNAKE,
    APPLE,
} Type;

typedef enum Direction {
    DIR_RIGHT = 0,
    DIR_LEFT,
    DIR_UP,
    DIR_DOWN,
} Direction;

typedef struct vec2i {
    I32 x, y;
} vec2i;

typedef struct Snake {
    vec2i *body;
    I32 cnt, cap;
    Direction dir;
} Snake;

void snake_add(Snake *s, vec2i seg) {
    if (s->cnt >= s->cap) {
        if (s->cap == 0) 
            s->cap = START_SIZE;
        else 
            s->cap *= 2; 
        
        vec2i *tmp = realloc(s->body, s->cap * sizeof(* s->body)); 
        if (tmp == NULL) {
            perror("allocating error"); 
            exit(1);
        }
        s->body = tmp;
    }
        
    s->body[s->cnt++] = seg;
}

void snake_init(Snake *s) {
    int centerX = COLS / 2;
    int centerY = ROWS / 2;
    for (I32 i=START_SIZE-1; i>=0; --i) {
        vec2i vec = {centerX + i - START_SIZE, centerY};
        snake_add(s, vec);
    } 
}

void snake_reset(Snake *s) {
      s->cnt = 0;
      s->cap = 0;
      s->dir = 0;
      snake_init(s);
}

bool snake_in(Snake *s) {
    vec2i head = * s->body;
    for (I32 i=1;i<s->cnt;++i) {
        vec2i seg = s->body[i];
        if (head.x == seg.x &&
            head.y == seg.y) 
            return true; 
    }
    return false;
}

bool snake_inapple(Snake *s, vec2i apple) {
    for (I32 i=0;i<s->cnt;++i) {
        vec2i seg = s->body[i];
        if (seg.x == apple.x &&
            seg.y == apple.y)
            return true;
    }
    return false;
}

static Type win[ROWS][COLS] = {0};

void draw_win(void) {
    for (I32 i=0; i<ROWS; ++i) {
        for (I32 j=0; j<COLS; ++j) {
            switch (win[i][j]) {
                case AIR:   printf("'"); break; 
                case SNAKE: printf("#"); break;
                case APPLE: printf("O"); break; 
            }
        }
        printf("\n");
    }
}

I32 main(void) {
    enableRawMode();  
    srand(time(NULL));
   
    /* initialization */
    Snake snake = {0};
    vec2i apple = { rand() % COLS, rand() % ROWS };
    
    snake_init(&snake);
    
    while (1) {
        /* clear screen */
        printf("\e[1;1H\e[2J");

        /* input */
        I32 c = 0; /* 00 00 00 00 */ 
        read(STDIN_FILENO, &c, 1);
        if (c == 'q') break;        

        switch (c) {
            case 'w': if (snake.dir!=DIR_DOWN)    snake.dir = DIR_UP;    break;
            case 'a': if (snake.dir!=DIR_RIGHT)   snake.dir = DIR_LEFT;  break;
            case 's': if (snake.dir!=DIR_UP)      snake.dir = DIR_DOWN;  break;
            case 'd': if (snake.dir!=DIR_LEFT)    snake.dir = DIR_RIGHT; break;
        }
        
        /* CLEAR WINDOW */
        mmemset(win, 0, sizeof(win));
        
        /* SNAKE LOGIC */
    
        vec2i head = * snake.body;
        vec2i last = snake.body[snake.cnt-1];
        
        switch (snake.dir) {
            case DIR_UP:    head.y--; break;
            case DIR_LEFT:  head.x--; break;
            case DIR_DOWN:  head.y++; break;
            case DIR_RIGHT: head.x++; break;
        }
         
        for (I32 i=snake.cnt-1; i>0; --i) {
            snake.body[i] = snake.body[i-1];
        }
            
        
        *snake.body = head;
        
        if (c=='g') snake_add(&snake, last);
        else if (c=='r') snake_reset(&snake); 

        if (snake_in(&snake)) snake_reset(&snake); 
        if (head.x >= COLS || head.x < 0 || head.y >= ROWS || head.y < 0) snake_reset(&snake);
        /* SNAKE LOGIC */

        /* APPLE LOGIC */
        if (head.x == apple.x &&
            head.y == apple.y) {
            do {
                apple.x = rand() % COLS;
                apple.y = rand() % ROWS;
            }
            while (snake_inapple(&snake, apple));
            snake_add( &snake, last);
        }
        /* APPLE LOGIC */
        
        /* draw */
        for (I32 i=0; i<snake.cnt; ++i) {
            I32 x = snake.body[i].x;
            I32 y = snake.body[i].y;
            win[y][x] = SNAKE;
        }
        
        win[apple.y][apple.x] = APPLE;
        
        draw_win();                
        
        /* small delay */
        usleep(200 * 1000); 
    }   
    free(snake.body);
    return 0;
}


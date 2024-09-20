#include <tonc.h>
#include <stdlib.h>


#define CBB_0  0
#define SBB_0 28
#define CBB_1  1
#define SBB_1 31

enum __DIRECTION {Left, Down, Up, Right};

typedef enum __DIRECTION Direction;

struct __TAIL {
  int* xs;
  int* ys;
  int size;
  int ptr;
  int visible;
};

typedef struct __TAIL Tail;

struct __SNAKE {
  int directionX;
  int directionY;
  int posX;
  int posY;
  Tail tail;
};

typedef struct __SNAKE Snake;

struct __FOOD {
  int posX;
  int posY;
};

typedef struct __FOOD Food;

BG_POINT bg0_pt= { 0, 0 };
SCR_ENTRY *bg0_map= se_mem[SBB_0];

void init_map() {
  // initialize a background
  REG_BG0CNT= BG_CBB(CBB_0) | BG_SBB(SBB_0) | BG_REG_64x64 | 1;
  REG_BG0HOFS= 0;
  REG_BG0VOFS= 0;
  REG_BG1CNT= BG_CBB(CBB_1) | BG_SBB(SBB_1) | BG_REG_64x64 | 0; // Last part sets priority
  REG_BG1HOFS= 0;
  REG_BG1VOFS= 0;

  // create the tiles: basic tile and a cross
  const TILE tiles[3]=
    {
      {{0x11111111, 0x01111111, 0x01111111, 0x01111111,
	 0x01111111, 0x01111111, 0x01111111, 0x00000001}},
      {{0x00000000, 0x00100100, 0x01100110, 0x00011000,
	 0x00011000, 0x01100110, 0x00100100, 0x00000000}},
      {{0x00000000, 0x01111110, 0x01111110, 0x01111110,
	 0x01111110, 0x01111110, 0x01111110, 0x00000000}},
    };
  tile_mem[CBB_0][0]= tiles[0];
  tile_mem[CBB_0][1]= tiles[1];
  tile_mem[CBB_0][2]= tiles[2];

  // create a palette
  pal_bg_bank[0][1]= RGB15(31,  0,  0);
  pal_bg_bank[1][1]= RGB15(16, 16, 16);

  // map

  SCR_ENTRY *pse = bg0_map;
  for (int j=0; j<32*20; j++) {
    *pse++= SE_PALBANK(1) | 0;
  }

};

int se_by_position(int x, int y) {
  return x + 32 * y;
};

void init_tail(Tail* tail, Snake* snake, int size) {
  tail->xs = (int *) realloc(tail->xs, sizeof(int)*size); 
  tail->ys = (int *) realloc(tail->ys, sizeof(int)*size);

  for (int i=0;i<size;i++) {
    // Initially just exist at the head
    tail->xs[i]=snake->posX;
    tail->ys[i]=snake->posY;
  }
  
  tail->ptr = 0;
  tail->size = size;
  tail->visible = 0;
}

void append_tail(Tail* tail, int x, int y) {
  tail->xs[tail->ptr] = x;
  tail->ys[tail->ptr] = y;
  tail->ptr = (tail->ptr + 1) % tail->size;
};

void draw_tail(Tail* tail) {
  for (int i=1; i<tail->visible+1; i++) {
    int idx = (tail->ptr-i + tail->size) % tail->size;
    bg0_map[se_by_position(tail->xs[idx], tail->ys[idx])] = SE_PALBANK(0) | 2;
  };
};

void undraw_tail(Tail* tail) {
  for (int i=1; i<tail->visible+1; i++) {
    int idx = (tail->ptr-i + tail->size) % tail->size;
    bg0_map[se_by_position(tail->xs[idx], tail->ys[idx])] = SE_PALBANK(1) | 0;
  };
};

void grow_tail(Tail* tail) {
  if (tail->visible < tail->size) {
    tail->visible++;
  }
};

void destroy_tail(Tail* tail) {
  undraw_tail(tail);
  free(tail->xs);
  free(tail->ys);
};

void init_snake(Snake* snake) {
  snake->posX = 10;
  snake->posY = 5;
  snake->directionX = 1;
  snake->directionY = 0;
  init_tail(&snake->tail, snake, 40);
};

void undraw_snake(Snake* snake) {
  bg0_map[se_by_position(snake->posX, snake->posY)] = SE_PALBANK(1) | 0;
  undraw_tail(&snake->tail);
};

void draw_snake(Snake* snake) {
  bg0_map[se_by_position(snake->posX, snake->posY)] = SE_PALBANK(0) | 2;
  draw_tail(&snake->tail);
};

int change_snake_direction(Snake* snake, Direction dir) {
      if (dir == Right) {
	snake->directionX = 1;
	snake->directionY = 0;
      }

      else if (dir == Left) {
	snake->directionX = -1;
	snake->directionY = 0;
      }

      else if (dir == Up) {
	snake->directionX = 0;
	snake->directionY = -1;
      }

      else if (dir == Down) {
	snake->directionX = 0;
	snake->directionY = 1;
      }

      else {
	return 1;
      };

      return 0;
}

void destroy_snake(Snake* snake) {
  undraw_snake(snake);
  destroy_tail(&snake->tail);
};

int snake_collides_self(Snake* snake) {
  Tail tail = snake->tail;
  for (int i=1;i<tail.visible+1;i++) {
    int idx = (tail.ptr - i + tail.size) % tail.size;
    if ((tail.xs[idx] == snake->posX) && (tail.ys[idx] == snake->posY)) {
      return 1;
    }
  }
  return 0;
}

int food_overlaps_tail(Food* food, Tail* tail) {
  for (int i=1;i<tail->visible+1;i++) {
    int idx = (tail->ptr - i + tail->size) % tail->size;
    if ((tail->xs[idx] == food->posX) && (tail->xs[idx] == food->posX )) {
      return 1;
    }
  }
  return 0;
}

void init_food(Food* food, Snake* snake) {
  food->posX = rand() % 30;
  food->posY = rand() % 20;

  // If food will overlap with the snake, try again
  if ((food->posX == snake->posX) && (food->posY == snake->posY)) {
    init_food(food, snake);
  };

  if (food_overlaps_tail(food, &snake->tail)) {
    init_food(food, snake);
  };
};

void undraw_food(Food* food) {
  bg0_map[se_by_position(food->posX, food->posY)] = SE_PALBANK(1) | 0;

};

void draw_food(Food* food) {
  bg0_map[se_by_position(food->posX, food->posY)] = SE_PALBANK(0) | 1;
};


int play_game(Snake* snake, Food* food, int highScore) {
  int score = 0;
  char scoreBuffer[20]; // Buffer to hold the formatted string
  sprintf(scoreBuffer, "Score: %d", score);
      
  char highScoreBuffer[30]; // Buffer to hold the formatted string
  sprintf(highScoreBuffer, "High Score: %d", highScore);
  
  float speed = 1.0;
  Direction direction = Right;

  draw_food(food);
  draw_snake(snake);
  
  float t = 1;
  
  while (1) {
    vid_vsync();
    key_poll();
    
    tte_write("#{P:0,0}");        // Goto (72, 64).
    tte_write(scoreBuffer);      // Print "Hello world!"
    tte_write("#{P:0,8}");        // Goto (72, 64).
    tte_write(highScoreBuffer);      // Print "Hello world!"

    if (score > highScore) {
      pal_bg_bank[0][1]= RGB15(0,  31,  0);
    };


    // Change snake direction
    if (key_is_down(KEY_DOWN)) {
      if (snake->directionY != -1) {
	direction = Down;
      };
    }

    else if (key_is_down(KEY_RIGHT)) {
      if (snake->directionX != -1) {
	direction = Right;
      };
    }

    else if (key_is_down(KEY_LEFT)) {
      if (snake->directionX != 1) {
	direction = Left;
      }
    }

    else if (key_is_down(KEY_UP)) {
      if (snake->directionY != 1) {
	direction = Up;
      }
    };

    if(t > 14.0 ) {
      
      change_snake_direction(snake, direction);
      
      if (snake_collides_self(snake)) {
	return score;
      };

      // Out of Bounds
      if ((snake->posX >= 30) | (snake->posX < 0) | (snake->posY >= 20) | (snake->posY < 0)) {
	return score;
      };

      // Undraw snake
      undraw_snake(snake);

      // Update snake
      append_tail(&snake->tail, snake->posX, snake->posY);
      snake->posX += snake->directionX;
      snake->posY += snake->directionY;

      if ((food->posX == snake->posX) && (food->posY == snake->posY)) {
	score++;
	sprintf(scoreBuffer, "Score: %d", score);

	if (score > highScore) {
	sprintf(highScoreBuffer, "High Score: %d", score);
	};

	speed = speed + 0.035*speed;
	undraw_food(food);
	init_food(food, snake);
	draw_food(food);
	grow_tail(&snake->tail);
      };

      // Redraw snake
      draw_snake(snake);

      t = 1.0;
    }
    
    else {
      t += speed;
    };

    REG_BG_OFS[0]= bg0_pt;	// write new position
    
  };

  return 0;
}

int main() {
  init_map();
  tte_init_con();

  tte_init_se_default(1, BG_CBB(CBB_1)|BG_SBB(SBB_1));
  
  REG_DISPCNT= DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ;

  int score = 0;
  
  // Write something
 while(1) {
   
    Snake snake = {0};
    init_snake(&snake);
    
    Food food = {0};
    init_food(&food, &snake);
    
    score = play_game(&snake, &food, score);

    destroy_snake(&snake);
    undraw_food(&food);
    pal_bg_bank[0][1]= RGB15(31,  0,  0);
  }

  return 0;
}

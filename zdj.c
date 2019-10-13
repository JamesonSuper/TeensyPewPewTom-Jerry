/**
 *  ZDJ.c, by Lawrence Buckingham.
 *
 *  Copyright (C) Queensland University of Technology, 2016-2019.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <cab202_graphics.h>
#include <cab202_timers.h>

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

// Set this to true when simulation is over
bool game_over = false;

// Hero state
double hero_x, hero_y;
int health;
#define HERO_IMG 'H'
#define MAX_HEALTH 100

// Zombie state
double zombie_x, zombie_y, zombie_dx, zombie_dy;
#define ZOMBIE_IMG 'Z'

// Gold state
double gold_x, gold_y;
int score;
#define GOLD_IMG 'G'

// Screen dimensions
int W, H;

/**
 *	Draw a border around the screen using '*' symbols.
 */
void draw_border() {
    const int ch = '*';
    draw_line(0, 0, 0, H - 1, ch);
    draw_line(0, 0, W - 1, 0, ch);
    draw_line(0, 2, W - 1, 2, ch);
    draw_line(0, H - 1, W - 1, H - 1, ch);
    draw_line(W - 1, 0, W - 1, H - 1, ch);
}

/**
 *	Draw the status information.
 */
void draw_status() {
    // Place-holder for something cool.
    draw_formatted(10, 1, "Health: %3d, Score: %3d", health, score);
}

/**
 *  Detects collision between objects that each occupy a single pixel.
 *
 *  Parameters:
 *  (x0, y0)   the position of one object.
 *  (x1, y1)    the position of the other object.
 *
 *  Returns true if and only if the rounded locations are the same.
 */
bool collided(double x0, double y0, double x1, double y1) {
    return round(x0) == round(x1) && round(y0) == round(y1);
}

/**
 *	Sets up the hero, placing it initially in the centre of the screen.
 */
void setup_hero() {
    hero_x = (W - 1) / 2;
    hero_y = (H - 1) / 2;
    health = MAX_HEALTH;
}

/**
 *	Draws the hero.
 */
void draw_hero() {
    draw_char(round(hero_x), round(hero_y), HERO_IMG);
}

/**
 *	Updates the position of the hero based on a key code.
 *
 *	Parameter: ch, the key code to process.
 */
void update_hero(int ch) {
    if (ch == 'a' && hero_x > 1) {
        hero_x--;
    }
    else if (ch == 'd' && hero_x < W - 2) {
        hero_x++;
    }
    else if (ch == 's' && hero_y < H - 2) {
        hero_y++;
    }
    else if (ch == 'w' && hero_y > 3) {
        hero_y--;
    }
}

/**
 *	Sets up the gold.
 */
void setup_gold() {
    gold_x = 1 + rand() % (W - 2);
    gold_y = 3 + rand() % (H - 4);
}

/**
 *	Draws the gold.
 */
void draw_gold() {
    draw_char(round(gold_x), round(gold_y), GOLD_IMG);
}

/**
 *  Updates the state of the gold, checking for collision with the hero and
 *  if necessary incrementing the score and re-spawning the gold.
 */
void update_gold(int key) {
    if (collided(hero_x, hero_y, gold_x, gold_y)) {
        score++;
        setup_gold();
    }
}

/**
 *	Draws the zombie.
 */
void draw_zombie() {
    draw_char(round(zombie_x), round(zombie_y), ZOMBIE_IMG);
}

/**
 *	Sets up the zombie at a random location and direction.
 */
void setup_zombie() {
    zombie_x = 1 + rand() % (W - 2);
    zombie_y = 3 + rand() % (H - 4);

    double zombie_dir = rand() * M_PI * 2 / RAND_MAX;
    const double step = 0.1;

    zombie_dx = step * cos(zombie_dir);
    zombie_dy = step * sin(zombie_dir);
}

void do_collided() {
    clear_screen();

    const char *message[] = {
        "Simulation over!",
        "Press any key to exit..."
    };

    const int rows = 2;

    for (int i = 0; i < rows; i++) {
        // Draw message in middle of screen.
        int len = strlen(message[i]);
        int x = (W - len) / 2;
        int y = (H - rows) / 2 + i;
        draw_formatted(x, y, message[i]);
    }

    show_screen();

    while (get_char() > 0) {}
    wait_char();

    game_over = true;
}

/**
 *	Moves the zombie a single step (if possible) with reflection
 *	from the border.
 */
void move_zombie() {
    // Assume that zombie is have not already collided with the borders.
    // Predict the next screen position of the zombie.
    int new_x = round(zombie_x + zombie_dx);
    int new_y = round(zombie_y + zombie_dy);

    bool bounced = false;

    if (new_x == 0 || new_x == screen_width() - 1) {
        // Bounce of left or right wall: reverse horizontal direction
        zombie_dx = -zombie_dx;
        bounced = true;
    }

    if (new_y == 2 || new_y == screen_height() - 1) {
        // Bounce off top or bottom wall: reverse vertical direction
        zombie_dy = -zombie_dy;
        bounced = true;
    }

    if (!bounced) {
        // no bounce: move instead.
        zombie_x += zombie_dx;
        zombie_y += zombie_dy;
    }
}

/**
 *	Moves the zombie (if it their turn), and checks for collision
 *	with hero.
 */
void update_zombie(int key) {
    if (key < 0) {
        move_zombie();
    }

    if (collided(hero_x, hero_y, zombie_x, zombie_y)) {
        setup_zombie();
        health--;

        if (health <= 0) {
            do_collided();
        }
    }
}

/**
 *	Draws the display.
 */
void draw_all() {
    clear_screen();
    draw_border();
    draw_status();
    draw_hero();
    draw_gold();
    draw_zombie();
    show_screen();
}

/**
 *  Sets up all objects in the game.
 */
void setup(void) {
    srand(get_current_time());
    W = screen_width();
    H = screen_height();
    setup_hero();
    setup_gold();
    setup_zombie();
}

/**
 *  Updates the state of all objects in the game, based on user input and
 *  the existing state.
 */
void loop() {
    int key = get_char();

    if (key == 'q') {
        game_over = true;
        return;
    }

    update_hero(key);
    update_gold(key);
    update_zombie(key);
}

/**
 *  Sets up the game and hosts the event loop.
 */
int main(void) {
    const int DELAY = 10;
    setup_screen();

    setup();

    while (!game_over) {
        draw_all();
        loop();
        timer_pause(DELAY);
    }

    return 0;
}
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <cab202_graphics.h>
#include <cab202_timers.h>
#include <time.h>

#define TOM 'T'
#define JERRY 'J'
#define PEN '*'
#define MAX_LEVELS 30

#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4
 
#define SOUTHEAST 32
#define NORTHEAST 12
#define SOUTHWEST 34
#define NORTHWEST 14

// GLOBAL VARIABLES -----
// For Wall drawing
int W, H, map_W, map_H;

// For timer
timer_id cheese_timer, automatic_firework_timer, character_movement_timer, trap_timer, fireworks_timer;
double game_start_time, game_current_time, start_pause_time, end_pause_time, elapsed_pause_time;
int game_elapsed_time, seconds, minutes;

// For on map items
int cheese_spawned = 0;
int cheese_coords[100][2];
int cheese_max = 5, cheese_eaten = 0;;
int trap_spawned, trap_activated;
int trap_coords[100][2];
int trap_max = 5;
int fireworks_active;
int fx, fy, prev_fy, prev_fx;
int door_x = -5, door_y = -5;
bool door_spawned;
int level = 1;
int passed_levels;

// Bool for pause, gameover and quit
bool paused = false, game_over = false, quit = false;

// Bool representing character selected
bool isJerry = true;

// For Jerry stat tracking
int jerry_score = 0;
int jerry_lives = 5;
double jx, jy, init_jx, init_jy;

// For Tom stat tracking
int tom_score = 0, tom_level_score = 0;
int tom_lives = 5;
double tx, ty, init_tx, init_ty;

// Creating an array for each coord
double x_1[MAX_LEVELS][100];
double y_1[MAX_LEVELS][100];
double x_2[MAX_LEVELS][100];
double y_2[MAX_LEVELS][100];
// Beginning Char
char beg_char[MAX_LEVELS][100];
// Amount of items scanned each line
int items_scanned[MAX_LEVELS][100];
// Number of succssfuly scanned lines per level
int wall_count[MAX_LEVELS];
//----------------------------------------------------//

// Reinitialise Variables
void restart_game(){
    level = 1;
    jerry_score = 0;
    tom_score = 0;
    jerry_lives = 5;
    tom_lives = 5;
    door_x = -5;
    door_y = -5;
    for(int i = 0; i < MAX_LEVELS; i++){
        for ( int j = 0; j < 100; j++){
            x_1[i][j] = 0;
            y_1[i][j] = 0;
            x_2[i][j] = 0;
            y_2[i][j] = 0;
            beg_char[i][j] = 0;
            items_scanned[i][j] = 0;
            
        }
        wall_count[i] = 0;
    }
    for(int k = 0; k < 100; k++){
        trap_coords[k][0] = -1;
        trap_coords[k][1] = -1;
        cheese_coords[k][0] = -1;
        cheese_coords[k][1] = -1;
    }
}

// Returns how many cheese is actually on the screen
int cheese_onscreen(){
    int counter = 0;
    for (int i = 0; i < 100; i++){
        if (cheese_coords[i][0] >= 0 && cheese_coords[i][1] >= 0){
            counter++;
        }
    }
    return counter;
}

// Drawing Status Bar
void status_bar(int input_time){
    // Calculating Time elapsed
    seconds = input_time - (minutes * 60);
    minutes = input_time / 60;
    
    draw_string((W/5)*0,0,"Student Number: N9943618");  
    draw_formatted((W/5)*4,0,"Time: %02d:%02d",minutes,seconds);

    if (isJerry == true){
        draw_formatted((W/5)*1,0,"Score: %d",jerry_score);
        draw_formatted((W/5)*2,0,"Lives: %d",jerry_lives);
        draw_string((W/5)*3,0,"Player: JERRY");
    }
    else if (isJerry == false){
        draw_formatted((W/5)*1,0,"Score: %d",tom_score);
        draw_formatted((W/5)*2,0,"Lives: %d",tom_lives);
        draw_string((W/5)*3,0,"Player: TOM");
    }

    // Second Line
    draw_formatted((W/5)*0,2,"Cheese: %d", cheese_onscreen());
    draw_formatted((W/5)*1,2,"Traps: %d", trap_spawned - trap_activated);
    draw_formatted((W/5)*2,2,"Weapons: %d", fireworks_active);
    draw_formatted((W/5)*3,2,"Level: %d", level);

    // Third line ending heading
    draw_line(0,3,W-1,3,'-');
}

// File to Arrays function
void filetoArray(char *argv[], int level){
    // Current file as 'stream'
    FILE * stream = fopen (argv[level],"r");
    int j = 0;
    wall_count[level]=0;
    // Looping until EOF is reached
    while(! feof( stream )){
        items_scanned[level][j] = fscanf(stream, "%c %lf %lf %lf %lf", 
                    &beg_char[level][j], &x_1[level][j], 
                    &y_1[level][j], &x_2[level][j], &y_2[level][j]);
        j++;
        wall_count[level]++;
    }
}  

// Input levels from file to set arrays
void load_levels(char *argv[], int argc){
    for (int i = 1; i < argc; i++){
        filetoArray(argv, i);
    }
}

// Algorithm to workout abs distance between 2 coords
int taxicab_geometry(int x1, int y1, int x2, int y2){
    int x_dist = abs(x2) - abs(x1);
    x_dist = pow(x_dist, 2);

    int y_dist = abs(y2) - abs(y1);
    y_dist = pow(y_dist, 2);

    return x_dist + y_dist;
}

// Choosing the smallest of the provided integers
int smallest_int(int left, int right, int up, int down){
    if (left < right || left == right){
        if (left < up || left == up){
            if (left < down || left == down){
                return left;
            }
        }
    }
    if (right < left || right == left){
        if (right < up || right == up){
            if (right < down || right == down){
                return right;
            }
        }
    }
    if (up < right || up == right){
        if (up < down || up == down){
            if (up < left || up == left){
                return up;
            }
        }
    }
    if (down < right || down == right){
        if (down < up || down == up){
            if (down < left || down == left){
                return down;
            }
        }
    }
    return 0;
}

// Choosing the largest of the provided integers
int largest_int(int left, int right, int up, int down){
    if (left > right || left == right){
        if (left > up || left == up){
            if (left > down || left == down){
                return left;
            }
        }
    }
    if (right > left || right == left){
        if (right > up || right == up){
            if (right > down || right == down){
                return right;
            }
        }
    }
    if (up > right || up == right){
        if (up > down || up == down){
            if (up > left || up == left){
                return up;
            }
        }
    }
    if (down > right || down == right){
        if (down > up || down == up){
            if (down > left || down == left){
                return down;
            }
        }
    }
    return 0;
}

// Making Toms seeking movement
void tom_auto_movement(){
    // Checking if Tom is near wall and if so, do not check pixel char in that direction to avoid seg fault.
    if ( ty >= H-1 && tx > map_W - 1){
        ty--;
        tx--;
        return;
    }
    else if ( ty > H - 2){
        int char_left = zdk_screen->pixels[(int)ty][(int)tx-1];
        int char_right = zdk_screen->pixels[(int)ty][(int)tx+1];
        int char_up = zdk_screen->pixels[(int)ty-1][(int)tx];
        // Setting defauls to large integers incase they cannot be calculated        
        int left = 100000, right = 100000, up = 100000, down = 100000;

        if (char_left != '*'){
            left = taxicab_geometry((int)tx-1,(int)ty,(int)jx,(int)jy);}
        if( char_right != '*'){
            right = taxicab_geometry((int)tx+1,(int)ty,(int)jx,(int)jy);}
        if( char_up != '*'){
            up = taxicab_geometry((int)tx,(int)ty-1,(int)jx,(int)jy);}
        int move = smallest_int(left, right, up, down);
        if (move == left){
            tx--;
            return;}
        else if ( move == right){
            tx++;
            return;}
        else if ( move == up ){
            ty--;
            return;}
        else if ( move == down){
            ty++;
            return;}
    } 
    else if ( tx <= 0){  
        int char_right = zdk_screen->pixels[(int)ty][(int)tx+1];
        int char_up = zdk_screen->pixels[(int)ty-1][(int)tx];
        int char_down = zdk_screen->pixels[(int)ty+1][(int)tx];    
        // Setting defauls to large integers incase they cannot be calculated.
        int left = 100000, right = 100000, up = 100000, down = 100000;

        if( char_right != '*'){
            right = taxicab_geometry((int)tx+1,(int)ty,(int)jx,(int)jy);}
        if( char_up != '*'){
            up = taxicab_geometry((int)tx,(int)ty-1,(int)jx,(int)jy);}
        if( char_down != '*'){
            down = taxicab_geometry((int)tx,(int)ty+1,(int)jx,(int)jy);}
        int move = smallest_int(left, right, up, down);

        if (move == left){
            tx--;
            return;}
        else if ( move == right){
            tx++;
            return;}
        else if ( move == up ){
            ty--;
            return;}
        else if ( move == down){
            ty++;
            return;}
    }
    else if ( tx > W){  
        int char_left = zdk_screen->pixels[(int)ty][(int)tx-1];
        int char_up = zdk_screen->pixels[(int)ty-1][(int)tx];
        int char_down = zdk_screen->pixels[(int)ty+1][(int)tx];
        
        int left = 100000, right = 100000, up = 100000, down = 100000;

        if ( char_left != '*'){
            left = taxicab_geometry((int)tx-1,(int)ty,(int)jx,(int)jy);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)tx,(int)ty-1,(int)jx,(int)jy);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)tx,(int)ty+1,(int)jx,(int)jy);
        }
        int move = smallest_int(left, right, up, down);

        if (move == left){
            tx--;
            return;
        }
        else if ( move == right){
            tx++;
            return;
        }
        else if ( move == up ){
            ty--;
            return;
        }
        else if ( move == down){
            ty++;
            return;
        }

    }
    else {
        int char_left = zdk_screen->pixels[(int)ty][(int)tx-1];
        int char_right = zdk_screen->pixels[(int)ty][(int)tx+1];
        int char_up = zdk_screen->pixels[(int)ty-1][(int)tx];
        int char_down = zdk_screen->pixels[(int)ty+1][(int)tx];

        int left = 100000, right = 100000, up = 100000, down = 100000;

        if (char_left != '*'){
            left = taxicab_geometry((int)tx-1,(int)ty,(int)jx,(int)jy);
        }
        if( char_right != '*'){
            right = taxicab_geometry((int)tx+1,(int)ty,(int)jx,(int)jy);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)tx,(int)ty-1,(int)jx,(int)jy);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)tx,(int)ty+1,(int)jx,(int)jy);
        }
        int move = smallest_int(left, right, up, down);

        if (move == left){
            tx--;
            return;
        }
        else if ( move == right){
            tx++;
            return;
        }
        else if ( move == up ){
            ty--;
            return;
        }
        else if ( move == down){
            ty++;
            return;
        }
    }
}

// Jerry to seek cheese if tom is far enough away
void jerry_seek_cheese(){
    // Deciding the closest cheese
    // Creating an array to store all Cheese On Screen.
    int cheese_toget[cheese_onscreen()][2];
    int j = 0;

    // Iterating through ALL of Cheese Coords to extract on screen ones.
    for (int i = 0; i < 100; i++){
        if (cheese_coords[i][0] > 0 && cheese_coords[i][1] > 0){
            cheese_toget[j][0] = cheese_coords[i][0];
            cheese_toget[j][1] = cheese_coords[i][1];
            j++;
        }
    }

    // Doing taxicab from each cheese_toget coord.
    int cheese_taxi_measures[j];
    for (int i = 0; i < j; i++){
        cheese_taxi_measures[i] = taxicab_geometry(jx,jy,cheese_toget[i][0],cheese_toget[i][1]);
    }

    // Sorting Array IF longer than 1 long
    int temp;
    if (j > 1){
        for (int i = 0; i < j; i++){
            for (int k = i + 1; k < j; k++){
                if(cheese_taxi_measures[i] > cheese_taxi_measures[k]){
                    temp = cheese_taxi_measures[i];
                    cheese_taxi_measures[i] = cheese_taxi_measures[k];
                    cheese_taxi_measures[k] = temp;
                }
            }
        }
    }
    
    int closest_cheese_x;
    int closest_cheese_y;

    for (int i = 0; i < j; i++){
        if (cheese_taxi_measures[0] == taxicab_geometry(jx,jy,cheese_toget[i][0],cheese_toget[i][1])){
            closest_cheese_x = cheese_toget[i][0];
            closest_cheese_y = cheese_toget[i][1];
        }
    }

    // Deciding on best route to closest cheese
    if (jy == H-1){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];

        int left = 1000000, right = 1000000, up = 1000000, down = 1000000;

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,closest_cheese_x,closest_cheese_y);
        }
        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,closest_cheese_x,closest_cheese_y);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,closest_cheese_x,closest_cheese_y);
        }
        int move = smallest_int(left, right, up, down);

        if (move == left){
            jx--;
            return;
        }
        else if ( move == right){
            jx++;
            return;
        }
        else if ( move == up ){
            jy--;
            return;
        }
    }
    else{
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];
        int char_down = zdk_screen->pixels[(int)jy+1][(int)jx];

        int left = 1000000, right = 1000000, up = 1000000, down = 1000000;

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,closest_cheese_x,closest_cheese_y);
        }
        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,closest_cheese_x,closest_cheese_y);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,closest_cheese_x,closest_cheese_y);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy+1,closest_cheese_x,closest_cheese_y);
        }
        int move = smallest_int(left, right, up, down);

        if (move == left){
            jx--;
            return;
        }
        else if ( move == right){
            jx++;
            return;
        }
        else if ( move == up ){
            jy--;
            return;
        }
        else if ( move == down){
            jy++;
            return;
        }
    }
}

// Returns an int that corresponds to a DEFINED direction
int check_for_border(int x,int y){ 
    //corners first
    // Top Left
    if(x == 0 && y == 4){
        return NORTHWEST;
    }
    // Top Right
    if(x == W-1 && y == 4){
        return NORTHEAST;
    }
    // Bottom Right
    if(x == W-1 && y == H-1){
        return SOUTHEAST;
    }
    // Bottom Left
    if(x == 0 && y == H-1){
        return SOUTHWEST;
    }
    
    //now simply walls
    if (x == 0){        
        return WEST;
    }
    if (x == W-1){
        return EAST;
    }
    if (y == 4){
        return NORTH;
    }
    if (y == H-1){
        return SOUTH;
    }
    //return 0 if no wall/corner
    return 0;
 
}

// Function to check if Jerry is on the border when auto-moving
void movement_boundary_safe(int boundary_direction){
   
    //if boundary_direction == 0, no boundarys, business as usual
    int left = 0, right = 0, up = 0, down = 0;
    if(boundary_direction == NORTH){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_down = zdk_screen->pixels[(int)jy-1][(int)jx];

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,(int)tx,(int)ty);}
        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,(int)tx,(int)ty);}
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);}
        
        int move = largest_int(left, right, up, down);
        if( move == left){
            jx--;}
        else if ( move == right){
            jx++;}
        else if ( move == down){
            jy++;}
    }
    if(boundary_direction == WEST){

        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];
        int char_down = zdk_screen->pixels[(int)jy+1][(int)jx];

        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,(int)tx,(int)ty);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy+1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);

        if ( move == right){
            jx++;}
        else if ( move == up ){
            jy--;}
        else if ( move == down){
            jy++;}
    }        
    if(boundary_direction == SOUTH){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,(int)tx,(int)ty);
        }
        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,(int)tx,(int)ty);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);
        }
        
        int move = largest_int(left, right, up, down);

        if (move == left){
            jx--;}
        else if ( move == right){
            jx++;}
        else if ( move == up ){
            jy--;}
    }
    if(boundary_direction == EAST){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];
        int char_down = zdk_screen->pixels[(int)jy+1][(int)jx];

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,(int)tx,(int)ty);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy+1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);

        if (move == left){
            jx--;}
        else if ( move == up ){
            jy--;}
        else if ( move == down){
            jy++;}
    }
    if(boundary_direction == NORTHEAST){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_down = zdk_screen->pixels[(int)jy+1][(int)jx];

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,(int)tx,(int)ty);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy+1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);

        if (move == left){
            jx--;}
        else if ( move == down){
            jy++;}
    }
    if(boundary_direction == NORTHWEST){
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_down = zdk_screen->pixels[(int)jy+1][(int)jx];

        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,(int)tx,(int)ty);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy+1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);
        
        if ( move == right){
            jx++;}
        else if ( move == down){
            jy++;}
    }
    if(boundary_direction == SOUTHWEST){
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];

        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,(int)tx,(int)ty);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);

        if ( move == right){
            jx++;}
        else if ( move == up ){
            jy--;}
    }
    if(boundary_direction == SOUTHEAST){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,(int)tx,(int)ty);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);

        if (move == left){
            jx--;}
        else if ( move == up ){
            jy--;}
    }
    else if(boundary_direction == 0){
        int char_left = zdk_screen->pixels[(int)jy][(int)jx-1];
        int char_right = zdk_screen->pixels[(int)jy][(int)jx+1];
        int char_up = zdk_screen->pixels[(int)jy-1][(int)jx];
        int char_down = zdk_screen->pixels[(int)jy+1][(int)jx];

        if (char_left != '*'){
            left = taxicab_geometry((int)jx-1,(int)jy,(int)tx,(int)ty);
        }
        if( char_right != '*'){
            right = taxicab_geometry((int)jx+1,(int)jy,(int)tx,(int)ty);
        }
        if( char_up != '*'){
            up = taxicab_geometry((int)jx,(int)jy-1,(int)tx,(int)ty);
        }
        if( char_down != '*'){
            down = taxicab_geometry((int)jx,(int)jy+1,(int)tx,(int)ty);
        }
        int move = largest_int(left, right, up, down);

        if (move == left){
            jx--;}
        else if ( move == right){
            jx++;}
        else if ( move == up ){
            jy--;}
        else if ( move == down){
            jy++;}
    }
}

// Function to decide whether to go to cheese, or run away from Tom
void jerry_auto_movement(){
    if(taxicab_geometry(jx,jy,tx,ty) < 400){
        movement_boundary_safe(check_for_border((int)jx,(int)jy));
    }
    else if(cheese_onscreen() > 0){
        jerry_seek_cheese();
    }
    // else{
    //     movement_boundary_safe(check_for_border((int)jx,(int)jy));
    // }
}

// Function to check next pixel to see if wall or Tom
bool firework_missed(char next_step){
    if (next_step == '*'){
        destroy_timer(fireworks_timer);
        fireworks_active = 0;
        fx = -10;
        fy = -10;
        return true;
    }
    else if (fx == tx && fy == ty){
        destroy_timer(fireworks_timer);
        fireworks_active = 0;
        tx = init_tx;
        ty = init_ty;
        if (!isJerry){
            tom_lives--;
        }
        return true;
    }
    else{
        return false;
    }
}

// Firework Automatic Movement
void fireworks(){
    prev_fx = fx;
    prev_fy = fy;
    if (fy > H-2){
        int left = taxicab_geometry(fx-1,fy,tx,ty);
        int right = taxicab_geometry(fx+1,fy,tx,ty);
        int up = taxicab_geometry(fx,fy-1,tx,ty);
        int down = 100000;

        int left_char = zdk_screen->pixels[(int)fy][(int)fx-1];
        int right_char = zdk_screen->pixels[(int)fy][(int)fx+1];
        int up_char = zdk_screen->pixels[(int)fy-1][(int)fx];
        
        int move = smallest_int(left, right, up, down);

        if(move == left){
            if (!firework_missed(left_char)){
                fx = fx - 1;
            } 
            
        }    
        else if(move == right){
            if (!firework_missed(right_char)){
                fx = fx + 1;
            }
        }
        else if(move == up){
            if (!firework_missed(up_char)){
                fy = fy - 1;
            }
        }
    }

    else{
        int left = taxicab_geometry(fx-1,fy,tx,ty);
        int right = taxicab_geometry(fx+1,fy,tx,ty);
        int up = taxicab_geometry(fx,fy-1,tx,ty);
        int down = taxicab_geometry(fx,fy+1,tx,ty);

        int left_char = zdk_screen->pixels[(int)fy][(int)fx-1];
        int right_char = zdk_screen->pixels[(int)fy][(int)fx+1];
        int up_char = zdk_screen->pixels[(int)fy-1][(int)fx];
        int down_char = zdk_screen->pixels[(int)fy+1][(int)fx];

        int move = smallest_int(left, right, up, down);

        if(move == left){
            if (!firework_missed(left_char)){
                fx = fx - 1;
            } 
            
        }    
        else if(move == right){
            if (!firework_missed(right_char)){
                fx = fx + 1;
            }
        }
        else if(move == up){
            if (!firework_missed(up_char)){
                fy = fy - 1;
            }
        }
        else if(move == down){
            if (!firework_missed(down_char)){
                fy = fy + 1;
            }
        }
    }
}

// Collision Detection with returns int code of next space
bool is_wall(int y, int x){
    
    int next_char = zdk_screen->pixels[y][x];
    if (next_char == '*'){
        return true;
    }
    return false;
}

// Check if pixel occupied function for cheese and door placement
bool check_occupied(int x, int y){
    int next_char = zdk_screen->pixels[y][x];
    if (next_char == ' '){
        return false;
    }
    else{
        return true;
    }    
}

// Create Random coordinate within screen width/height
int random_x(){
    int x_rand = abs(rand() % W - 1);
    return x_rand;
}
int random_y(){
    int y_rand = abs(rand() % H - 4) + 4;
    return y_rand;
}

// Create Cheese Coordinates
void create_cheese(){
    int x_rand = random_x();
    int y_rand = random_y();

    bool result = check_occupied(x_rand, y_rand);
    if (!result){
        cheese_coords[cheese_spawned][0] = x_rand;
        cheese_coords[cheese_spawned][1] = y_rand;
    }
    else{
        create_cheese();
    }
}

// Create Trap Coordinates
void create_trap(){
    trap_coords[trap_spawned][0] = tx;
    trap_coords[trap_spawned][1] = ty;
}

// Spawn Cheese function
void draw_cheese(int x, int y){
    draw_char(x,y,'C');
}

// Spawn Trap function
void draw_trap(int x, int y){
    draw_char(x,y,'#');
}

// Drawing Character Functions
void draw_tom(double x_1, double y_1){
    draw_char((x_1), (y_1), TOM);
}
void draw_jerry(double x_1, double y_1){
    draw_char((x_1), (y_1), JERRY);
}

// Draw Walls Functions
void initial_load_room(){
    // iterating through coords provided and drawing screen
    for (int i = 0; i < 100; i++){
        if (items_scanned[level][i] == 3){
            if (beg_char[level][i] == 'T'){
                tx = round((x_1[level][i]*map_W));
                ty = round((y_1[level][i]*map_H)+4);
                draw_tom(tx,ty);
                init_tx = tx;
                init_ty = ty;
            }
            else if (beg_char[level][i] == 'J'){
                jx = round((x_1[level][i]*map_W));
                jy = round((y_1[level][i]*map_H)+4);
                draw_jerry(jx,jy);
                init_jx = jx;
                init_jy = jy;
            }
        }
        else if (items_scanned[level][i] == 5){
            draw_line(round(x_1[level][i]*map_W), round(y_1[level][i]*map_H+4), 
            round(x_2[level][i]*map_W), round(y_2[level][i]*map_H+4), PEN);
        }
    }
}

// Check if curr pos of Jerry/Tom overlaps with an item
void check_stepped_on(){
    if (isJerry){
        if (jx == door_x && jy == door_y){
            door_x = -12;
            door_y = -21;
            level++;
            door_spawned = false;
            cheese_eaten = 0;
            trap_spawned = 0;
            cheese_spawned = 0;
            tom_level_score = 0;
            // Setting Tom and Jerry to their new coords
            initial_load_room();
            for(int i = 0; i < 100; i++){
                cheese_coords[i][0] = -1;
                cheese_coords[i][1] = -1;
                trap_coords[i][0] = -1;
                trap_coords[i][1] = -1;
            }
            if (level == passed_levels){
                game_over = true;
            }
        }    
    }
    else if(!isJerry){
        if (tx == door_x && ty == door_y){
            door_x = -12;
            door_y = -21;
            level++;
            door_spawned = false;
            cheese_eaten = 0;
            trap_spawned = 0;
            cheese_spawned = 0;
            tom_level_score = 0;
            // Setting Tom and Jerry to their new coords
            initial_load_room();
            for(int i = 0; i < 100; i++){
                cheese_coords[i][0] = -1;
                cheese_coords[i][1] = -1;
                trap_coords[i][0] = -1;
                trap_coords[i][1] = -1;
            }
            if (level == passed_levels){
                game_over = true;
            }
        }  
    }
    if (jx == tx && jy == ty){
        // Setting character positions to init values
        jx = init_jx;
        jy = init_jy;
        tx = init_tx;
        ty = init_ty;

        // Taking a life
        if(isJerry){
            jerry_lives = jerry_lives - 1;
        }
        // Adding points to Toms Score
        if (!isJerry){            
            tom_score += 5;
            tom_level_score += 5;
        }
    } 
    else{
        for (int i = 0; i < 100; i++){
            if (jx == cheese_coords[i][0] && jy == cheese_coords[i][1]){
                jerry_score++;
                cheese_eaten++;
                // if cheese is found, make it appear of screen rest of game.
                cheese_coords[i][0] = -1;
                cheese_coords[i][1] = -1;
                cheese_max++;
            }
        }
        for (int i = 0; i < trap_spawned; i++){
            if (jx == trap_coords[i][0] && jy == trap_coords[i][1]){
                // if trap is found, make it appear of screen rest of game.
                trap_coords[i][0] = -1;
                trap_coords[i][1] = -1;
                trap_max++;
                trap_activated++;
                if(isJerry){
                    jerry_lives--;
                }
                else if(!isJerry){
                    tom_score++;
                    tom_level_score++;
                }
            }
        }

    }
}

// Manual Character movement functions
void TomManualMovement(int key_code){
    if ( key_code == 'a' && tx >= 1){  
        if (!is_wall(ty,tx-1)){
            tx--;
        }
        
    }
    else if ( key_code == 'd' && tx < W - 1){  
        if (!is_wall(ty,tx+1)){
            tx++;
        }
    }
    else if ( key_code == 'w' && ty > 4){          
        if (!is_wall(ty-1,tx)){
            ty--;
        }
    }
    else if ( key_code == 's' && ty < H - 1){
        if (!is_wall(ty+1,tx)){
            ty++; 
        }
    }
    check_stepped_on();
}
void JerryManualMovement(int key_code){
    if ( key_code == 'a' && jx >= 1){  
        if (!is_wall(jy,jx-1)){
            jx--;
        }
        
    }
    else if ( key_code == 'd' && jx < W - 1){  
        if (!is_wall(jy,jx+1)){
            jx++;
        }
    }
    else if ( key_code == 'w' && jy > 4){          
        if (!is_wall(jy-1,jx)){
            jy--;
        }
    }
    else if ( key_code == 's' && jy < H - 1){
        if (!is_wall(jy+1,jx)){
            jy++; 
        }
    }
    check_stepped_on();
}

// Draw walls function to draw walls every loop
void draw_walls(){
    // Iterating through coords provided and drawing screen
    for (int i = 0; i < 100; i++){
        if (items_scanned[level][i] == 5){
            draw_line(round(x_1[level][i]*map_W), round(y_1[level][i]*map_H+4), 
            round(x_2[level][i]*map_W), round(y_2[level][i]*map_H+4), PEN);
        }
    }
}

// Conditions on whether to spawn door or not
void gen_door_coords(){
    if ((cheese_eaten > 4 && door_spawned == false) || (tom_level_score > 4 && door_spawned == false)){
        door_x = random_x();
        door_y = random_y();
        if (!check_occupied(door_x, door_y)){
            door_spawned = true;
        }
        else{
            gen_door_coords();
        }
    }
}

// Function called to draw the door.
void draw_door(){
    draw_char(door_x,door_y,'X');
}

// Function called 
void setup(int argc, char *argv[]){
    // Setting global vars
    W = screen_width();
    H = screen_height();
    map_W = W-1;
    map_H = H-5;

    clear_screen();
    load_levels(argv, argc);

    initial_load_room();
}

void loop(){
    clear_screen();
    int key = get_char();
    
    // Do whilst paused
    if (paused){  
        if (key == ' '){
            paused = false;
            end_pause_time = get_current_time();
        }
        
        draw_walls();
        status_bar(start_pause_time - game_start_time);
        
        // Drawing Trap coords
        for(int i = 0; i < trap_spawned; i++){
            draw_trap(trap_coords[i][0],trap_coords[i][1]);
        }

        // Drawing Cheese coords
        for(int i = 0; i < cheese_spawned; i++){
            draw_cheese(cheese_coords[i][0],cheese_coords[i][1]);
        }

        JerryManualMovement(key);
        draw_string(W/4.3,(H/3.2)+4,"GAME IS PAUSED PRESS SPACE TO CONTINUE");
        draw_jerry(jx,jy);
        draw_tom(tx,ty);
        show_screen();
    }

    // Start Pause
    if (key == 'p'){
        paused = true;
        start_pause_time = get_current_time();
    }

    // If not paused, run below
    else if (!paused){
        // Calculating elapsed time since start of game 
        game_current_time = get_current_time();
        elapsed_pause_time = end_pause_time - start_pause_time;
        game_elapsed_time = (int)floor(game_current_time - game_start_time - elapsed_pause_time);  

        draw_walls();
        status_bar(game_elapsed_time);
        
        if (key == 'z'){
            if (isJerry == true){
                isJerry = false;
            }
            else if (isJerry == false){
                isJerry = true;
            }
        }
        // Two paths of movement, depending on which character is selected
        if (isJerry){
            if (key == 'f' && fireworks_active == 0){
                fx = jx;
                fy = jy;
                fireworks_timer = create_timer(30);
                fireworks_active = 1;
            }
            if (fireworks_active == 1){
                if (timer_expired(fireworks_timer)){
                    fireworks();
                }
            }
            JerryManualMovement(key);
            if (timer_expired(character_movement_timer)){
                tom_auto_movement();
            }
        }
        else if (isJerry == false){
            if (key == 'c' && cheese_onscreen() < 5){
                cheese_coords[cheese_spawned][0] = tx;
                cheese_coords[cheese_spawned][1] = ty;
                cheese_spawned++;
            }
            else if (key == 'm' && (trap_spawned - trap_activated) < 5){
                trap_coords[trap_spawned][0] = tx;
                trap_coords[trap_spawned][1] = ty;
                trap_spawned++;
            }
            TomManualMovement(key);
            if (timer_expired(character_movement_timer)){
                jerry_auto_movement();
            }
            if (timer_expired(automatic_firework_timer)){
                fx = jx;
                fy = jy;
                fireworks_timer = create_timer(30);
                fireworks_active = 1;
            }
            if (fireworks_active == 1){
                if (timer_expired(fireworks_timer)){
                    fireworks();
                }
            }
        }

        // Creating a new cheese coord every 5 secs
        if (timer_expired(cheese_timer) && cheese_spawned != cheese_max){
            create_cheese();
            cheese_spawned++;
        }

        // Creating a new Trap every 3 secs
        if (timer_expired(trap_timer) && trap_spawned != trap_max){
            create_trap();
            trap_spawned++;
        }

        // Drawing Trap coords
        for(int i = 0; i < trap_spawned; i++){
            draw_trap(trap_coords[i][0],trap_coords[i][1]);
        }

        // Drawing Cheese coords
        for(int i = 0; i < cheese_spawned; i++){
            draw_cheese(cheese_coords[i][0],cheese_coords[i][1]);
        }
        // Drawing fireworks if in flight
        if (fireworks_active == 1){
            draw_char(prev_fx, prev_fy,'-');
            draw_char(fx,fy,'+');
        }
        gen_door_coords();
        draw_door();
        draw_jerry(jx,jy);
        draw_tom(tx,ty);
    }

    show_screen();
    if (tom_lives <= 0 || jerry_lives <= 0){
        game_over = true;
    }

}

int main(int argc, char *argv[]){
    // ZDK screen init
    setup_screen();
    // Initialising Random
    srand(time(0));
    game_start_time = get_current_time();
    
    while (quit == false){
        restart_game();
        clear_screen();
        // Initialising Timer game start time and 5 second timer 
        cheese_timer = create_timer(5000);
        trap_timer = create_timer(3000);
        character_movement_timer = create_timer(150);
        automatic_firework_timer = create_timer(5000);

        // Passing first text file through to setup
        setup(argc, argv);
        passed_levels = argc;
        while (game_over == false) {
            loop();
        }
        
        // Program enters here when users life drops to 0
        clear_screen();
        draw_string(W/3,(H/3),"GAME OVER!! Press 'r' to Restart or 'q' to quit.");
        show_screen();
        char input = wait_char();
        
        if(input == 'r'){
            game_over = false;
            clear_screen();
            draw_string(W/3,(H/3)+1,"this worked.");
            show_screen();
        }
        else if(input == 'q'){
            quit = true;
            return 0;
        }
        clear_screen();
    }
}
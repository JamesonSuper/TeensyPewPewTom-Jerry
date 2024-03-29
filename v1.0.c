#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include <macros.h>
#include <cpu_speed.h>
#include <lcd_model.h>
#include <graphics.h>
#include <cab202_adc.h>

// Global Variables
double tx = LCD_X - 6, ty = LCD_Y - 9, tdx, tdy, direction;
double jx = 0, jy = 8;
int levelNum = 1;
int jerryLives = 5;
int jerryScore = 0;
unsigned char paused = 1;
double pausedTime = 0;
double gameTime = 0;
uint8_t elapsedMins = 0;

int cheese_max = 5;
int cheese_eaten = 0;
int cheese_secs = 4;

int traps_max = 5;
int traps_eaten = 0;
int traps_secs = 6;

double charSpeed = 0.3;
double wallSpeed = 0.01;
uint8_t wallBuffer[504];

int left_adc;
int right_adc;

typedef struct{
	int x1, y1, onScreen;
}items;
typedef struct{
	double x1, y1, x2, y2, dx, dy, deltax, deltay;
}wall;

items cheese_coords[5];
items trap_coords[5];
items door;
items firework_coords[20];
wall walls[4];

///Interrupts for timer and debouncing
volatile int timeCounter = 0;

volatile uint8_t SW3Counter = 0;
volatile uint8_t SW3State = 0;
int prevSW3State = 0;

volatile uint8_t SW2Counter = 0;
volatile uint8_t SW2State = 0;
int prevSW2State = 0;

volatile uint8_t JoystickUpCounter = 0;
volatile uint8_t JoystickUpState = 0;

volatile uint8_t JoystickDownCounter = 0;
volatile uint8_t JoystickDownState = 0;

volatile uint8_t JoystickLeftCounter = 0;
volatile uint8_t JoystickLeftState = 0;

volatile uint8_t JoystickRightCounter = 0;
volatile uint8_t JoystickRightState = 0;

volatile uint8_t JoystickCentreCounter = 0;
volatile uint8_t JoystickCentreState = 0;
int prevJoystickCentreState = 0;
// Global variables end



// Given an x and y, return the pixels state
// at that location on the current buffer
int IsPixelSet(int x, int y, uint8_t buffer[])
{
	int bank = y / 8;
	int bit = y % 8;
	int index = (bank * 84) + x;
	uint8_t byte = buffer[index];
	return BIT_VALUE(byte, bit);
}

int random_x(){
	return abs(rand() % 80); // 80 To account for width of trap bitmap
}
int random_y(){
	return abs(rand() % 48-11) + 8; // - 11 as 8 for status bar and 3 for cheese/trap height 
}

// Returns 1 if space occupied, 0 otherwise
int check_occupied(int x, int y, int length, int height){
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < length; j++)
		{
			if(IsPixelSet(x+j, y+i, screen_buffer)){
				return 1;
			}
		}
	}
	return 0;
}


// Copying screen buffer to walls buffer
void copyScreenBuffer(uint8_t screen_buffer[], uint8_t wallBuffer[]){
	for (int i = 0; i < 504; i++)
	{
		wallBuffer[i] = screen_buffer[i];
	}
}

///Draws a double to the screen
void draw_double(uint8_t x, uint8_t y, double value, colour_t colour) {
	char buffer[20];
	snprintf(buffer, sizeof(buffer), "%f", value);
	draw_string(x, y, buffer, colour);
}

///Draws an int to the screen, if last arg is 1, the
///int is printed with a leading 0
void draw_int(uint8_t x, uint8_t y, uint8_t value, colour_t colour, int leadingZero) {
	char buffer[10];
	if(leadingZero == 1){
		snprintf(buffer, sizeof(buffer), "%02d", value);
		draw_string(x, y, buffer, colour);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%d", value);
		draw_string(x, y, buffer, colour);
	}
}

void jerry_dead(){
	jerryLives--;
	jx = 0, jy = 8;	
}

void tom_dead(){
	tx = LCD_X - 6, ty = LCD_Y - 9;
}

void reset_dir_and_speed(){
	direction  = rand() * M_PI * 2 / RAND_MAX;
}

// Provide new chars topleft x and y, direction will then check that characters perimiter for a wall.
int wall_collision_check(int new_x, int new_y, char direction){
	int char_left = (int)new_x;
	int char_right = (int)new_x+4;
	int char_top = (int)new_y;
	int char_bottom = (int)new_y+4;
	
	int result = 0;
	switch (direction)
	{
	case 'U':
		for (int i = 0; i < 5; i++)
		{
			// Start Top Left - Checking Top side border
			if(IsPixelSet(char_left + i, char_top, wallBuffer)){
				result = 1;
			}
		}
		break;
	case 'D':
		for (int i = 0; i < 5; i++)
		{
			// Start Bottom left - Checking bottom side
			if(IsPixelSet(char_left + i, char_bottom, wallBuffer)){
				result = 1;
			}
		}
		break;
	case 'L':
		for (int i = 0; i < 5; i++)
		{
			// Start Top Left - Checking left side border
			if(IsPixelSet(char_left, char_top + i, wallBuffer)){
				result = 1;
			}
		}
		break;
	case 'R':
		for (int i = 0; i < 5; i++)
		{
			// Start Top Right - Checking right side border
			if(IsPixelSet(char_right, char_top + i, wallBuffer)){
				result = 1;
			}
		}
		break;
	default:
		break;
	}
	return result;
}

void create_walls(){
	wall wall1 = {18, 15, 13, 25, (10*wallSpeed), (5*wallSpeed), 5, 10};
	wall wall2 = {25, 35, 25, 45, (10*wallSpeed), (0*wallSpeed), 0, 10};
	wall wall3 = {45, 10, 60, 10, (0*wallSpeed), (15*wallSpeed) * -1, 15, 0};
	wall wall4 = {58, 25, 72, 30, (5*wallSpeed) * -1, (14*wallSpeed), 14, 5};
	walls[0] = wall1;
	walls[1] = wall2;
	walls[2] = wall3;
	walls[3] = wall4;
}

//void collision_check

void react_to_walls(char character){
	char collidedU = 0;
	char collidedD = 0;
	char collidedL = 0;
	char collidedR = 0;

	if (character == 'T')
	{
		// If wall is coming from the Right
		if (wall_collision_check(tx, ty, 'L') == 1)
		{
			collidedL = 1;
			tx++;
		}
		// If wall is coming from the Left
		else if (wall_collision_check(tx, ty, 'R') == 1)
		{
			collidedR = 1;
			tx--;
		}
		// If wall is coming from above
		if (wall_collision_check(tx, ty, 'U') == 1)
		{
			collidedU = 1;
			ty++;
		}
		// If wall is coming from below
		else if (wall_collision_check(tx, ty+1, 'D') == 1)
		{
			collidedD = 1;
			ty--;
		}
		if (collidedL || collidedR ||collidedU || collidedD)
		{
			reset_dir_and_speed();
		}		
		else if (ty < 6 || ty > 45 || tx < -1 || tx > 82)
		{
			reset_dir_and_speed();
			tom_dead();
		}
	}
	
	else if (character == 'J')
	{	
		// If wall is coming from the Right
		if (wall_collision_check((int)jx, (int)jy, 'L') == 1)
		{
			collidedL = 1;
			jx++;
		}
		// If wall is coming from the Left
		else if (wall_collision_check((int)jx, (int)jy, 'R') == 1)
		{
			collidedR = 1;
			jx--;
		}
		// If wall is coming from above
		if (wall_collision_check((int)jx, (int)jy, 'U') == 1)
		{
			collidedU = 1;
			jy++;
		}
		// If wall is coming from below
		else if (wall_collision_check((int)jx, (int)jy, 'D') == 1)
		{
			collidedD = 1;
			jy--;
		}
		if ((collidedL && collidedR) || (collidedU && collidedD))
		{
			jerry_dead();
		}
		else if ((collidedL && collidedR && collidedU) || (collidedL && collidedR && collidedD))
		{
			jerry_dead();
		}
		/// THIS NEEDS WORK, CLIPPING INTO THE WALL MOVING RIGHT WHEN JERRY MOVING LEFT WTF 
		
		else if ((int)jy <= 7 || (int)jy > 44 || (int)jx < -1 || (int)jx > 79)
		{
			jerry_dead();	
		}
	}
	return;
}

void move_walls(){
	for (int i = 0; i < 4; i++)
	{
		walls[i].x1 += walls[i].dx;
		walls[i].x2 += walls[i].dx;
		walls[i].y1 += walls[i].dy;
		walls[i].y2 += walls[i].dy;		
		react_to_walls('J');
		react_to_walls('T');
		// Branch of if's to separate into which direction each wall is travelling
		//-------------------- DX
		//If the wall is travlling to the right
		if (walls[i].dx > 0)
		{
			
			if (walls[i].x1 >= 84 && walls[i].x2 >= 84)
			{
				walls[i].x1 = walls[i].x1 - (84 + walls[i].deltax);
				walls[i].x2 = walls[i].x2 - (84 + walls[i].deltax);
			}
		}
		// If the wall is travlling to the left
		else if (walls[i].dx < 0)
		{
			
			if (walls[i].x1 <= 0 && walls[i].x2 <= 0)
			{
				walls[i].x1 = walls[i].x1 + 84 + walls[i].deltax;
				walls[i].x2 = walls[i].x2 + 84 + walls[i].deltax;
			}
		}
		
		//-------------------- DY
		// Is the wall travelling up
		if (walls[i].dy < 0)
		{
			
			if (walls[i].y1 <= 8 && walls[i].y2 <= 8)
			{
				walls[i].y1 = walls[i].y1 + 40 + walls[i].deltay;
				walls[i].y2 = walls[i].y2 + 40 + walls[i].deltay;
			}
		}
		// Is the wall travelling down
		else if (walls[i].dy > 0)
		{
			
			if (walls[i].y1 >= 48 && walls[i].y2 >= 48)
			{
				walls[i].y1 = walls[i].y1 - (41 + walls[i].deltay);
				walls[i].y2 = walls[i].y2 - (41 + walls[i].deltay);
			}
		}
		draw_line(walls[i].x1, walls[i].y1, walls[i].x2, walls[i].y2, 1);
	}
}

// Same as move walls but doesnt add directional movement, for use in paused mode and L 1.
void draw_walls(){
	for (int i = 0; i < 4; i++)
	{
		draw_line(walls[i].x1, walls[i].y1, walls[i].x2, walls[i].y2, 1);
	}
}	

// Setup Data Direction Registers, to enable input/output
void setupDDRS(){
	// Right Button
	CLEAR_BIT(DDRF, 5); // SW3 - Button Right
	CLEAR_BIT(DDRF, 6); // SW2 - Button Left
	SET_BIT(DDRB, 2); // LED 0
	SET_BIT(DDRB, 3); // LED 1
	SET_BIT(DDRD, 6); // LED 2
	CLEAR_BIT(DDRD, 1); // Joystick Up
	CLEAR_BIT(DDRB, 7); // Joystick Down
	CLEAR_BIT(DDRB, 1); // Joystick Left
	CLEAR_BIT(DDRD, 0); // Joystick Right
	CLEAR_BIT(DDRB, 0); // Joystick Centre
}

// Configuring the registers for timers
void setupTimers(){
	sei();
	//Timer for clock - Timer0
	TCCR0A = 0;
	TCCR0B = 5; //0b101	Every 1024 clock cycles == 7812.5 ticks/sec
	TIMSK0 = 1;

	// TCCR1A = 0;
	// TCCR1B = 1;
	// TIMSK1 = 1;
}
// Erasing any counted time.
void reset_timers(){
	gameTime = 0;
	timeCounter = 0;
}

void reset_cheese_positions(){
	for (int i = 0; i < 5; i++)
	{
		cheese_coords[i].x1 = -5;
		cheese_coords[i].y1 = -5;
		cheese_coords[i].onScreen = 0;
	}
}

void reset_trap_positions(){
	for (int i = 0; i < 5; i++)
	{
		trap_coords[i].x1 = -5;
		trap_coords[i].y1 = -5;
		trap_coords[i].onScreen = 0;
	}
}

void reset_char_positions(){
	tx = LCD_X - 6, ty = LCD_Y - 9, jx = 0, jy = 8;
	reset_dir_and_speed();
}

void reset_fireworks(){
	for (int i = 0; i < 20; i++)
	{
		firework_coords[i].x1 = -10;
		firework_coords[i].y1 = -10;
		firework_coords[i].onScreen = 0;
	}
}

void reset_variables(){
	reset_timers();
	reset_char_positions();
	reset_cheese_positions();
	reset_trap_positions();
	reset_fireworks();
}

void nextLevel(){
	levelNum = 2;	
	door.onScreen = 0;
	// Move Door of screen. 
	reset_cheese_positions();
	reset_trap_positions();
	reset_fireworks();
}

// Collision detection for on map items
void check_for_cheese(){
	for (int y = 0; y < 5; y++)
	{
		if (cheese_coords[y].onScreen == 1) // Proceed if cheese onscreen - Uses y coord for effeciency
		{
			for (int x = 0; x < 5; x++) // Looping over x coord of jerry
			{
				for (int i = 0; i < 5; i++) // Looping through each cheese in cheese_coords
				{
					for (int j = 0; j < 3; j++) // Lopping through cheeses y coordinate
					{
						for (int k = 0; k < 2; k++) // Looping through cheeses x coordinate
						{
							if (((int)jx+x == (cheese_coords[i].x1 + k)) && ((int)jy+y == (cheese_coords[i].y1 + j)) && (cheese_coords[i].onScreen == 1))
							{
								cheese_coords[i].onScreen = 0;
								jerryScore++;
								break;
							}
						}
					}
				}
			}
		}
	}
}

void check_for_traps(){
	for (int y = 0; y < 5; y++) //Looping through y coord of jerry
	{
		if (trap_coords[y].onScreen == 1) // Proceed if trap onscreen - Uses y coord for effeciency.
		{
			for (int x = 0; x < 5; x++) // Looping over x coord of jerry
			{
				for (int i = 0; i < 5; i++) // Looping through each trap in trap_coords
				{
					for (int j = 0; j < 3; j++) // Lopping through traps y coordinate
					{
						for (int k = 0; k < 3; k++) // Looping through traps x coordinate
						{
							if (((int)jx+x == (trap_coords[i].x1 + k)) && ((int)jy+y == (trap_coords[i].y1 + j)) && (trap_coords[i].onScreen == 1))
							{
								trap_coords[i].onScreen = 0;
								jerryLives--;
								break;
							}
						}
					}
				}
			}
		}
	}
}
void check_for_door(){
	if (door.onScreen == 1)
	{
		for (int y = 0; y < 5; y++) //Looping through y coord of jerry
		{
			for (int x = 0; x < 5; x++) // Looping over x coord of jerry
			{
				for (int j = 0; j < 4; j++) // Lopping through traps y coordinate
				{
					for (int k = 0; k < 3; k++) // Looping through traps x coordinate
					{
						if (((int)jx+x == (door.x1 + k)) && ((int)jy+y == (door.y1 + j)))
						{
							nextLevel();
							break;
						}
					}
				}
			}
		}
	}
	
	
}

// Start Game screen
void drawstartScreen(){
	draw_string(12, 4, "Tom & Jerry!", FG_COLOUR);
	draw_string(2, 20, "HIT SW3 TO START", 1);
	draw_string(15, 32, "James Scott", FG_COLOUR);
	draw_string(22, 41, "N9943618", FG_COLOUR);
}
	
void draw_jerry(){
	uint8_t jerryBitmap[5] = {
		0b10101,
		0b11111,
		0b00100,
		0b01110,
		0b01010
		};
	for (int i = 0; i < 5; i++){
		for (int j = 0; j < 5; j++){
			if (BIT_VALUE(jerryBitmap[i], (4-j)) == 1){
				draw_pixel((int)jx + j, (int)jy + i, 1);
			}
		}
	}
}

void draw_tom(){
	uint8_t tomBitmap[5] = {
		0b10011,
		0b10011,
		0b11110,
		0b01110,
		0b01010
		};
	for (int i = 0; i < 5; i++){
		for (int j = 0; j < 5; j++){
			if (BIT_VALUE(tomBitmap[i], (4-j)) == 1){
				draw_pixel(tx + j, ty + i, 1);
			}
		}
	}
}

void setup_tom(){
	reset_dir_and_speed();
}

// Checks for Tom/Jerry collision
int tom_jerry_iscollided(){
	int jerry_top = (int)(int)jy, jerry_bottom = (int)(int)jy+4, jerry_left = (int)(int)jx, jerry_right = (int)(int)jx+4;
	int tom_top = (int)ty, tom_bottom = (int)ty+4, tom_left = (int)tx, tom_right = (int)tx+4;
	if (jerry_top == tom_bottom || jerry_bottom == tom_top){
		if (jerry_left <= tom_right && jerry_right >= tom_left){
			SET_BIT(PORTD, 6);
			return 1;
		}
	}
	else if (jerry_left == tom_right || jerry_right == tom_left){
		if (jerry_top <= tom_bottom && jerry_bottom >= tom_top){
			SET_BIT(PORTD, 6);
			return 1;
		}
	}
	else {
		CLEAR_BIT(PORTD, 6);
		return 0;
	}
	return 0;
}

void change_tom_speed(){
	if(left_adc <= 2){
		charSpeed = 0.2;
	}
	else if (left_adc > 2 && left_adc <= 4)
	{
		charSpeed = 0.3;
	}
	else if (left_adc > 4 && left_adc <= 6)
	{
		charSpeed = 0.4;
	}
	else if (left_adc > 6 && left_adc <= 8)
	{
		charSpeed = 0.5;
	}
	else
	{
		charSpeed = 0.6;
	}
}

double jerrySpeed(){
	if(left_adc <= 2){
		return 0.4;
	}
	else if (left_adc > 2 && left_adc <= 4)
	{
		return 0.5;
	}
	else if (left_adc > 4 && left_adc <= 6)
	{
		return 0.6;
	}
	else if (left_adc > 6 && left_adc <= 8)
	{
		return 0.8;
	}
	else
	{
		return 1;
	}
}

void move_tom(){
	react_to_walls('T');
	if(tom_jerry_iscollided()){
		reset_char_positions();
		jerryLives--;
	}
	change_tom_speed();
	tdx = charSpeed * cos(direction);
	tdy = charSpeed * sin(direction);
	int new_x = round(tx + tdx), new_y = round(ty + tdy);
	uint8_t bounced = 0;

	if (new_x <= 0 || new_x > 79){
		bounced = 1;
	}
	if (new_y <= 8 || new_y > 43){
		bounced = 1;
	}
	if (bounced == 1){
		reset_dir_and_speed();
	}
	else{
		tx += tdx;
		ty += tdy;
	}
}

void jerry_movement(){
	double speedfactor = jerrySpeed();
	if(tom_jerry_iscollided()){
		tom_dead();
		jerry_dead();
	}
	else{
		if (JoystickUpState == 1 && (int)jy > 8){
			if (wall_collision_check((int)jx, (int)jy-1, 'U') == 0)
			{

				jy = jy - 1 * speedfactor;
			}
		}
		else if (JoystickDownState == 1 && (int)jy < 43){
			if (wall_collision_check((int)jx, (int)jy+1, 'D') == 0)
			{
				jy = jy + 1 * speedfactor;
			}
		}
		else if (JoystickLeftState == 1 && (int)jx > 0){
			if (wall_collision_check((int)jx-1, (int)jy, 'L') == 0)
			{
				jx = jx - 1 * speedfactor;
			}
		}
		else if (JoystickRightState == 1 && (int)jx < 79){
			if (wall_collision_check((int)jx+1, (int)jy, 'R') == 0)
			{
				jx = jx + 1 * speedfactor;
			}
		}
		check_for_cheese();
		check_for_traps();
	}
}

void statusBar(){
	// Spacing must be different for score sizes etc
	if (jerryScore < 10)
	{
		// Level
		draw_string(0, 0, "Lv", FG_COLOUR);
		draw_char(10, 0, ':', FG_COLOUR);
		draw_int(14, 0, levelNum, FG_COLOUR, 0);

		// Lives
		draw_string(21, 0, "HP", FG_COLOUR);
		draw_char(31, 0, ':', FG_COLOUR);
		draw_int(35, 0, jerryLives, FG_COLOUR, 0);

		// Score
		draw_string(42, 0, "S", FG_COLOUR);
		draw_char(48, 0, ':', FG_COLOUR);
		draw_int(52, 0, jerryScore, FG_COLOUR, 0);
	}
	else
	{
		// Level
		draw_string(0, 0, "L:", FG_COLOUR);
		draw_int(9, 0, levelNum, FG_COLOUR, 0);

		// Lives
		draw_string(17, 0, "H:", FG_COLOUR);
		draw_int(26, 0, jerryLives, FG_COLOUR, 0);

		// Score
		draw_string(34, 0, "S:", FG_COLOUR);
		draw_int(44, 0, jerryScore, FG_COLOUR, 0);
	}
	// Timer
	if (gameTime >= 60){
		elapsedMins++;  // Making this equal 0 so that it
		gameTime = 0;   // doesn't read 60 for a second
	}
	draw_int(60, 0, elapsedMins, FG_COLOUR, 1);
	draw_char(70, 0, ':', FG_COLOUR);
	draw_int(74, 0, (int)gameTime, FG_COLOUR, 1);
	
	// Bottom Bar
	draw_line(0, 7, 84, 7, FG_COLOUR);
}

// Map Spawning items
void create_cheese(){
	int x_rand = random_x(), y_rand = random_y();
	if (!check_occupied(x_rand, y_rand, 4, 5)){
		for (int i = 0; i < 5; i++)
		{
			if (cheese_coords[i].onScreen == 0)
			{
				cheese_coords[i].onScreen = 1;
				cheese_coords[i].x1 = x_rand;
				cheese_coords[i].y1 = y_rand;
				return;
			}
		}
	}
	else{
		create_cheese();
	}
}
void create_trap(){
	for (int i = 0; i < 5; i++)
	{
		if (trap_coords[i].onScreen == 0)
		{
			trap_coords[i].onScreen = 1;
			trap_coords[i].x1 = (int)tx;
			trap_coords[i].y1 = (int)ty;
			return;
		}
	}
}
void create_door(){
	int x_rand = random_x(), y_rand = random_y();
	if (!check_occupied(x_rand, y_rand, 4, 4)){
		door.x1 = x_rand;
		door.y1 = y_rand;
		door.onScreen = 1;
		return;
	}
	else{
		create_door();
	}
}
void spawn_cheese(){
	if ((int)gameTime + 2 == cheese_secs)
	{
		cheese_secs += 2;
		create_cheese();
		if (cheese_secs > 63){cheese_secs = 4;}
	}
	check_for_cheese();
}
void spawn_traps(){
	if ((int)gameTime + 3 == traps_secs)
	{
		traps_secs += 3;
		create_trap();
		if (traps_secs > 66){traps_secs = 6;}
	}
	check_for_traps();
}
void draw_cheese(int x, int y){
	uint8_t cheeseBitmap[3] = {
		0b11,
		0b10,
		0b11
		};
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < 2; j++){
			if (BIT_VALUE(cheeseBitmap[i], (1-j)) == 1){
				draw_pixel(x + j, y + i, 1);
			}
		}
	}
}
void draw_trap(int x, int y){
	uint8_t trapBitmap[3] = {
		0b101,
		0b010,
		0b101
		};
	for (int i = 0; i < 3; i++){
		for (int j = 0; j < 3; j++){
			if (BIT_VALUE(trapBitmap[i], (2-j)) == 1){
				draw_pixel(x + j, y + i, 1);
			}
		}
	}
}
void draw_door(){
	if (door.onScreen == 1)
	{
		uint8_t doorBitmap[4] = {
			0b111,
			0b101,
			0b101,
			0b111
			};
		for (int i = 0; i < 4; i++){
			for (int j = 0; j < 3; j++){
				if (BIT_VALUE(doorBitmap[i], (2-j)) == 1){
					draw_pixel(door.x1 + j, door.y1 + i, 1);
				}
			}
		}
	}
}
void draw_all_cheeses(){
	for (int i = 0; i < 5; i++)
	{
		if (cheese_coords[i].onScreen == 1)
		{
			draw_cheese(cheese_coords[i].x1, cheese_coords[i].y1);
		}
	}
}
void draw_all_traps(){
	for (int i = 0; i < 5; i++)
	{
		if (trap_coords[i].onScreen == 1)
		{
			
			draw_trap(trap_coords[i].x1, trap_coords[i].y1);
		}
	}
}

// FIREWORKS
items update_firework(items firework){
	// whether to left or right
	int next_x = 0, next_y = 0;
	if (firework.x1 > tx)
	{
		next_x = (firework.x1 - 1);
	}
	else if (firework.x1 < tx)
	{
		next_x = (firework.x1 + 1);
	}
	// whether to move up or down
	if (firework.y1 > ty)
	{
		next_y = (firework.y1 - 1);
	}
	else if (firework.y1 < ty)
	{
		next_y = (firework.y1 + 1);
	}
	// Is that pixel a wall
	if (IsPixelSet(next_x, next_y, wallBuffer))
	{
		firework.onScreen = 0;
	}
	else if ((next_x >= tx) && (next_x < (tx + 4)))
	{
		if ((next_y >= ty) && (next_y < (ty + 4)))
		{
		tom_dead();
		firework.x1 = next_x;
		firework.y1	= next_y;
		firework.onScreen = 0;
		}
	}
	else
	{
		firework.x1 = next_x;
		firework.y1	= next_y;
		firework.onScreen = 1;
	}
	return firework;
}
void shootFirework(){
	for (int i = 0; i < 20; i++)
	{
		if (firework_coords[i].onScreen == 0)
		{
			firework_coords[i].x1 = (int)jx;
			firework_coords[i].y1 = (int)jy;
			firework_coords[i].onScreen = 1;
			break;
		}
	}
}
void draw_fireworks(){
	for (int i = 0; i < 20; i++)
	{
		if (firework_coords[i].onScreen == 1)
		{
			draw_pixel(firework_coords[i].x1, firework_coords[i].y1, 1);
		}
		
		
	}
}
void move_fireworks(){
	for (int i = 0; i < 20; i++)
	{
		if (firework_coords[i].onScreen == 1){
			firework_coords[i] = update_firework(firework_coords[i]);
		}
	}
	
}
void fireworks(){
	if (JoystickCentreState == 0 && prevJoystickCentreState == 1)
	{
		shootFirework();
	}
	prevJoystickCentreState = JoystickCentreState;
	move_fireworks();
	draw_fireworks();
}
void updateADCs(){
	left_adc = adc_read(0) / 100;
	right_adc = adc_read(1) / 100;
}

// Timer functions
void calculate_timer(){
	gameTime = (timeCounter * 256.0 + TCNT0) * 1024.0 / 8000000.0;
	gameTime = gameTime - pausedTime;
	gameTime = gameTime - (60 * elapsedMins); // Subtracting minutes
}
void calculate_pause(){
	pausedTime = (timeCounter * 256.0 + TCNT0) * 1024.0 / 8000000.0;
	for (int i = 0; i < elapsedMins; i++) // Subtracting minutes
	{
		pausedTime = pausedTime - 60;
	}
	pausedTime = pausedTime - gameTime;
}

void nextLevelButton(){
	if (SW2State == 0 && prevSW2State == 1)
	{
		nextLevel();
	}
	prevSW2State = SW2State;
}

int startup(){
	if (SW3State == 0 && prevSW3State == 1){return 1;}
	prevSW3State = SW3State;
	return 0;
}

void paused_gameplay(){
	draw_walls();
	statusBar();
	// Calculating how long the game was paused for
	calculate_pause();
	updateADCs();

	// Drawing text across the screen to show game status
	draw_string(27, 20, "PAUSED", 1);
	draw_tom();
	draw_jerry();
	draw_all_cheeses();
	draw_all_traps();
	if (jerryScore > 4 && door.onScreen == 0 && levelNum == 1)
	{
		create_door();
	}
	draw_door();
	nextLevelButton();
	if (jerryScore > 2)
	{
		fireworks();
	}

	// PUT ALL DRAWING ABOVE THIS
	check_for_cheese();
	check_for_traps();
	check_for_door();
}

void normal_gameplay(){
	if (levelNum == 2)
	{
		move_walls();
	}
	else
	{
		draw_walls();
	}
	copyScreenBuffer(screen_buffer, wallBuffer);
	statusBar();
	// Only update timers if normal gameplay
	calculate_timer();
	updateADCs();
	move_tom();
	draw_tom();
	draw_jerry();
	draw_all_cheeses();
	draw_all_traps();
	if (jerryScore > 4 && door.onScreen == 0 && levelNum == 1)
	{
		create_door();
	}
	draw_door();
	nextLevelButton();
	if (jerryScore > 2)
	{
		fireworks();
	}
	// PUT ALL DRAWING ABOVE THIS
	spawn_cheese();
	spawn_traps();
	check_for_cheese();
	check_for_traps();
	check_for_door();
	// DEBUGGING DRAWING
	// SET_BIT(PORTB, 3);
	draw_int(5,20,left_adc,1,0);
	draw_int(5,30,right_adc,1,0);
	// draw_int(30,20,gameTime + 2 ,1,0);
	// draw_int(30,30,cheese_secs,1,0);
	
}

void pause_check(){
	if (SW3State == 0 && prevSW3State == 1)
	{
		if (paused == 0)
		{
			paused = 1;
		}
		else
		{
			paused = 0;
		}
	}
	prevSW3State = SW3State;
	if (paused)
	{
		paused_gameplay();
	}
	else
	{
		normal_gameplay();
	}
	return;
}

void setup() {
	set_clock_speed(CPU_8MHz);
	adc_init();
	lcd_init(LCD_DEFAULT_CONTRAST);
	lcd_clear();
	clear_screen();

	setupTimers();
	setupDDRS();

	create_walls();
	drawstartScreen();
	show_screen();	
}

void process() {
	clear_screen();
	pause_check();
	check_for_cheese();
	check_for_traps();
	jerry_movement();
	show_screen();
	}

// ----------------------------------------------------------

int main(void) {
	setup();
	// Wait until Right switch is pressed
	while(startup() == 0);
	// Seeding rand based on time spent on homescreen
	srand(timeCounter); 
	setup_tom();
	reset_variables();
	_delay_ms(10);
	while(1) {
		process();
	}
}

ISR(TIMER0_OVF_vect){
	timeCounter++;
	
	SW3Counter = SW3Counter << 1;
	SW2Counter = SW2Counter << 1;
	JoystickUpCounter = JoystickUpCounter << 1;
	JoystickDownCounter = JoystickDownCounter << 1;
	JoystickLeftCounter = JoystickLeftCounter << 1;
	JoystickRightCounter = JoystickRightCounter << 1;
	JoystickCentreCounter = JoystickCentreCounter << 1;

	// Different tolerances of mask for different buttons
	uint8_t low_reponse_mask = 0b00000111;
	uint8_t high_response_mask = 0b00000011;

	SW3Counter = SW3Counter & low_reponse_mask;
	SW2Counter = SW2Counter & low_reponse_mask;
	JoystickUpCounter = JoystickUpCounter & high_response_mask;
	JoystickDownCounter = JoystickDownCounter & high_response_mask;
	JoystickLeftCounter = JoystickLeftCounter & high_response_mask;
	JoystickRightCounter = JoystickRightCounter & high_response_mask;
	JoystickCentreCounter = JoystickCentreCounter & low_reponse_mask;

	SW3Counter = SW3Counter|BIT_IS_SET(PINF, 5);
	SW2Counter = SW2Counter|BIT_IS_SET(PINF, 6);
	JoystickUpCounter = JoystickUpCounter|BIT_IS_SET(PIND, 1);
	JoystickDownCounter = JoystickDownCounter|BIT_IS_SET(PINB, 7);
	JoystickLeftCounter = JoystickLeftCounter|BIT_IS_SET(PINB, 1);
	JoystickRightCounter = JoystickRightCounter|BIT_IS_SET(PIND, 0);
	JoystickCentreCounter = JoystickCentreCounter|BIT_IS_SET(PINB, 0);

	if(SW3Counter == 7){
		SW3State = 1;
	}
	else if(SW3Counter == 0){
		SW3State = 0;
	}
	if(SW2Counter == 7){
		SW2State = 1;
	}
	else if(SW2Counter == 0){
		SW2State = 0;
	}
	if(JoystickUpCounter == 3){
		JoystickUpState = 1;
	}
	else if(JoystickUpCounter == 0){
		JoystickUpState = 0;
	}
	if(JoystickDownCounter == 3){
		JoystickDownState = 1;
	}
	else if(JoystickDownCounter == 0){
		JoystickDownState = 0;
	}
	if(JoystickLeftCounter == 3){
		JoystickLeftState = 1;
	}
	else if(JoystickLeftCounter == 0){
		JoystickLeftState = 0;
	}
	if(JoystickRightCounter == 3){
		JoystickRightState = 1;
	}
	else if(JoystickRightCounter == 0){
		JoystickRightState = 0;
	}
	if(JoystickCentreCounter == 3){
		JoystickCentreState = 1;
	}
	else if(JoystickCentreCounter == 0){
		JoystickCentreState = 0;
	}
}
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

// Global Variables
double tx = LCD_X - 6, ty = LCD_Y - 9, tdx, tdy, speed = 0.4;
int jx = 0, jy = 8;
int levelNum = 1;
int jerryLives = 5;
int score = 0;
unsigned char paused = 1;
double pausedTime = 0;
double gameTime = 0;
uint8_t elapsedMins = 0;
uint8_t wallBuffer[504];


///Interrupts for timer and debouncing
volatile int timeCounter = 0;

volatile uint8_t SW3Counter = 0;
volatile uint8_t SW3State = 0;
int prevSW3State = 0;

volatile uint8_t SW2Counter = 0;
volatile uint8_t SW2State = 0;

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
// Global variables end

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

///Setup Data Direction Registers, to enable input/output
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

///Configuring the pins for timers
void setupTimers(){
	sei();
	//Timer for clock - Timer0
	TCCR0A = 0;
	TCCR0B = 5; //0b101	Every 1024 clock cycles == 7812.5 ticks/sec
	TIMSK0 = 1;
}
// Erasing any counted time.
void reset_timers(){
	gameTime = 0;
	timeCounter = 0;
}

void reset_variables(){
	reset_timers();
	
}

int IsBitSet( unsigned char byte, int index )
{
  int mask = 1<<index;
  return (byte & mask) != 0;
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



// Start Game screen
void startScreen(void){
	draw_string(12, 4, "Tom & Jerry!", FG_COLOUR);
	draw_string(15, 32, "James Scott", FG_COLOUR);
	draw_string(22, 41, "N9943618", FG_COLOUR);
}

void draw_walls(){
	draw_line(18, 15, 13, 25, FG_COLOUR);
	draw_line(25, 35, 25, 45, FG_COLOUR);
	draw_line(45, 10, 60, 10, FG_COLOUR);
	draw_line(58, 25, 72, 30, FG_COLOUR);
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
		for (int j = 0; j < 6; j++){
			if (BIT_VALUE(jerryBitmap[i], (5-j)) == 1){
				draw_pixel(jx + j, jy + i, 1);
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
		for (int j = 0; j < 6; j++){
			if (BIT_VALUE(tomBitmap[i], (5-j)) == 1){
				draw_pixel(tx + j, ty + i, 1);
			}
		}
	}
}

void reset_dir_and_speed(){
	double direction  = rand() * M_PI * 2 / RAND_MAX;
	tdx = speed * cos(direction);
	tdy = speed * sin(direction);
}

void setup_tom(){
	reset_dir_and_speed();
}

void move_tom(){
	int new_x = round(tx + tdx), new_y = round(ty + tdy);
	uint8_t bounced = 0;
	if (new_x < 0 || new_x > 78)
	{
		bounced = 1;
	}
	if (new_y <= 8 || new_y >= 44)
	{
		bounced = 1;
	}
	if (bounced == 1)
	{
		reset_dir_and_speed();
	}
	else
	{
		tx += tdx;
		ty += tdy;
	}
}

void jerry_movement(){
	uint8_t bank = 0;
	uint8_t byte = 0;
	uint8_t collided = 0;
	if (JoystickUpState == 1 && jy > 8)
	{
		bank = jy / 8;
		byte = jy % 8;
		
		for (int i = jx; i < jx+5; i++)
		{
			if(IsBitSet(wallBuffer[jx],0)){
				collided = 1;
			}
		}
		if (collided == 0)
		{
			jy = jy - 1;
		}
		//uint8_t temp = wallBuffer[index];

	}
	else if (JoystickDownState == 1 && jy < 43)
	{
		jy = jy + 1;
	}
	else if (JoystickLeftState == 1 && jx >= 0)
	{
		jx = jx - 1;
	}
	else if (JoystickRightState == 1 && jx < 78)
	{
		jx = jx + 1;
	}
	draw_int(10,35,bank,1,0);
	draw_int(10,25,byte,1,0);
}

void statusBar(){
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
	draw_int(52, 0, score, FG_COLOUR, 0);
	
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

// Copying screen buffer to walls buffer
void copyScreenBuffer(uint8_t screen_buffer[], uint8_t wallBuffer[]){
	for (int i = 0; i < 504; i++)
	{
		wallBuffer[i] = screen_buffer[i];
	}
}

int startup(){
	draw_string(2, 20, "HIT SW3 TO START", 1);
	if (SW3State == 0 && prevSW3State == 1){return 1;}
	prevSW3State = SW3State;
	return 0;
}

void paused_gameplay(){
	// Calculating how long the game was paused for
	pausedTime = (timeCounter * 256.0 + TCNT0) * 1024.0 / 8000000.0;
	for (int i = 0; i < elapsedMins; i++) // Subtracting minutes
	{
		pausedTime = pausedTime - 60;
	}
	pausedTime = pausedTime - gameTime;

	// Drawing text across the screen to show game status
	draw_string(27, 20, "PAUSED", 1);
}

void normal_gameplay(){
	// Only update timers if normal gameplay
	gameTime = (timeCounter * 256.0 + TCNT0) * 1024.0 / 8000000.0;
	gameTime = gameTime - pausedTime;
	for (int i = 0; i < elapsedMins; i++) // Subtracting minutes
	{
		gameTime = gameTime - 60;
	}
	move_tom();
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
	
	lcd_init(LCD_DEFAULT_CONTRAST);
	lcd_clear();
	clear_screen();

	setupTimers();
	setupDDRS();
	
	startScreen();
	show_screen();	
}

void process() {
	clear_screen();
	draw_walls();
	copyScreenBuffer(screen_buffer, wallBuffer);
	statusBar();
	// Take a copy of the screen buffer and use this for wall detection.
	// Take copy before anything but the status bar and walls have been drawn
	pause_check();
	
	jerry_movement();
	
	draw_jerry();
	draw_tom();

	show_screen();}

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
		_delay_ms(1);
	}
}
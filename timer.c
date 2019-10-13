#include <stdint.h>
#include <stdio.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include <cpu_speed.h>

#include <graphics.h>
#include <macros.h>
#include "lcd_model.h"

void setup(void) {
	set_clock_speed(CPU_8MHz);
	lcd_init(LCD_DEFAULT_CONTRAST);
	lcd_clear();

	//	(a) Initialise Timer 0 in normal mode so that it overflows 
	//	with a period of approximately 0.008 seconds.
	//	Hint: use the table you completed in a previous exercise.
    CLEAR_BIT(TCCR0B,WGM02);
    SET_BIT(TCCR0B,CS02);
    CLEAR_BIT(TCCR0B,CS01);
    CLEAR_BIT(TCCR0B,CS00);

	//	(b) Enable timer overflow interrupt for Timer 0.
    SET_BIT(TIMSK0, TOIE0);

	//	(c) Turn on interrupts.
    sei();

	//	(d) Enable the joystick left switch for digital input.
	CLEAR_BIT(DDRB, 1);

	//	(e) Display your student number, "n9934618", with nominal
	//	top-left corner at screen location (20,27).
    draw_string(20,27,"n9934618",FG_COLOUR);

	// Keep the next instruction intact.
	show_screen();
}

//	(f) Create a volatile global variable called switch_counter.
//	The variable should be an 8-bit unsigned integer. 
//	Initialise the variable to 0.
volatile uint8_t switch_counter = 0;

//	(g) Define a volatile global 8-bit unsigned global variable 
//	called switch_state which will store the current state of the switch.
volatile uint8_t switch_state;

//	(h) Define an interrupt service routine to process timer overflow
//	interrupts for Timer 0. Every time the interrupt service
//	routine is called, switch_counter should:
ISR(TIMER0_OVF_vect) {
    //(h.a) Left-shift switch_counter one place;
	switch_counter = switch_counter << 1;

    
    //(h.b) Bitwise AND with a mask in which the 3 bits on the right
    //are 1 and the others are 0.
    uint8_t mask = 0b00000111;
    switch_counter = switch_counter & mask;

//      (h.c) Use bitwise OR to add the current open/closed value of the 
//			joystick left switch to the history.
    switch_counter = switch_counter|BIT_IS_SET(PINB,1);

//		(h.d) If switch_counter is equal to the bit mask, then the switch has been 
//			observed 3 times in a row to be closed. Assign the value 1 to 
//			switch_state, indicating that the switch should now be considered to be
//			officially "closed".    
    if(switch_counter == 7){
        switch_state = 1;
    }

//		(h.e) If switch_counter is equal to 0, then the switch has been observed 
//			to be open at least 3 in a row, so store 0 in switch_state, 
//			indicating that the switch should now be considered to be officially "open".
    else if(switch_counter == 0){
        switch_state = 0;
    }


}
//		
//		
//			
//		


// -------------------------------------------------
// Test driver.
// -------------------------------------------------

void process(void) {
	static uint8_t prevState = 0;
	if ( switch_state != prevState ) {
		prevState = switch_state;
		draw_string( 30, 40, prevState ? "closed" : "open  ", FG_COLOUR);
		show_screen();
	}
}

int main(void) {
	setup();

	for ( ;; ) {
		process();
	}
}
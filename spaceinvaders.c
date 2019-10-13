//invaders

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>


#include "lcd.h"
#include "graphics.h"
#include "cpu_speed.h"


#include "small_graphics.h"
#include "ascii_font_small.h"
#include "macros.h"
#include "sprite.h"

#define FREQUENCY 8000000.0
#define PRESCALER_0 1024.0
#define PRESCALER_1 1024.0
#define N_ALIENS 15

//--------------------------------------------------
//--------------------------------------------------

//Function Dec

void init_hardware(void);

//--------------------------------------------------

//Globals

int lives;
int score;
int counter;
int over;
volatile int level;



Sprite ship;
volatile int vol_shipx;
volatile int debouncedR;
volatile int debouncedL;
volatile int gamestart =0;


Sprite bullet;
volatile int vbullet;
volatile int vbulletorigin;
int bulletorigin = LCD_Y-6;
double bulx;
double buly;
int bulvis;

Sprite alien[N_ALIENS];
int alienx = 0;
int alieny = 0;
int backwards = 0;
int dy = 2;
double alien_x[N_ALIENS];
double alien_y[N_ALIENS];
int alienvis[N_ALIENS];
int aliensleft = N_ALIENS;

Sprite bomb[N_ALIENS];
int bomb_x[N_ALIENS];
int bomb_y[N_ALIENS];
int bomb_launched[N_ALIENS];
int bomb_origin[N_ALIENS];
int selectalien;
int bomb_x[N_ALIENS];
int bomb_y[N_ALIENS];

int pixels1[8][8];
int pixels2[8][8];
int pixels3[8][8];


volatile int vcounter1=0;
int loopmanip =0;



//--------------------------------------------------

//Alien struct
/*
typedef struct Game {                  
			  
	Sprite aliens[N_ALIENS];			  
			  
} Game;				
*/
//--------------------------------------------------


unsigned char ship_bm[9] = {
    0b00001100, 0b00110000, 
    0b00110101,	0b10101100,
    0b00111111, 0b11111100,


};

unsigned char alien_bm[4] = {
    0b00111100, 
    0b11011011, 
    0b10100101, 
    
 


};


unsigned char bullet_bm[3] = {
    0b10000000,
    0b10000000,
    0b10000000


};


//-----------------------------------------

void setup_game(){
	lives = 3;
	score =0;
	over = 0;
	level =1;
}

void draw_game(){
	

	draw_string_small(0,0, "S: ");
	draw_string_small(20,0, "L: ");
	draw_string_small(39,0, "LVL: ");

	draw_int_small(9,0,score);
	draw_int_small(29,0,lives);
	draw_int_small(59,0,level);

	draw_line(0,7,LCD_X,7);

	if (gamestart == 0){
		draw_string_small(5,LCD_Y/2+5, "SW3: Select LVL ");
		draw_string_small(5,LCD_Y/2-5, "SW2: Start Game ");
	}
}

//---------------------------------------

void setup_ship(){

	init_sprite(&ship, LCD_X/2-8, LCD_Y-4, 16, 3, ship_bm);
	//draw_sprite(&ship);
	ship.x = LCD_X/2-8;
	vol_shipx = ship.x;
	

}

void draw_ship(Sprite * ship){
	draw_sprite(ship);
}

void update_ship(Sprite * ship){
	if (debouncedR == 1){
		if (gamestart == 1)
		{
			vol_shipx = vol_shipx+1;
			ship->x = vol_shipx;
			debouncedR = 0;
		}

		


	}

	if (debouncedL == 1){
		if (gamestart == 1)
		{
			vol_shipx = vol_shipx-1;
			ship->x = vol_shipx;
			debouncedL = 0;
		}

		
	}
	
	if( ship->x <1){
		ship->x = 2;
		vol_shipx = ship->x;
	}
	if( ship->x >LCD_X-15){
		ship->x = LCD_X-16;
		vol_shipx = ship->x;

	}
}

//---------------------------------------
/*
void launch_bullet(){
	if (vbullet==1)
	{
		int initialx = vol_shipx+8;
		for(int i = bulletorigin; i<=6;i--){
			set_pixel(initialx,i,1);
			set_pixel(initialx,i+1,0);
			bullety = i;
			
			if (bullety <= 6)
			{
				bullety = bulletorigin;
				vbullet =0;
			}
		}
		
	}
	
	//draw_sprite(&ship);

}
*/

//---------------------------------------

void setup_bullet(Sprite * ship){
	
	init_sprite(&bullet, vol_shipx+8, bulletorigin, 1, 3, bullet_bm);
	bullet.is_visible = 0;

	
}

void draw_bullet(Sprite * bullet){
	draw_sprite(bullet);
}

// bullet finished I think


void move_bullet(Sprite * bullet){

	if (vbullet == 1){


		if (vbulletorigin == 1 && vbullet ==0)
		{
			bullet->x = vol_shipx + 8;
			vbulletorigin = 0;
		}
		
		
		
		
		bullet->is_visible = 1;
		bullet->y = bullet->y-1;
		bulx = bullet->x;
		buly = bullet->y;

		
	}
	if (bullet->y < 10 || vbullet == 0){
		
		bullet->x=vol_shipx+8;
		bullet->is_visible = 0;
		vbullet = 0;
		bullet->x = vol_shipx +8;
		bullet->y = bulletorigin;
		bulx = bullet->x;
		buly = bullet->y;
		

	}
	
	
//------------------------------------	

}

void setup_bomb(){
	for (int i =0; i<N_ALIENS; i++){
		init_sprite(&bomb[i], 0, 0, 1, 3, bullet_bm); //////// PROBLEM WITH CO-ORDS
		bomb_launched[i] = 0;
		bomb_y[i] = bomb[i].y;
		bomb_x[i] = bomb[i].x;
	}
}


void draw_bomb(Sprite * bomb){
		for (int i = 0; i<N_ALIENS; i++){
			if (alienvis[i] == 1 && bomb_launched[i] ==1)
			{
				draw_sprite(&bomb[i]);
			}
				
	}
}


void move_bomb(Sprite * bomb){


	

    	

    
    
	for (int i =0; i<N_ALIENS; i++){
		
		if(alienvis[i] == 0 && bomb_launched[i] == 0){
			bomb_launched[i] = 0;
		}


		if (bomb_launched[i] == 1){
			bomb[i].is_visible = 1;

			bomb[i].y = bomb[i].y+1;
			bomb_y[i] = bomb[i].y;
			
		}
		
		
		
		if (bomb[i].y > LCD_Y || bomb_launched[i] == 0){
			bomb[i].y = alien_y[i];
			bomb[i].x = alien_x[i]+4;
			bomb_launched[i] = 0;
			bomb[i].is_visible =0;
			bomb_y[i] = bomb[i].y;
			bomb_x[i] = bomb[i].x;

		}
		if (alienvis[i] == 0 && bomb[i].y > LCD_Y){
			bomb_launched[i]=0;
		}
	}
	
	if (loopmanip >= aliensleft*2)
	{
    	selectalien = rand() % 14;
    	
    	if (alienvis[selectalien] == 1)
    		{
    			bomb_launched[selectalien] = 1;	
    		}	
    			
    	loopmanip = 0;
    	

    }
	

}
//---------------------------------------

void bulletcollision(){
	
	if(vbullet == 1)
	{
		for (int i = 0; i<N_ALIENS; i++){
		
			if (alienvis[i] == 1)
			{
				if (alien_y[i] >= buly-4 && alien_y[i] <= buly)
				{

					if (alien_x[i] >= bulx -8 &&  alien_x[i] <= bulx){
						vbullet =0;
						alienvis[i] = 0;

						score = score +1;
						aliensleft = aliensleft-1;
					}
				}
			}
		
		}

		if (level == 2)
		{


			for (int i = 0; i < 8; i++)
			{
				if (bulx == LCD_X/4+i-3)
					
				{
					for (int j=0; j<8; j++)
					{
						if (buly <  LCD_Y-11-j )
						{
							if (pixels1[i][j]==0)
							{
								
							}
							else{
								vbullet = 0;
								pixels1[i][j]=0;
							}
							
							
						}
					}
				}

				if (bulx == LCD_X/4*2+i-3)
					
				{
					for (int j=0; j<8; j++)
					{
						if (buly <  LCD_Y-11-j )
						{
							if (pixels2[i][j]==0)
							{
								
							}
							else{
								vbullet = 0;
								pixels2[i][j]=0;
							}
							
							
						}
					}
				}

				if (bulx == LCD_X/4*3+i-3)
					
				{
					for (int j=0; j<8; j++)
					{
						if (buly <  LCD_Y-11-j )
						{
							if (pixels3[i][j]==0)
							{
								
							}
							else{
								vbullet = 0;
								pixels3[i][j]=0;
							}
							
							
						}
					}
				}
			}
		}
	}
}
					
					
					
					

	
	


void bombcollision(){
	
	for (int i = 0; i<N_ALIENS; i++){
		if(bomb_launched[i] == 1){
		
				if (alienvis[i] == 0 && bomb_launched ==0)
						{
							bomb_launched[i] = 0;
						}
			
			
				if (bomb_y[i] >= LCD_Y-6 && bomb_y[i]<= LCD_Y-3)
				{

					if (bomb_x[i] >= vol_shipx+2 &&  bomb_x[i] <= vol_shipx+13){
						bomb_launched[i] = 0;
						lives = lives - 1;
						
						
					}
				}
			if (level == 2)
			{


				for (int p = 0; p < 8; p++)
				{
					if (bomb_x[i] == LCD_X/4+p-3)
					
					{
						for (int j=0; j<8; j++)
						{
							if (bomb_y[i] >  LCD_Y-13-j )
							{
								if (pixels1[p][j]==0)
								{
								
								}
								else{
									bomb_launched[i]= 0;
									pixels1[p][j]=0;
								}
							
							
							}
						}
					}

					if (bomb_x[i] == LCD_X/4*2+p-3)
					
					{
						for (int j=0; j<8; j++)
						{
							if (bomb_y[i] >  LCD_Y-13-j )
							{
								if (pixels2[p][j]==0)
								{
								
								}
								else{
									bomb_launched[i]= 0;
									pixels2[p][j]=0;
								}
							
							
							}
						}
					}

					if (bomb_x[i] == LCD_X/4*3+p-3)
					
					{
						for (int j=0; j<8; j++)
						{
							if (bomb_y[i] >  LCD_Y-13-j )
							{
								if (pixels3[p][j]==0)
								{
								
								}
								else{
									bomb_launched[i]= 0;
									pixels3[p][j]=0;
								}
							
							
							}
						}
					}
				}
			}
			
		
		}


		
	}
	
}

//---------------------------------------

void setup_walls(){
	for (int i = 0; i < 8; i++)
	{
		for (int j=0; j<8;j++)
		{
			pixels1[i][j]=1;
			pixels2[i][j]=1;
			pixels3[i][j]=1;
		}
	}
	

}





void draw_walls(){
	if (level == 2){


		for (int i = 0; i < 8; i++)
		{
			for (int j=0; j<8;j++)
			{
				if (pixels1[i][j] == 1){
					set_pixel(LCD_X/4+i-3,LCD_Y-11-j,1);
				}
				if (pixels2[i][j] == 1){
					set_pixel(LCD_X/4*2+i-3,LCD_Y-11-j,1);
				
				}
				if (pixels3[i][j] == 1){
					set_pixel(LCD_X/4*3+i-3,LCD_Y-11-j,1);
				
				}
				for  (int p = 0; p<N_ALIENS;p++){
					if (pixels1[i][j] == 0 && vbullet == 0 && bomb_launched[p] == 0){
						set_pixel(LCD_X/4+i-3,LCD_Y-11-j,0);
					}
					if (pixels2[i][j] == 0 && vbullet == 0 && bomb_launched[p] == 0){
						set_pixel(LCD_X/4*2+i-3,LCD_Y-11-j,0);
				
					}
					if (pixels3[i][j] == 0 && vbullet == 0 && bomb_launched[p] == 0){
						set_pixel(LCD_X/4*3+i-3,LCD_Y-11-j,0);
					


				
					}	
				}
				
			}
	
		}
	}
}
//---------------------------------------

void setup_aliens(){
	alienx = LCD_X-10;
	alieny = 10;
	for ( int i = 0; i < N_ALIENS/3; i++ ) {
		init_sprite(&alien[i], alienx,alieny,8,3,alien_bm);
		alienx = alienx - 9;
		alien_x[i] = alien[i].x;
		alien_y[i] = alien[i].y;
		alienvis[i] = 1;
	}
	alieny = alieny + 5;
	alienx = LCD_X-10;
	for (int i = 5; i<N_ALIENS/3*2; i++){
		
		init_sprite(&alien[i], alienx,alieny,8,3,alien_bm);
		
		alienx = alienx - 9;
		alien_x[i] = alien[i].x;
		alien_y[i] = alien[i].y;
		alienvis[i] = 1;

	}
	alienx=LCD_X-10;
	alieny = alieny + 5;
	for (int i = 10; i<N_ALIENS; i++){
		init_sprite(&alien[i], alienx,alieny,8,3,alien_bm);
		alienx = alienx -9;
		alien_x[i] = alien[i].x;
		alien_y[i] = alien[i].y;
		alienvis[i] = 1;
		
	}
	
	



}

void draw_aliens(Sprite * alien){
	for (int i = 0; i<N_ALIENS; i++){
		draw_sprite(&alien[i]);
	}

		

}

void move_aliens(Sprite * alien){
	
	///use is visible bools

	if (alien[14].x <= 0){
			
			backwards = 1;

			

	}
	if (alien[14].x >= LCD_X/2-3)
	{
			
			backwards = 0;
			
	}

	for (int i = 0; i<N_ALIENS; i++){
		
		if (level == 2){ ///// CHANGE LEVEL TO 2 WHEN MORE CODE COMPLETE
			

			if (alien[14].y >= LCD_Y/2+2){
				dy = -2;
			}
			if (alien[14].y <= 20){
				dy = 2;
			}	



			if (alien[14].x <= 0){
				alien[i].y = alien[i].y +dy;
				alien_y[i] = alien[i].y;
			}
			if (alien[14].x >= LCD_X/2-3){
				alien[i].y = alien[i].y +dy;
				alien_y[i] = alien[i].y;
			}
			

		}

		if (backwards == 0)
		{
			
			alien[i].x = alien[i].x - 1;
			alien_x[i] = alien[i].x;

		}
		if (backwards == 1){
			
			alien[i].x = alien[i].x + 1;
			alien_x[i] = alien[i].x;
		
		}
		
		if (alienvis[i] == 0){
			alien[i].is_visible = 0;
			
		}
		
		
		
	}
}



//---------------------------------------




int main() {

    set_clock_speed(CPU_8MHz);
    init_hardware();
    setup_game();
    setup_ship();
    draw_ship(&ship);
    setup_bullet(&ship);
    draw_bullet(&bullet);
    setup_aliens(alien);
    draw_aliens(alien);
    setup_bomb(bomb);
    draw_bomb(bomb);
    setup_walls();


    while(1)
    {
    	if (gamestart==0){
    		clear_screen();
    		draw_game();
    		show_screen();
    		if (debouncedR == 1){
    			level = level +1;
    			debouncedR = 0;
    		}
    		if (level >2){
    			level = 1;
    		}
    	}
    	else
    	{

    	
    		if (vcounter1 >8)
    			{
    				move_aliens(alien);
    			

    			
    				vcounter1 = 0;
    			}

    			if(vcounter1 % 5 == 0){

    				//move_bullet(&bullet);
    				move_bomb(bomb);
    			
    			
    			}
    			if(vcounter1 % 3 == 0){
    				//move_bomb(bomb);
    				move_bullet(&bullet);
    			}

    		if (TCNT2 >=1)
    		{
    		
    			clear_screen();
    			draw_game();
    			update_ship(&ship);
    			draw_aliens(alien);
    		
    			draw_bomb(bomb);
    			bulletcollision();
    			bombcollision();
    		
    		
    		

    			draw_ship(&ship);
    		
    			draw_bullet(&bullet);
    			draw_walls();
    		


    			//launch_bullet();
    		
    		
    			show_screen();
    			//_delay_ms(5);
    			TCNT2 = 0;
    			loopmanip++;
    			if (lives == 0 || score == 15){
    				over = 1;
    			}
    		
    		}
    		if (over == 1){
    			clear_screen();
    			draw_string_small(16,LCD_Y/2, "GAME over ");
    			show_screen();
    			_delay_ms(8000);
    			gamestart = 0;
    			lives =3;
    			score = 0;
    			setup_ship();
				setup_bullet(&ship);
				setup_aliens(alien);
				setup_bomb(bomb);
				setup_walls();
				aliensleft = N_ALIENS;
    			over = 0;
    			

    		}
    	
    
        
		}
	}
	return 0;
}





void init_hardware(void) {
    // Initialising the LCD screen
    lcd_init(LCD_DEFAULT_CONTRAST);

    // Initalising the push buttons as inputs
    DDRF &= ~((1 << PF5) | (1 << PF6));

    // Initialising the LEDs as outputs
    DDRB |= ((1 << PB2) | (1 << PB3));

    // Initialise the USB serial
    //usb_init();


    // SW1 are connected to pins D1 and D0. Configure all pins as Input(0)
	DDRD &= ~(1<<PD1);

    // Setup two timers with overflow interrupts:
    // 0 - button press checking @ ~30Hz
    // 1 - system time overflowing every 8.3s
    TCCR0B |= ((1 << CS02) | (1 << CS00));
    //TCCR1B |= ((1 << CS12) | (1 << CS10));
    TIMSK0 |= (1 << TOIE0);
    //TIMSK1 |= (1 << TOIE1);

    TCCR2B |= (1 << CS02)|(1 << CS00);


    // Enable interrupts globally
    sei();
}

ISR(TIMER0_OVF_vect) {
    // De-bounced button press checking
    if ((PINF>>PF5) & 1) {
        
    	//show_screen();
        _delay_ms(30);

        if((PINF>>PF5) & 1){
        	if (gamestart ==1)
          	{
           		debouncedR = 1;
            
            
            	draw_ship(&ship);
            	//show_screen();
           }
            else{
            	debouncedR = 1;
            	clear_screen();
            	draw_game();
            }
            
        }
    }
    if ((PINF>>PF6) & 1) {
        //show_screen();

        _delay_ms(30);

        if((PINF>>PF6) & 1){
        	if (gamestart ==1)
          	{
           		debouncedL = 1;
            
            
            	draw_ship(&ship);
            	//show_screen();
           }
            else 
            {
            	debouncedL=1;
            	gamestart = 1;
            }
            
            
        }
    }
    	
    	
    	
    	//draw_ship(&ship);
 	
    if((PIND>>PD1)&1){
    	_delay_ms(30);

    	if((PIND>>PD1)&1)
    	{
    		//setup_bullet();

    		vbullet = 1;
    		vbulletorigin =1;

    		//draw_string_small(15,15, "LVL: "); //test rubbish
    	}
    }

    vcounter1 = vcounter1+1; //goign to modulo with bullet to update movement ie only move when modulo 5 is true
        
        
}
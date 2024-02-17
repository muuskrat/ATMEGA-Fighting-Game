/*	Author: lab
 *  Partner(s) Name: Mario Orlando
 *	Lab Section:
 *	Assignment: Final # part 1.   
 *	https://youtu.be/vsrzIAXufrM
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../header/timer.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "io.h"
//#include "anolog.h"
#endif



unsigned char heart[8] = { 0x00, 0x0A, 0x15, 0x11, 0x0A, 0x04, 0x00, 0x00 };
unsigned char idleAni[8] = { 0x00, 0x06, 0x06, 0x04, 0x04, 0x04, 0x0A, 0x0A };
unsigned char rightAni[8] = { 0x00, 0x06, 0x06, 0x04, 0x07, 0x04, 0x0A, 0x12 };
unsigned char leftAni[8] = { 0x00, 0x18, 0x18, 0x10, 0x10, 0x18, 0x14, 0x10 };
unsigned char upAni[8] = { 0x06, 0x06, 0x08, 0x08, 0x06, 0x0D, 0x00, 0x00 };
unsigned char downAni[8] = { 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x12, 0x0D };

unsigned char EidleAni[8] = { 0x02, 0x1E, 0x1C, 0x06, 0x06, 0x06, 0x0A, 0x0A };
unsigned char EattAni[8] = { 0x02, 0x1E, 0x1C, 0x06, 0x1E, 0x06, 0x0A, 0x0A };

unsigned int xAxis;
unsigned int yAxis;

enum SoundSm {soundStart, soundNeutral, hitP, hitE, dodgeHitP, dodgeFailE, dodgeP, charge} sound;
unsigned char sing = 0x00;

enum MenuSM {menuStart, play, br, highscore, winnar, lose, brPlay, brHigh} menu;
unsigned char win = 0x00;
unsigned char hs = 0x00;
unsigned char op = 0x00;
unsigned char ref = 0x00;
unsigned char round = 0x00;

enum EnemySM {enemyStart, Eidle, Eattack, loadAttack, EattackHold, Ehurt, perfectE} enemy;

unsigned char cntE = 0x00;
unsigned char diff = 9;
unsigned char attackTimer = 10;
unsigned char hurtTimer = 6;
unsigned char enemyReset = 27;
unsigned char enemyPos = 27;
unsigned char eHealth = 9;

enum PlayerSM {playerStart, idle, left, up, down, right, hurt, PattackHold, perfectP} player;

unsigned char cntP = 0x00;
unsigned char playerReset = 22;
unsigned char playerPos = 22;
unsigned char pHealth = 9;

unsigned char incomingL = 0;
unsigned char incomingU= 0;
unsigned char incomingD = 0;


void set_PWM(double frequency) {
	static double current_frequency;
	  if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		
    	else { TCCR3B |= 0x03;}
    	
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
    
	    else if (frequency > 31250) { OCR3A = 0x0000; }
	    
	    else { OCR3A = (short) (8000000 / (128 * frequency)) - 1; }
    
    TCNT3 = 0;
    current_frequency = frequency;
  }
}



void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}


void LCD_Custom_Char (unsigned char loc, unsigned char *msg)
{
    unsigned char i;
    if(loc < 8){
     LCD_WriteCommand(0x40 + (loc * 8)); 
   
      for(i = 0; i < 8; i++)  
          LCD_WriteData(msg[i]);      
    }   
}



void ADC_Init()
{
	ADCSRA = 0x87;		
	ADMUX = 0x40;		
}

int ADC_Read(char channel)
{
	int ADC_value;
	
	ADMUX = (0x40) | (channel & 0x07);
	ADCSRA |= (1<< ADSC);	
	while((ADCSRA &(1<<ADIF))== 0);	
	
	ADCSRA |= (1 << ADIF);	
	ADC_value = (int)ADCL;	
	ADC_value = ADC_value + (int)ADCH*256; 

	return ADC_value;		
}

void randomize(){
	attackTimer = rand() % (20 + 1 - 3) + 3;
	
	incomingL = rand() % (2);
	incomingU = rand() % (2);
	incomingD = rand() % (2);
	
	if((incomingL == 0) && (incomingU == 0) && (incomingD == 0))
		randomize();
	
	if((incomingL == 1) && (incomingU == 1) && (incomingD == 1))
		randomize();
}

//enum MenuSM {menuStart, play, highscore, win, lose} menu;
//enum SoundSm {soundStart, soundNeutral, hitP, hitE, dodgeHitP, dodgeFailE, dodgeP, charge} sound;

void Tick_Sound(){
	switch(sound){
		case soundStart:
			sound = soundNeutral;
			break;
			
		case soundNeutral:
    			if(enemy == Ehurt)
					sound = hitP;
				else if(enemy == charge)
					sound = charge;
				else if(player == hurt)
					sound = hitE;
				else if((player == up) || (player == down) || (player == left))
					sound = dodgeP;
				else if((player == perfectP))
					sound = dodgeHitP;
				else
					sound = soundNeutral;	
			break;

		case hitP:
			if(enemy != Ehurt)
				sound = soundNeutral;
			else
				sound = hitP;
			
			break;
		
		case hitE:
			if(player != hurt)
				sound = soundNeutral;
			else
				sound = hitE;
			
			break;
		
		case dodgeHitP:
			if(enemy != hurt)
				sound = soundNeutral;
			else
				sound = dodgeHitP;	
			break;
		
		case dodgeFailE:
			if(enemy != hurt)
				sound = soundNeutral;
			else
				sound = dodgeHitP;
			break;
		
		case dodgeP:
			if(player == hurt)
				sound = dodgeFailE;
			else if(player == perfectP)
				sound = dodgeHitP;
			else if(player != idle)
				sound = dodgeP;
			else
				sound = soundNeutral;
			break;
		
		case charge:
			
			break;
	}

	switch(sound){
		case menuStart:
			set_PWM(0);
			break;
			
		case soundNeutral:
			set_PWM(0);
			break;
			
		case hitP:
			set_PWM(440.00);
			break;
			
		case hitE:
			set_PWM(261.63);
			break;
			
		case dodgeHitP:
			set_PWM(523.25);
			break;
				
		case dodgeFailE:
			set_PWM(130.8);
			break;
			
		case dodgeP:
			set_PWM(349.23);
			break;
			
		case charge:
			set_PWM(174.6);
			break;
	
			 
	}
}












void Tick_Menu(){
	switch(menu){
		case menuStart:
			menu = br;
			break;
			
		case br:
			pHealth = 9;
			eHealth = 9;
			if((xAxis >= 500 && xAxis <= 600) && (yAxis >= 500 && yAxis <= 600)){
				menu = br; 
			}
			else if(xAxis > 600)
    				menu = brPlay;//left;
    			else if(xAxis < 500)
    				menu = brHigh;//right;
    			
			break;
		case brPlay:
			op++;
			if(xAxis > 600){
				if(op == 15){
					op = 0;
					
					LCD_ClearScreen();
					
					LCD_Cursor(1);
					LCD_WriteData(5);
					LCD_WriteData(eHealth + '0');
					
					LCD_Cursor(15);
					LCD_WriteData(5);
					LCD_WriteData(pHealth + '0');
					menu = play;
				}
			}
			else{
				op = 0;
				menu = br;
			}
			break;
			
		case brHigh:
			op++;
			if(xAxis < 500){
				if(op == 15){
					op = 0;
					menu = highscore;
				}
			}
			else{
				op = 0;
				menu = br;
			}
			break;
		
		case play:
			if(eHealth == 0){
				round++;
				menu = winnar;
			}
			if(pHealth == 0)
				menu = lose;
			break;
			
		case highscore:
			if(!((xAxis >= 500 && xAxis <= 600) || (yAxis >= 500 && yAxis <= 600))){
				menu = br;
			}
			else
				menu = highscore;
			break;
			
		case winnar:
			if((xAxis >= 500 && xAxis <= 600) && (yAxis >= 500 && yAxis <= 600)){
				menu = winnar;
			}
			else{
				eHealth = 9;
				
				LCD_ClearScreen();
				LCD_Cursor(1);
				LCD_WriteData(5);
				LCD_WriteData(pHealth + '0');
					
				LCD_Cursor(15);
				LCD_WriteData(5);
				LCD_WriteData(eHealth + '0');
				
				menu = play;
				
			}
			break;
		
		case lose:
			if((xAxis >= 500 && xAxis <= 600) || (yAxis >= 500 && yAxis <= 600)){
				menu = lose;
			}
			else{
				if(round > hs)
					hs = round;
				menu = br;
			}
			break;
	}

	switch(menu){
		case menuStart:
			break;
			
		case br:
			ref++;
			if((ref % 5) == 0)
				LCD_DisplayString(1, "PLAY            HIGHSCORE     ");
			break;
		case brPlay:
			ref++;
			if((ref % 5) == 0)
				LCD_DisplayString(1, "||PLAY||        HIGHSCORE     ");
			break;
		case brHigh:
			ref++;
			if((ref % 5) == 0)
				LCD_DisplayString(1, "PLAY            ||HIGHSCORE||     ");
				
		case play:
			break;
			
		case highscore:
			ref++;
			if((ref % 5) == 0){
				LCD_DisplayString(1, "Highscore");
				LCD_Cursor(25);
				LCD_WriteData(hs + '0');
			}
			break;
			
		case winnar:
			ref++;
			if((ref % 5) == 0){
				LCD_ClearScreen();
				LCD_DisplayString(1, "Round: ");
				LCD_Cursor(8);
				LCD_WriteData(round + '0');
			}
			break;
		
		case lose:
			ref++;
			if((ref % 5) == 0){
				LCD_ClearScreen();
				LCD_DisplayString(1, "You lose :( ");
			}
			break;
			 
	}
}



void Tick_player(){
	LCD_Custom_Char(0, idleAni);
	LCD_Custom_Char(1, rightAni);
	LCD_Custom_Char(2, leftAni);
	LCD_Custom_Char(3, upAni);
	LCD_Custom_Char(4, downAni);

	

	switch(player){
		case playerStart:
			player = idle;
			break;
			
		case idle:
		cntP = 0;
		if((xAxis >= 500 && xAxis <= 600) && (yAxis >= 500 && yAxis <= 600)){
			if(enemy == EattackHold){
				pHealth--;
				cntP = 0;
				player = hurt;
			}
				
			else
				player = idle; 
		}
   	 	else if(xAxis > 600)
   	 		player = up;
    		else if(xAxis < 500)
    			player = down;
    		else if(yAxis > 600)
    			player = left;
    		else if(yAxis < 500)
    			player = right;
    		else if(enemy == EattackHold){
				pHealth--;
				player = hurt;
			}
    		break;
			
		case right:
			//cntP++;
			playerPos++;
			if(enemy == Eattack){
				cntP = 0;
				pHealth--;
				player = hurt;
			}
			else if(playerPos < 26){
				playerPos--;
				LCD_Cursor(playerPos--);
				LCD_WriteData(' ');
				playerPos++; playerPos++;
				player = right;
			}
			else if(playerPos >= 26){
				playerPos--;
				LCD_Cursor(playerPos--);
				LCD_WriteData(' ');
				playerPos++;playerPos++;
				cntP = 0;
				player = PattackHold;
			}
			break;
			
		case PattackHold:
			cntP++;
			if(cntP >= 3){
				cntP = 0;
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				playerPos = playerReset;
				player = idle;
				
			}
			else		
				player = PattackHold;
			break;
			
		case left:
			if(cntP == 0)
				playerPos--;
			if(cntP == 1){
				LCD_Cursor(playerReset);
				LCD_WriteData(' ');
			}
			
			if((yAxis < 500) && (enemy == EattackHold)){ //(cntP <= 9) && 
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				playerPos = playerReset;
				cntP = 0;
				player = perfectP;
				break;
			}
				
			cntP++;
			if((incomingL == 1) && (enemy == EattackHold)){
				LCD_Cursor(playerReset);
				LCD_WriteData(' ');
				pHealth--;
				pHealth--;
				cntP = 0;
				player = hurt;
			}
			else if(cntP >= 5){
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				player = idle;
			}
			else
				player = left;
			break;
			
		case up:
			if(cntP == 0)
				playerPos = playerPos - 16;
			if(cntP == 1){
				LCD_Cursor(playerReset);
				LCD_WriteData(' ');
			}
			
			if((yAxis < 500) && (enemy == EattackHold)){ //(cntP <= 9) && 
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				playerPos = playerReset;
				cntP = 0;
				player = perfectP;
				break;
			}
			cntP++;
			if((incomingU == 1) && (enemy == EattackHold)){
				LCD_Cursor(playerReset);
				LCD_WriteData(' ');
				pHealth--;
				pHealth--;
				cntP = 0;
				player = hurt;
			}
			else if(cntP >= 5){
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				player = idle;
			}
			else
				player = up;
			break;
			
		case down:
		
			if((yAxis < 500) && (enemy == EattackHold)){ //(cntP <= 9) && 
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				playerPos = playerReset;
				cntP = 0;
				player = perfectP;
				break;
			}
			
			cntP++;
			if((incomingD == 1) && (enemy == EattackHold)){
				pHealth--;
				pHealth--;
				cntP = 0;
				player = hurt;
			}
			else if(cntP >= 5)
				player = idle;
			else
				player = down;
			break;
			
		case hurt:
			cntP++;
			if(cntP >= 3){
				LCD_Cursor(playerPos);
				LCD_WriteData(' ');
				playerPos = playerReset;
				player = idle;
				}
			else{
				player = hurt;
			}
			break;
			
		case perfectP:
			cntP++;
			if(cntP <= 3){
				player = perfectP;
			}
			else{
				cntP = 0;
				player = idle;
				}
			break;
			

	}

	switch(player){
	
		case playerStart:
			LCD_Cursor(playerPos);
			LCD_WriteData(0);
			break;
			
		case idle:
			playerPos = playerReset;
			LCD_Cursor(playerReset);
			LCD_WriteData(0);
			break;
			
		case right:
			LCD_WriteData(' ');
			LCD_Cursor(playerPos);
			LCD_WriteData(1);
			break;
			
		case left:
			LCD_Cursor(playerPos);
			LCD_WriteData(2);
			break;
			
		case up:
			LCD_Cursor(playerPos);
			LCD_WriteData(3);
			break;
			
		case down:
			LCD_Cursor(playerPos);
			LCD_WriteData(4);
			break;
			
		case PattackHold:
			LCD_Cursor(playerPos);
			LCD_WriteData(1);
			break;
		
		case hurt:
			LCD_Cursor(2);
			LCD_WriteData(pHealth + '0');
			LCD_Cursor(playerPos);
			LCD_WriteData(5);
			break;
			
		case perfectP:
			LCD_Cursor(playerPos);
			LCD_WriteData(1);
			break;
	}
}



void Tick_enemy(){
	LCD_Custom_Char(6, EidleAni);
	LCD_Custom_Char(7, EattAni);
	

	switch(enemy){
		case enemyStart:
			enemy = Eidle;
			break;
			
		case Eidle:
			cntE++;
			if(cntE >= attackTimer){
				cntE = 0;
				enemy = loadAttack;
				enemyPos = enemyReset;
				LCD_Cursor(enemyReset);
				LCD_WriteData(' ');
				randomize();
				enemyPos++;
			}
			else if(player == PattackHold){
				eHealth--;
				enemy = Ehurt;
				}
			else
				enemy = Eidle;
			break;
			
		case loadAttack:
			cntE++;
			if(cntE >= diff){
				cntE = 0;
				enemy = Eattack;
			}
			else
				enemy = loadAttack;
			//randomAttack();
			break;
			
		case Eattack:
			enemyPos--;
			if(enemyPos != 23){
				LCD_Cursor(enemyPos);
				LCD_WriteData(' ');
				enemy = Eattack;
			}
			else if(enemyPos <= 23){
				LCD_Cursor(enemyPos);
				LCD_WriteData(' ');
				LCD_Cursor(24);
				LCD_WriteData(' ');
				cntE == 0;
				enemy = EattackHold;
			}
			break;


		case EattackHold:
			if(player == perfectP){
				cntE = 0;
				eHealth--;
				enemy = perfectE;	
				break;
			}
			cntE++;
			if(cntE >= 3){
				cntE = 0;
				LCD_Cursor(enemyPos);
				LCD_WriteData(' ');
				enemyPos = enemyReset;
				LCD_Cursor(8);
				LCD_WriteData(' ');
				LCD_WriteData(' ');
				LCD_WriteData(' ');
				enemy = idle;
				
			}
			else		
				enemy = EattackHold;
			break;
						
		case Ehurt:
			cntE++;
			hurtTimer++;
			if(hurtTimer >= 3){
				hurtTimer = 0;
				enemy = idle;
			}
			else{
				enemy = Ehurt;
			}
			break;
		
		case perfectE:
			cntE = 0;
			hurtTimer++;
			if(hurtTimer >= 3){
				LCD_Cursor(enemyPos);
				LCD_WriteData(' ');
				enemyPos = enemyReset;
				hurtTimer = 0;
				enemy = idle;
			}
			else{

				enemy = perfectE;
			}
			break;
		

	}

	switch(enemy){
	
		case enemyStart:
			LCD_Cursor(enemyPos);
			LCD_WriteData(6);
			break;
			
		case Eidle:
			LCD_Cursor(enemyReset);
			LCD_WriteData(6);
			break;
			
		case loadAttack:
			LCD_Cursor(enemyPos);
			LCD_WriteData(7);
			break;
			
		case Eattack:			
			LCD_WriteData(' ');
			LCD_Cursor(enemyPos);
			LCD_WriteData(7);
			
			LCD_Cursor(8);
			LCD_WriteData(' ');
			LCD_WriteData(' ');
			LCD_Cursor(8);
			if(incomingL == 1){
				LCD_WriteData('<' + 0);	
			}
			if(incomingU == 1){
				LCD_WriteData('^' + 0);			
			}
			if(incomingD == 1){
				LCD_WriteData('v' + 0);		
			}
			break;
			
		case EattackHold:
			LCD_Cursor(enemyPos);
			LCD_WriteData(7);
			break;
			
		case Ehurt:
			LCD_Cursor(16);
			LCD_WriteData(eHealth + '0');
			LCD_Cursor(enemyPos);
			LCD_WriteData(5);
			break;
			
		case perfectE:
			LCD_Cursor(16);
			LCD_WriteData(eHealth + '0');
			LCD_Cursor(enemyPos);
			LCD_WriteData(5);
			break;
			
	}
}
  


int main(void) {
    /* Insert DDR and PORT initializations */
   	DDRA = 0x00; PORTA = 0xFF;
   	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00; 
    /* Insert your solution below */


	
    TimerSet(50);
	TimerOn();
	
	LCD_init();
	ADC_Init();
	PWM_on();
	//LCD_DisplayString(1, "Moshi Moshi");
	
	//LCD_Custom_Char(0, idle1);
	//LCD_Custom_Char(1, attack);	
	
	
	LCD_Custom_Char(5, heart);
	


	 
	
	//LCD_Cursor(2);
	//LCD_DisplayString(1, "9 A# D#      ");  
	
	
	
	
	
    while (1) {
    	
    	//LCD_ClearScreen();
    	//LCD_WriteData(val + '0');
    	//LCD_DisplayString(1, );
   	
    	
    	xAxis = ADC_Read(0);
    	yAxis = ADC_Read(1);	
    	
    	Tick_Menu();
    	if(menu == play){
    		Tick_Sound();
    		Tick_player();
    		Tick_enemy();
	}

    	while(!TimerFlag);
			TimerFlag = 0;

    		

	}
    return 1;
    
}















/*
    	if((xAxis >= 500 && xAxis <= 600) && (yAxis >= 500 && yAxis <= 600)){
    		LCD_Cursor(1);
			LCD_WriteData(0);
    	}
    	else if(xAxis > 600)
    		LCD_DisplayString(1, "Up");
    	else if(xAxis < 500)
    		LCD_DisplayString(1, "Down");
    	else if(yAxis > 600)
    		LCD_DisplayString(1, "Left");
    	else if(yAxis < 500){
    		LCD_Cursor(1);
		LCD_WriteData(1);
		//LCD_DisplayString(1, "Right");
    	}
    		
    	else
    		LCD_DisplayString(1, "Moshi Moshi");
*/


/*
 	  	
    	if(val >= 500 && val <= 600)
    		LCD_DisplayString(1, "idle");
    	else if(val > 600)
    		LCD_DisplayString(1, "hello");
    	else if(val < 500)
    		LCD_DisplayString(1, "yahallo");
    	else
    		LCD_DisplayString(1, "Moshi Moshi");

*/


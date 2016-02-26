/* mipslabwork.c
   For copyright and licensing, see file COPYING */

#include <stdint.h>
#include <pic32mx.h>
#include "mipslab.h"
#include <math.h>

#define BASE_FREQUENCY 440
#define FREQUENCY_SCALING 1.059463094

int calculate_baudrate_divider(int sysclk, int baudrate, int highspeed);
void timerSetup();
void pollUart();
void uartSetup();
void user_isr();
void labinit();
void labwork();
void display_stuff();
void update_frequency();
void handleMIDIInterrupt();
void handleTimerInterrupt();

unsigned char message1 = 0;
unsigned char message2 = 0;
unsigned char message3 = 0;

char state = 0;

int note = 0;
int gate = 0;
double frequency = BASE_FREQUENCY;

/* Interrupt Service Routine */
void user_isr( void ) {
	if ((IFS(0) >> 27) & 0x1){	// 0b10000000000000
		handleMIDIInterrupt();
	}
	if ((IFS(0) >> 8) & 0x1) {
		handleTimerInterrupt();
	}
	return;
}

void handleMIDIInterrupt(){

	switch (state){
		case 0:
			message1 = U1RXREG;
			unsigned char noteOnOff = message1 >> 4;
			if ((noteOnOff == 9) || (noteOnOff == 10))
				state = 1;
			else
				state = 0;
			break;
		case 1:
			message2 = U1RXREG;
			state = 2;
			break;
		case 2:
			message3 = U1RXREG;
			if ((message3 > 0)){
				gate = 1;
				note = message2 - 81;
			}else{
				gate = 0;
			}
			state = 0;
			break;
		default:
			state = 0;
			gate = 0;
			break;
	}




/*	switch (state){*/
/*		case 0:*/
/*			message1 = U1RXREG;*/
/*			if (message1 != 0xfe)*/
/*				state = 1;*/
/*			break;*/
/*		case 1:*/
/*			message2 = U1RXREG;*/
/*			if (message2 != 0xfe)*/
/*				state = 2;*/
/*			else*/
/*				state = 0;*/
/*			break;*/
/*		case 2:*/
/*			message3 = U1RXREG;*/
/*			if (message3 != 0xfe){*/
/*				unsigned char noteOnOff = message1 >> 4;*/
/*				if ((noteOnOff == 9) && (message3 > 0)){*/
/*					gate = 1;*/
/*					note = message2 - 81;*/
/*				}else{*/
/*					gate = 0;*/
/*				}*/
/*			}*/
/*			state = 0;*/
/*			break;*/
/*		default:*/
/*			state = 0;*/
/*			gate = 0;*/
/*			break;*/
/*	}*/
	IFS(0) &= ~(1 << 27);
	return;
}
void handleTimerInterrupt(){
	if (gate)
		PORTE = ~(PORTE);
	else
		PORTE = 0;
	IFS(0) &= ~(1 << 8);
	return;
}

void timerSetup(){
	T2CON = 0x70;		// Prescaling för timer pbc = 10mHz -> 10000000 / 245 = 39062 Hz
	TMR2 = 0;		// Nollställ timer
	PR2 = 312500;		// Vad timern ska räkna till
	T2CON |= (1 << 15);	// Starta timer
}

void init_interrupt(){
	IEC(0) |= 0x100;
	IPC(2) = 7 << 2;
	
	IEC(0) |= (1 << 27);
	IPC(6) |= 31;
	timerSetup();
	enable_interrupt();
	return;
}

/* Lab-specific initialization goes here */
void labinit( void )
{
	TRISE = 0;
	PORTE = 0;
	uartSetup();
	init_interrupt();
	
	return;
}

/* This function is called repetitively from the main program */
void main_loop( void ) {
	//display_stuff();
	update_frequency();
	
	return;
}

void update_frequency(){
	frequency = BASE_FREQUENCY;
	double freqScaling = FREQUENCY_SCALING;
	int i;
	if (note < 0){
		int positiveNote = note * -1;
		for (i = 0; i < positiveNote; i++){
			frequency = frequency / FREQUENCY_SCALING;
		}
	}else{
		for (i = 0; i < note; i++){
			frequency = frequency * FREQUENCY_SCALING;
		}
	}
	PR2 = (312500/frequency);
	
	return;
}

void display_stuff(){
	char string1[] = "        ";
	char string2[] = "        ";
	//char string3[] = "        ";
	
	string1[0] = 48 + note;
	string2[0] = 48 + gate;
	
	//PORTE = (int)frequency;

	display_string(0, "MIDI");
	display_string(1, string1);
	display_string(2, string2);
	//display_string(3, string3);
	

	display_update();
	return;
}

int calculate_baudrate_divider(int sysclk, int baudrate, int highspeed) {
	int pbclk, uxbrg, divmult;
	unsigned int pbdiv;
	
	divmult = (highspeed) ? 4 : 16;
	/* Periphial Bus Clock is divided by PBDIV in OSCCON */
	pbdiv = (OSCCON & 0x180000) >> 19;
	pbclk = sysclk >> pbdiv;
	
	/* Multiply by two, this way we can round the divider up if needed */
	uxbrg = ((pbclk * 2) / (divmult * baudrate)) - 2;
	/* We'll get closer if we round up */
	if (uxbrg & 1)
		uxbrg >>= 1, uxbrg++;
	else
		uxbrg >>= 1;
	return uxbrg;
}

void uartSetup(){
	U1STA = 0;
	U1MODE = 0;
	
	OSCCON &= ~0x180000;
	OSCCON |= 0x080000;
	U1BRG = calculate_baudrate_divider(80000000, 31250, 0);
	
	/* 8-bit data, no parity, 1 stop bit */
	U1MODE = 0x8000;
	/* Enable transmit and recieve */
	U1STASET = 0x1400;	
	return;	
}

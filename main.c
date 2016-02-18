#include <pic32mx.h>

void uartSetup();
void pollUart();

int tmp = 0;

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

int main() {
	TRISE = 0;
	PORTE = 0;
	uartSetup(); 
	pollUart();
	return 0;
}

void pollUart(){
	int message1 = 0;
	int message2 = 0;
	int message3 = 0;
	
	//while (1){
	
		U1STA &= ~1;
		while(!(U1STA & 0x1)); //wait for read buffer to have a value
		message1 = U1RXREG & 0xFF;
		
		
/*		while(!(U1STA & 0x1)); //wait for read buffer to have a value*/
/*		message2 = (U1RXREG & 0xFF);*/
/*		*/
/*		U1STA &= ~3;*/
/*		while(!(U1STA & 0x1)); //wait for read buffer to have a value*/
/*		message3 = (U1RXREG & 0xFF);*/
/*		U1STA &= ~3;*/
/*		*/
/*		PORTE = message1 & 0xff;*/
/*		delay(400);*/
/*		PORTE = message2 & 0xff;*/
/*		delay(400);*/
/*		PORTE = message3 & 0xff;*/
/*		delay(400);*/

		//if ((message1 >= 128) && (message1 < 144)){
			PORTE = message1;
		//	break;
		//}
			
		
		
	//}
}


void uartSetup(){
	U1STA = 0;
	U1MODE = 0;
	//OSCCON = 0;
	//OSCCON |= (0x3 << 19);	// PBCLOCK (OSCCON<20:19>) = SYSCLK / 8 = 10 MHz
	//U1BRG = 159;		// Baud-rate 31.25 kHz
	
	OSCCON &= ~0x180000;
	OSCCON |= 0x080000;
	U1BRG = calculate_baudrate_divider(80000000, 31250, 0);
	
	//U1STA |= (1 << 12);	// Enable UART Receiver
	//U1MODE |= (1 << 15);	// Enable UART1
	
	/* 8-bit data, no parity, 1 stop bit */
	U1MODE = 0x8000;
	/* Enable transmit and recieve */
	U1STASET = 0x1400;	
	
}

int dataTransferIsComplete(){
	return (U1STA & 1);
}



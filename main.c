#include <pic32mx.h>

int main() {
	pollUart();
	return 0;
}

void pollUart(){
	while (1){
		while (!dataTransferIsComplete());
		// Take action
		//U1RXREG
	}
}


void uartSetup(){
	OSCCON |= (0x2 << 19);	// PBCLOCK (OSCCON<20:19>) = SYSCLK / 8 = 10 MHz
	U1BRG = 19;		// Baud-rate 31.25 kHz
	U1MODE = 0;
	U1STA |= (1 << 12);	// Enable UART Receiver
	U1MODE |= (1 << 15);	// Enable UART1
}

int dataTransferIsComplete(){
	return (U1STA & 1);
}

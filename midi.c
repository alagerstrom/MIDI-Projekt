#include <stdint.h>
#include <pic32mx.h>

#define BASE_FREQUENCY 440
#define FREQUENCY_SCALING 1.059463094
#define MAX_KEYS_PRESSED 20
#define ZERO_PITCH 8192
#define ARPEGGIO_COUNT_TO_VALUE 1000

void timerSetup(void);
void pollUart(void);
void uartSetup(void);
void userIsr(void);
void midiInit(void);
void initInterrupt(void);
int main(void);
void uglyArpeggio(void);
void uglyVibrato(void);
void addKey(int key);
void removeKey(int key);
void timerSetup(void);
void arpeggioTimerSetup(void);
void updateFrequency(void);
void handleMIDIInterrupt(void);
void handleTimerInterrupt(void);

unsigned int tempMessage = 0;
unsigned int message1 = 0;
unsigned int message2 = 0;
unsigned int message3 = 0;
unsigned int statusByte = 0;

char state = 0;

int note = 0;
int gate = 0;
double frequency = BASE_FREQUENCY;
unsigned char keys[MAX_KEYS_PRESSED];
double actualFrequency = BASE_FREQUENCY;

int portamento = 0;
int pitchBend = 8192;

int arpeggioPosition = 0;
int arpeggioCounter = 0;
int arpeggioEnable = 0;
int arpeggioSpeed = 0;

int vibratoAmount = 0;
int vibratoAmountSetting = 0;
int vibratoCounter = 0;
int vibratoDirectionUp = 0;
int vibratoLength = 1000;

/* ------------------------------------------------------------------*/

int main(void){	
	midiInit();
	while(1){
		uglyVibrato();
		updateFrequency();
		if (arpeggioEnable)
			uglyArpeggio();
		else
			arpeggioPosition = 0;
	}
	return 0;
}

/*-------------------------------------------------------------------*/


void midiInit(void){	
	int i;
	for (i = 0; i < MAX_KEYS_PRESSED; i++)
		keys[i] = 255;
		
	//pin 4 RF1
	TRISF &= ~(1 << 1); // Pin 4 till output
	PORTF &= ~(1 << 1); // Pin 4 till 0
		
	TRISE = 0;
	PORTE = 0;
	uartSetup();
	initInterrupt();
	timerSetup();
	enable_interrupt();
	
	return;
}
void uartSetup(void){
	U1STA = 0;
	U1MODE = 0;
	OSCCON &= ~0x180000;
	OSCCON |= 0x080000;
	U1BRG = 159;
	U1MODE = 0x8000;
	U1STASET = 0x1000;	
	return;	
}



void initInterrupt(void){
	IEC(0) |= 0x100;	// Enable timer interrupt
	IPC(2) = 7 << 2;
	
	IEC(0) |= (1 << 27);	// Enable UART interrupt
	IPC(6) |= 31;
		
	
	return;
}

void timerSetup(void){
	T2CON = 0x70;		// Prescaling för timer sysclk = 80mHz -> 80000000 / 256 = 312500 Hz
	TMR2 = 0;		// Nollställ timer
	PR2 = 1000;		// Vad timern ska räkna till
	T2CON |= (1 << 15);	// Starta timer
	return;
}




// init finished
/*------------------------------------------------------------------------*/


void userIsr(void){
	if ((IFS(0) >> 27) & 0x1){	
		IFS(0) &= ~(1 << 27);
		handleMIDIInterrupt();
	}
	if ((IFS(0) >> 8) & 0x1) {
		IFS(0) &= ~(1 << 8);
		handleTimerInterrupt();
	}
	return;
}


void handleMIDIInterrupt(void){
	
	//kolla om byten är status eller data
	tempMessage = U1RXREG;
	
	
	if (tempMessage >= 128){
		//hantera status byte
		message1 = tempMessage;
		statusByte = message1 >> 4;
		state = 0;

	}else{
		//hantera data byte
		switch (state){
			case 0:
				//tonhöjd
				message2 = tempMessage;
				state = 1;
				break;
			case 1:
				//velocity
				message3 = tempMessage;
				if (statusByte == 9){
					if ((message3 > 0)){
						addKey(message2);
						vibratoAmount = message3;
					}else{
						removeKey(message2);
					}					
				}
				
				if (statusByte == 0xB){
					// MOD WHEEL PORTAMENTO
					if (message2 == 1){
						portamento = message3;
					}	
					
					// ARPEGGIO CONTROL 7
					if (message2 == 7){
						arpeggioSpeed = (127 - message3);
						if (message3 == 0)
							arpeggioEnable = 0;
						else
							arpeggioEnable = 1;
					}
					// VIBRATO AMOUNT SETTING CONTROL 2
					if (message2 == 2){
						vibratoAmountSetting = message3;
					}		
				}
				if (statusByte == 0xE){
					pitchBend = (message3 << 7);
					pitchBend |= message2;
				}
				
				state = 0;

				break;
			default:
				state = 0;
				gate = 0;
				break;
		}
	}
	return;
}
void handleTimerInterrupt(void){
	if (gate){
		if ((PORTF >> 1) & 0x1)
			PORTF &= ~(1 << 1);
		else
			PORTF |= 1 << 1;
	}else{
		PORTF &= ~(1 << 1);
	}
	return;
}




void addKey(int key){
	int i;
	int alreadyExists = 0;
	for (i = 0; i < MAX_KEYS_PRESSED; i++){
		if (key == keys[i])
			alreadyExists = 1;
	}
	if (!alreadyExists){
		for (i = MAX_KEYS_PRESSED - 1; i > 0; i--){
			keys[i] = keys[i - 1];
		}
		keys[0] = key;
	}
	return;
}

void removeKey(int key){
	int i;
	for (i = 0; i < MAX_KEYS_PRESSED; i++){
		if (key == keys[i])
			keys[i] = 255;
	}
	for (i = 0; i < MAX_KEYS_PRESSED - 1; i++){
		if (keys[i] == 255){
			int temp = keys[i];
			keys[i] = keys[i + 1];
			keys[i + 1] = temp;
			
		}
	}
	return;
}


void uglyArpeggio(void){
	arpeggioCounter++;
	if (arpeggioCounter > (ARPEGGIO_COUNT_TO_VALUE + (double)arpeggioSpeed / 15 * ARPEGGIO_COUNT_TO_VALUE)){
		arpeggioCounter = 0;
		arpeggioPosition++;
		if (((arpeggioPosition + 1) > MAX_KEYS_PRESSED) || (keys[arpeggioPosition] > 128))
			arpeggioPosition = 0;
		PORTE = (1 << arpeggioPosition);
	}
	return;
}

void uglyVibrato(void){
	if (vibratoDirectionUp){
		vibratoCounter++;
		if (vibratoCounter >= vibratoLength)
			vibratoDirectionUp = 0;
	}else{
		vibratoCounter--;
		if (vibratoCounter <= (vibratoLength * -1))
			vibratoDirectionUp = 1;
	}	
	
	return;
}



void updateFrequency(void){
	frequency = BASE_FREQUENCY;
	
	if (keys[arpeggioPosition] < 128){
		note = keys[arpeggioPosition] - 81;
		gate = 1;
	}else{
		gate = 0;
	}
	
	int i;
	if (note < 0){
		int positiveNote = note * -1;
		for (i = 0; i < positiveNote; i++){
			frequency = (double)frequency/FREQUENCY_SCALING;
		}
	}else{
		for (i = 0; i < note; i++){
			frequency = (double)frequency*FREQUENCY_SCALING;
		}
	}
	
	// PORTAMENTO
	if (actualFrequency < frequency)
		actualFrequency = actualFrequency + ((frequency - actualFrequency) / ((portamento * 20) + 1));
	else if (actualFrequency > frequency)
		actualFrequency = actualFrequency - ((actualFrequency - frequency) / ((portamento * 20) + 1));
	
	
	
	// PITCH BEND
	double pitchScale = 1;
	if (pitchBend < ZERO_PITCH)
		pitchScale = (double)ZERO_PITCH/((2 * ZERO_PITCH - 1) - pitchBend);
	else if (pitchBend > ZERO_PITCH)
		pitchScale = (double)pitchBend/ZERO_PITCH;

	double pitchedFrequency = actualFrequency * pitchScale;
	
	
	// VIBRATO
	double vibratoFrequency = pitchedFrequency + ((vibratoCounter *
			(double)vibratoAmount/127) / 10) * (double)vibratoAmountSetting/127;
	
	
	PR2 = (312500/vibratoFrequency);
	if (TMR2 >= PR2)
		TMR2 = 0;
	return;
}



#asm
.equ __w1_port=0x1b
.equ __w1_bit=0
#endasm
#include <1wire.h>

#include <tiny2313.h>
#include <delay.h>

#include "ds18b20.h"

#define A   1
#define B   2
#define C   4
#define D   8
#define E  16
#define F  32
#define G  64

//didits pins
#define DIGIT_1  1
#define DIGIT_2  2
#define DIGIT_3  4

static flash unsigned char digit[] = {
	(A+B+C+D+E+F),   // 0
	(B+C),// 1
	(A+B+D+E+G),// 2
	(A+B+C+D+G),// 3
	(B+C+F+G),// 4
	(A+C+D+F+G),// 5
	(A+C+D+E+F+G),// 6
	(A+B+C),// 7
	(A+B+C+D+E+F+G),// 8
	(A+B+C+D+F+G),// 9
	(A+B+C+E+F+G),// A - 10
	(C+D+E+F+G),// b - 11
	(A+D+E+F),// C - 12
	(B+C+D+E+G),// d - 13
	(A+D+E+F+G),// E - 14
	(A+E+F+G),// F - 15
	(G),// 16 - minus
	(A+B+F+G),// 17 - grad?
	(0),// 18 - blank
	(C+E+G),// n
	(D+E+F+G),// t
	(A),// upper
	(D)// lower
};

static flash unsigned char commonPins[] = {
	0b11111111 ^ DIGIT_1,
	0b11111111 ^ DIGIT_2,
	0b11111111 ^ DIGIT_3
};

// maximum number of DS1820/DS18S20/DS18B20 devices connected to the 1 Wire bus
#define MAX_DS18b20 4

#define SPACE 18
#define SYMBOL_F 15
#define ZERO 0
#define HALF 5

#define DP_PIN PORTB.7
#define LED_BLUE PORTD.4
#define LED_RED PORTD.5
#define LED_GREEN PORTD.3

ds18b20_temperature_data_struct temperature;
unsigned char ds18b20_devices;
unsigned char rom_code[MAX_DS18b20][9];

unsigned char digit_out[3], cur_dig;

bit showMinus;
unsigned char currentSensor;

interrupt [TIM0_OVF] void timer0_ovf_isr(void) {
	PORTD|=0b00000111;
	PORTB=0b00000000;
	PORTB=digit[digit_out[cur_dig]];

	if (cur_dig == 1) {
		DP_PIN = 1;
	}
	PORTD &= commonPins[cur_dig];
	cur_dig++;
	if (cur_dig > 2) {
		cur_dig = 0;
	}
	LED_RED = ~showMinus;
	LED_BLUE = showMinus;
}

void view_term(void) {
	unsigned char decades;

	showMinus = temperature.minusChar;
	decades = temperature.temperatureIntValue / 10;
	digit_out[0] = decades > 0 ? decades : SPACE;
	digit_out[1] = temperature.temperatureIntValue % 10;
	digit_out[2] = temperature.halfDegree ? HALF : ZERO;
}
void main(void) {
	unsigned char i;

// Input/Output Ports initialization
// Port A initialization
// Func2=In Func1=In Func0=In 
// State2=T State1=T State0=T 
	PORTA = 0x00;
	DDRA = 0x00;

// Port B initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTB = 0x00;
	DDRB = 0xFF;

// Port D initialization
// Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State6=T State5=T State4=T State3=T State2=T State1=T State0=T 

	PORTD = 0x00;
	DDRD = 0xFF;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 62,500 kHz
// Mode: Normal top=FFh
// OC0A output: Disconnected
// OC0B output: Disconnected
	TCCR0A = 0x00;
	TCCR0B = 0x03;
	TCNT0 = 0x00;
	OCR0A = 0x00;
	OCR0B = 0x00;

// Timer/Counter 1 initialization
// Clock value: Timer1 Stopped
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TCNT1H = 0x00;
	TCNT1L = 0x00;
	ICR1H = 0x00;
	ICR1L = 0x00;
	OCR1AH = 0x00;
	OCR1AL = 0x00;
	OCR1BH = 0x00;
	OCR1BL = 0x00;

// External Interrupt(s) initialization
// INT0, INT1, Interrupt on any change on pins PCINT0-7: Off
	GIMSK = 0x00;
	MCUCR = 0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
	TIMSK = 0x02;

// Universal Serial Interface initialization
// Mode: Disabled
	USICR = 0x00;

// Analog Comparator initialization
// Analog Comparator: Off
	ACSR = 0x80;


 	w1_init();
 	ds18b20_devices = w1_search(0xf0, rom_code);

 #asm("sei")

 	//skip first values
 	if (ds18b20_devices >= 0) {
 		for (i = 0; i < ds18b20_devices; i++) {
 			ds18b20_temperature(&rom_code[i][0]);
 		}
 	}

 	digit_out[0] = SYMBOL_F;
 	digit_out[1] = SPACE;
 	digit_out[2] = ds18b20_devices;

	 while (1) {

	 	if (ds18b20_devices >= 1) {
            LED_GREEN = 1;
	 		temperature = ds18b20_temperature_struct(&rom_code[currentSensor][0]);
            LED_GREEN = 0;
	 		view_term();
	 		currentSensor++;
	 		if (currentSensor >= ds18b20_devices) {
	 			currentSensor = 0;
	 		}
	 		delay_ms(2000);
	 	};
	 }
}

/*
 * Aufgabe_02.c
 *
 * Created: 03.01.2025 16:31:21
 * Author : Felix
 */ 

#define F_CPU 16000000UL
#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)
#define STABILIZATION_DELAY_MS 5


#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>


typedef struct analog {
	uint16_t adc;
	uint32_t volts;
	uint32_t value;
} analog;



// ---------------------------------------------------------------------------
// UART
// ---------------------------------------------------------------------------
void uart_init(){
	// set UBRR0H and UBRR0L
	UBRR0H = (BAUD_CONST >> 8);
	UBRR0L = BAUD_CONST;
	// Frame-Format: 8 Databits, 1 Stopbit, no Parity
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	// Enable RX and TX
	UCSR0B = (1 << TXEN0);
}

void uart_putchar(char c){
	// Wait until Data Register is emtpy
	while (!(UCSR0A & (1 << UDRE0)));
	// Then write into Register
	UDR0 = c;
}

void uart_print(const char* str){
	while (*str){
		uart_putchar(*str++);
	}
}




// ---------------------------------------------------------------------------
// ADC
// ---------------------------------------------------------------------------
void adc_init(){
	ADCSRA = (1 << ADEN) // Enable the ADC
	| (1 << ADPS2) // Set ADPS0-2 to 1 for prescaler of 128
	| (1 << ADPS1) // (=> 16.000.000 / 128 = 125.000  ->  125 kHz ADC-Frequency for 16 MHz)
	| (1 << ADPS0);
}

uint16_t adc_read(){
	ADCSRA |= (1 << ADSC); // Start Conversion
	while (ADCSRA & (1 << ADSC)); // Wait until Conversion is finished
	// ADC is 10 bits in two registers: ADCL is low 8 bits, ADCH is high 2 bits
	// Combine them into one 10-bit result
	uint8_t low  = ADCL;
	uint8_t high = ADCH;
	uint16_t result = (high << 8) | low;
	return result;
}



// ---------------------------------------------------------------------------
// POTI
// ---------------------------------------------------------------------------
analog read_external_poti(){
	// REFS1 = 0 and REFS0 = 1 for AVCC 5V as Voltage Reference
	// MUX3..0 = 0 for ADC0 (PC0 or A0) <==!!!
	ADMUX = (1 << REFS0);
	_delay_ms(STABILIZATION_DELAY_MS); // Wait at least 5ms for stabilasation of 1,1V Reference

	analog p;
	p.adc = (uint16_t)adc_read(); // Get ADC reading
	p.volts = (uint32_t)p.adc * (uint32_t)5000 / (uint32_t)1023;
	p.value = 0;
	return p;
}



// ------------------------------------------------------
// Timer0 PWM
// ------------------------------------------------------
void timer0_pwm_init(void) {
    // Set PD6 (OC0A pin on ATmega328P) as output
    DDRD |= (1 << PD6);

    // TCCR0A – Timer/Counter Control Register A
    // WGM02=0, WGM01=1, WGM00=1 -> Fast PWM with TOP = 0xFF
    // COM0A1=1 -> Clear OC0A on Compare Match, set OC0A at BOTTOM
	// Datasheet page 106
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1);

    // TCCR0B – Timer/Counter Control Register B
    // CS01=1, CS00=1 -> Clock prescaler of 64 (16mHz with prescaler of 64 ticks 250 at 1kHz)
	// Datasheet page 109
    TCCR0B = (1 << CS01) | (1 << CS00);

    // Init duty cycle in OCR0A to 0
    OCR0A = 0;
}



int main(void){
	
	analog poti;
	uint8_t duty;
	char buffer[64];

	// Initialize ADC and Timer1
	uart_init();
	adc_init();
	timer0_pwm_init();

	while (1){
		
		// 10-bit result: 0-1023
		poti = read_external_poti();  

		// Convert ADC value to OCR0A Duty Cycle
		duty = (uint32_t)poti.adc * (uint32_t)255 / (uint32_t)1023;

		// Update Duty Cycle
		OCR0A = duty;
		
		// UART Debug 
		sprintf(buffer, "Poti ADC: %d\nDuty Cycle: %d\nOCR1A: %d\n\r", poti.adc, duty, OCR0A);
		uart_print(buffer);


		_delay_ms(100);
	}

	return 0;
	
}


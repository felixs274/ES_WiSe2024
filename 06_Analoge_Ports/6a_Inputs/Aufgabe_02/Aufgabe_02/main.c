/*
 * Aufgabe_02.c
 *
 * Created: 02.01.2025 16:19:07
 * Author : Felix
 */ 

#define F_CPU 16000000UL
#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// UART
// ---------------------------------------------------------------------------
void uart_init(unsigned int baud){
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
void adc_init(void){
	
	ADMUX = (1 << REFS0); // REFS1 = 0, REFS0 = 1, for AVCC as Voltage Reference

	ADCSRA = (1 << ADEN) // Enable the ADC
	| (1 << ADPS2) // Set ADPS0-2 to 1 for prescaler of 128
	| (1 << ADPS1) // (=> ~125 kHz ADC-Frequency for 16 MHz)
	| (1 << ADPS0); 
}

uint16_t adc_read(void){
	// Select Analog Channel 0
	ADMUX &= 0xF0;  // Delete first 4 Bit (Channelbits)
	ADMUX |= 0x00;  // Select ADC0

	ADCSRA |= (1 << ADSC); // Start Conversion

	while (ADCSRA & (1 << ADSC)); // Wait until Conversion is finished

	// Return 10 Bit ADC value
	// ADC = ADCL + (ADCH << 8)
	return ADC;
}

// ---------------------------------------------------------------------------
// Hauptprogramm
// ---------------------------------------------------------------------------
int main(void){
	
	// Pin PC0 as Input
	DDRC &= ~(1 << DDC0);
	
	uart_init(9600);
	adc_init();

	// Welcome Text
	uart_print("ADC-Messung auf A0 (PC0), Referenz: AVcc=5V, 125kHz\r\n");

	// Buffer for sprintf
	char buffer[50];

	while (1){
		// Read ADC (0..1023)
		uint16_t adc_value = adc_read();

		// Calcuate Millivolt
		// AVcc = 5.0 V: 
		// So max ADC value of 1023 = 5000 mV
		// => mV = (adc_value * 5000) / 1023
		// uint32_t to prevent overflow
		uint32_t milliVolt = (uint32_t)adc_value * (uint32_t)5000 / (uint32_t)1023;

		// Result to String
		sprintf(buffer, "ADC=%4u => %4lu mV\r\n", adc_value, milliVolt);

		// Print over UART
		uart_print(buffer);

		_delay_ms(200);
	}
	
	return 0;
}

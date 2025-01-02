/*
 * TemperatureCalibration.c
 *
 * Created: 02.01.2025 20:13:52
 * Author : Felix
 */ 


#define F_CPU 16000000UL
#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)
#define STABILIZATION_DELAY_MS 15

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
	ADCSRA = (1 << ADEN) // Enable the ADC
	| (1 << ADPS2) // Set ADPS0-2 to 1 for prescaler of 128
	| (1 << ADPS1) // (=> ~125 kHz ADC-Frequency for 16 MHz)
	| (1 << ADPS0);
}

uint16_t adc_read(void){
	ADCSRA |= (1 << ADSC); // Start Conversion
	while (ADCSRA & (1 << ADSC)); // Wait until Conversion is finished
	// ADC is 10 bits in two registers: ADCL is low 8 bits, ADCH is high 2 bits
	// Combine them into one 10-bit result
	uint8_t low  = ADCL;
	uint8_t high = ADCH;
	uint16_t result = (high << 8) | low;
	return result;
	// Return 10 Bit ADC value
	return ADC;
}



// ---------------------------------------------------------------------------
// Temperature
// ---------------------------------------------------------------------------
void printInternalTemperature(void){
	// Buffer for sprintf
	char buffer[50];
	// REFS1 = 1, REFS0 = 1, for 1.1V as Voltage Reference and select ADC8
	ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
	_delay_ms(STABILIZATION_DELAY_MS); // Wait at least 5ms for stabilasation of 1,1V Reference


	// ####################################
	// Calibration ############################
	
	// According to the Datasheet, the ADC will measure 314 mV at about +25°C
	// We will use these values and the given calibration function to determine our constants T_OS and k
	// 314 mV would be about 292 ADC counts. (0.314*1024)/1.1 = 292.3054

	//uint16_t adcVal = adc_read();
	uint16_t adcVal = (uint16_t)292; // adc_read();
	uint32_t milliVolt = (uint32_t)adcVal * (uint32_t)1100 / (uint32_t)1023;


	const uint16_t T_OS = 267; // (292-x)/1
	const uint16_t k    = 100;  // 1 count/°C * 100 for uint16_t

	int16_t temperatureC = (((int16_t)adcVal - (int16_t)T_OS) * 100) / k;
	
	// End ####################################
	// ####################################

	
	sprintf(buffer, "ADC: %d Counts, Voltage: %lu mV, Temperature: %d C\r\n",adcVal, milliVolt, temperatureC);
	uart_print(buffer); // Print String
}



// ---------------------------------------------------------------------------
// Hauptprogramm
// ---------------------------------------------------------------------------
int main(void){
		
	uart_init(9600);
	adc_init();

	while (1){

		printInternalTemperature();		
		_delay_ms(200);
		
	}
	
	return 0;
}
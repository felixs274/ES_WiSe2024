/*
 * Aufgabe_02.c
 *
 * Created: 02.01.2025 16:19:07
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
// Temperature
// ---------------------------------------------------------------------------
analog readInternalTemperatureC(){
	// REFS1 = 1, REFS0 = 1, for 1.1V as Voltage Reference
	// MUX3..0 = 1000 to select ADC8 (internal temp)
	ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
	_delay_ms(STABILIZATION_DELAY_MS); // Wait at least 5ms for stabilasation of 1,1V Reference

	analog t;
	t.adc = (uint16_t)adc_read(); // Get ADC reading

	// According to the Datasheet, the ADC will measure 314 mV at about +25°C
	// We will use these values and the given calibration function to determine our constants T_OS and k
	// 314 mV would be about 292 ADC counts. (0.314*1024)/1.1V = 292.3054
	
	// T = { [(ADCH << 8) | ADCL] - T_OS } / k
	// According to the Datasheet, we have approximately 1 mV/°C, so k = 1
	// T = ( 292 ADC counts - T_OS ) / 1
	// T_OS = 267

	const uint16_t T_OS = 267;
	const uint16_t k    = 1;  // 1 count/°C * 100 for uint16_t

	t.value = (uint32_t )(t.adc - T_OS) / k; // ????????????
	t.volts = (uint32_t)t.adc * (uint32_t)1100 / (uint32_t)1023;
    return t;
}




// ---------------------------------------------------------------------------
// POTI
// ---------------------------------------------------------------------------
analog readExternalPoti(){
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





// ---------------------------------------------------------------------------
// Hauptprogramm
// ---------------------------------------------------------------------------
int main(){
	
	// Pin PC0 as Input
	DDRC &= ~(1 << DDC0);
	
	uart_init();
	adc_init();

	// Buffer for sprintf
	char buffer0[64];
	char buffer1[64];

	while (1){

		analog potiV = readExternalPoti();
		analog temperatureC = readInternalTemperatureC();

		// Result to String
		sprintf(buffer0, "Poti Voltage: %lu mV\n", potiV.volts);
		uart_print(buffer0);
		sprintf(buffer1, "Temperature: %lu C, ADC: %d = %lu mV\n\r", temperatureC.value, temperatureC.adc, temperatureC.volts);
		uart_print(buffer1);

		_delay_ms(200);
	}
	
	return 0;
}

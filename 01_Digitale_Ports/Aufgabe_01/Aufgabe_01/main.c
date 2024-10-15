/*
 * Aufgabe_01.c
 *
 * Created: 07.10.2024 18:31:22
 * Author : Felix
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>


int main(void){
	
	DDRB |= (1<<DDB2) | (1<<DDB1) | (1<<DDB0);
	PORTB |= (1<<DDB2) | (1<<DDB1) | (1<<DDB0);
	
    while (1){
		
		for(short counter = 0; counter <= 7; counter++){
			PORTB = (PORTB & ~(0b00000111)) | counter;
			_delay_ms(1000);
		}
		
    }
}
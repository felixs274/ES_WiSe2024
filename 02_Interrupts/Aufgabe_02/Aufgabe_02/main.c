/*
 * Aufgabe_03.c
 *
 * Created: 28.10.2024 16:22:55
 * Author : Felix
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Counter Max
#define c_max 7
	
// 8 Bit Register for Button States
volatile unsigned short btn_reg = 0;

#define btn_reg_active 0
#define btn_reg_start 1
#define btn_reg_reset 2


// Global Vars
unsigned int timer_ms = 0;
unsigned short counter = c_max;


// Read bit n of register
unsigned short read_btn_reg(unsigned short reg_n) {
	return (btn_reg & (1 << reg_n));
}

// Write bit n of register
unsigned short write_btn_reg(unsigned short reg_n, unsigned short reg_val) {
	if (reg_val) {
		btn_reg |= (1 << reg_n);
		return 1;
	}
	btn_reg &= ~(1 << reg_n);
	return 1;
}


// Load values of Dip Switch
unsigned short load_dip() {
	return ((PIND >> 5) & 0b00000111);
}


// Check button press
unsigned short check_button_press(unsigned short pin) {
	return(!(PIND & (1 << pin)) || !(PINC & (1 << pin)));
}


// Start Button A3 Press
ISR(INT0_vect) {
	cli();
	if (check_button_press(PIND2)) {
		write_btn_reg(btn_reg_active, 1);
		} else {
		write_btn_reg(btn_reg_active, 0);
	}
	sei();
}

// Reset Button A4 Press
ISR(PCINT1_vect) {
	cli();
	if (check_button_press(PINC0)) {  // Check if button is low
		counter = load_dip(); // Set counter to DIP switch value
		write_btn_reg(btn_reg_active, 0); // Deactivate counter
		PORTB = (PORTB & ~(0b00000111)) | counter;
	}
	sei();
}


// Counter reset if software Interrupt
ISR(PCINT0_vect) {
	cli();
	counter = 0;
	PORTB = (PORTB & ~(0b00000111)) | counter;
	sei();
}


int main(void) {

	// Set LEDs DDRB2-0 to Output = 1
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);

	// Set DDRD7-5 (DIP Switch) to Input = 0
	DDRD &= ~((1 << DDD7) | (1 << DDD6) | (1 << DDD5));

	// Set Buttons A4 & A3 to Input = 0
	DDRD &= ~(1 << DDD2); // A3 (INT0)
	DDRC &= ~(1 << DDC0); // A4 (PCINT8 on PCINT1)
	
	// Set DB3 as Input for SW Interrupt
	DDRB |= (1 << DDB3);

	// Enable Pull-Ups for DIP and Buttons
	PORTD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2);
	PORTC |= (1 << DDC0);


    // PD2 (INT0) on Falling Edge
    EICRA |= (1 << ISC01);
    EIMSK |= (1 << INT0);
	
	// Pin-Change-Interrupt for PC0 (PCINT8)
	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1 << PCINT8); 

	// Pin-Change-Interrupt Software Interrupt on PB3
	PCICR |= (1 << PCIE0);
	PCMSK0 |= (1 << PCINT3);

	sei(); 
	

	while (1) {
		// Update counter and display on LEDs every second if active
		if (read_btn_reg(btn_reg_active)) {
			if (timer_ms >= 1000) {
				timer_ms = 0; // Reset timer
				if (counter == c_max) {
					PORTB ^= (1 << DDB3); // Flip PB3 and trigger SW Interrupt
				} else {
					counter += 1; // Increment counter
				}
				PORTB = (PORTB & ~(0b00000111)) | counter; // Output counter to LEDs
			}
		}

		_delay_ms(100);
		timer_ms += 100;
	}
	
}

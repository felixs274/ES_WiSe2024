/*
 * Aufgabe_03.c
 *
 * Created: 16.10.2024 14:22:55
 * Author : Felix
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

// Counter Max
#define c_max 7

// 8 Bit Register for Button States
volatile unsigned short btn_reg = 0;

#define btn_reg_active 0
#define btn_reg_start 1
#define btn_reg_reset 2


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


// Check button press with debounce and edge detection
unsigned short check_button_press(unsigned short pin, unsigned short reg) {
	if (!(PIND & (1 << pin))) {
		write_btn_reg(reg, 1);
	}
	else {
		write_btn_reg(reg, 0);
	}
	return read_btn_reg(reg);
}



int main(void) {
	unsigned int timer_ms = 0;
	unsigned short counter = c_max;

	// Set DDRB2-0 to Output = 1
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);

	// Set DDRD7-5 (DIP Switch) to Input = 0
	DDRD &= ~((1 << DDD7) | (1 << DDD6) | (1 << DDD5));

	// Set DDRD4-3 (Buttons A4 & A3) to Input = 0
	DDRD &= ~((1 << DDD4) | (1 << DDD3));	

	// Enable Pull-Ups for DDRD7-3
	PORTD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD4) | (1 << DDD3);

	// Main Loop
	while (1) {
		// Check for Start Button A3 Press with debounce
		if (check_button_press(PIND3, btn_reg_start)) {
			write_btn_reg(btn_reg_active, 1); // Activate counter
		}
		
		// Check for Reset Button A4 Press with debounce
		if (check_button_press(PIND4, btn_reg_reset)) {
			counter = load_dip(); // Set counter to DIP switch value
			write_btn_reg(btn_reg_active, 0); // Deactivate counter
			PORTB = (PORTB & ~(0b00000111)) | counter;
		}

		// Update counter and display on LEDs every second if active
		if (read_btn_reg(btn_reg_active)) {
			if (timer_ms >= 1000) { // 1-second interval
				timer_ms = 0; // Reset timer
				if (counter == c_max) {
					counter = 0; // Reset to max on overflow
				} else {
					counter += 1; // Decrement counter
				}
				PORTB = (PORTB & ~(0b00000111)) | counter; // Output counter to LEDs
			}
		}
		
		_delay_ms(100); // Delay of 100ms
		timer_ms += 100; // Increment timer_ms by 100ms
	}
}

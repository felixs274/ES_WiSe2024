#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint32_t timer1_ticks = 0; // Track the number of timer ticks
volatile uint8_t led_counter = 0;  // LED counter variable

// Timer1 Compare Match Interrupt
ISR(TIMER1_COMPA_vect) {
	timer1_ticks++;
}

// Configure Inputs and Outputs
void port_io_init() {
	// Configure PB0, PB1, and PB2 as output for the counter display
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);
}

// Initialize Timer1 Interrupts for Compare Match Mode
void timer_interrupt_init() {
	// Set Timer1 to CTC mode
	TCCR1B |= (1 << WGM12);

	// Set the compare value for 1 µs interrupt
	OCR1A = 15;

	// Enable Timer1 Compare Match A interrupt
	TIMSK1 |= (1 << OCIE1A);

	// Prescaler of 1
	TCCR1B |= (1 << CS10);
}

int main() {
	port_io_init();
	timer_interrupt_init();

	sei(); // Enable Interrupts

	while (1) {
		// Check if 1 second (1,000,000 ticks) has passed
		if (timer1_ticks >= 1000000) {
			timer1_ticks = 0; // Reset the tick counter

			// Update LED display
			PORTB = (PORTB & ~(0b00000111)) | led_counter;

			// Increment LED counter (roll over after 7)
			if (led_counter == 7)
			led_counter = 0;
			else
			led_counter++;
		}
	}
}

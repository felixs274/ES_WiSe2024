/*
 * Aufgabe_01.c
 *
 * Created: 11.11.2024 14:27:45
 * Author : Felix
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Counter Max
#define C_MAX 7

// Global variables
volatile uint8_t counter = C_MAX;  // 3-bit counter variable
volatile uint8_t timer0_overflows = 0;

// Event & Button state flags
volatile uint8_t active = 0;
volatile uint8_t event_50ms = 0;
volatile uint8_t event_1s = 0;

// Load values from DIP switch
unsigned short load_dip() {
	return ((PIND >> 5) & 0b00000111);
}

/*
For 1000ms:
16.000.000 / (1024 * 256) = 61,035....

for 50ms:
16.000.000 / (1024 * 256) * (1/20s) = 3,05...
*/
// Timer0 interrupt for 50 ms and 1 second events
ISR(TIMER0_OVF_vect) {
	
	// Increment overflow counter every time the timer overflows
	timer0_overflows++;

	// Check if 50ms has passed (3 overflows)
	if (timer0_overflows % 3 == 0) {
		event_50ms = 1;
	}
	
    // Check if 1000ms has passed (61 overflows)
    if (timer0_overflows >= 61) {
	    event_1s = 1;
	    timer0_overflows = 0;
    }
	
}


int main(void) {

    // Set Timer0
    TCCR0A = 0x00;    // Normal mode
    TCCR0B = 0x05;    // Prescaler of 1024

    // Enable overflow interrupt for Timer0
    TIMSK0 = (1 << TOIE0);

    // Enable global interrupts
    sei();

	// Configure PB0, PB1, and PB2 as output for the counter display
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);

	// Configure PD5-PD7 (DIP Switch) and buttons (PD2, PC0) as inputs
	DDRD &= ~((1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2));
	DDRC &= ~(1 << DDC0);

	// Enable pull-ups for DIP switches and buttons
	PORTD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2);
	PORTC |= (1 << DDC0);

	while (1) {
		// Check 50ms event
		if (event_50ms) {
			event_50ms = 0;  // Reset 50ms event flag

			// Button polling for start/stop
			if (!(PIND & (1 << DDD2))) {  // Button A3 (start/stop)
				active = 1;  // Toggle active state on button press
			}

			if (!(PINC & (1 << DDC0))) {  // Button A4 (reset)
				counter = load_dip();        // Set counter to DIP switch value
				active = 0;  // Deactivate counter
				PORTB = (PORTB & ~(0b00000111)) | counter;  // Update LEDs
			}
		}

		// Check 1-second event
		if (event_1s) {
			event_1s = 0;  // Reset 1s event flag

			// Update counter if active
			if (active == 1) {
				if (counter == C_MAX) {
					counter = 0; // Reset counter if it reaches C_MAX
					} else {
					counter += 1;  // Increment counter
				}
				PORTB = (PORTB & ~(0b00000111)) | counter;  // Update LEDs
			}
		}
	}

	return 0;
}

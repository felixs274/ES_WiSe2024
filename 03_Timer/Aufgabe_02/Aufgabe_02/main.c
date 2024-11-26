#define F_CPU 16000000UL
#define C_MAX 7
#define MAX_TIMERS 2

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Global Variables
volatile static uint32_t timer1_ticks = 0;  // Counts Timer1 ticks
volatile static uint8_t led_counter = 0;   // LED counter
volatile uint8_t interrupt_flag = 0;

// Typedef for Callback function
typedef void (*TimerCallback)(void);

// Configure Software Timer
typedef struct {
	uint32_t ticks;         // Timer duration in Timer1 ticks
	TimerCallback callback; // Callback function
	uint8_t is_active;      // Active flag
} Timer;

// Array of Timers
static Timer timers[MAX_TIMERS] = {0};

// Load values from DIP switch
uint8_t load_dip() {
	return ((PIND >> 5) & 0b00000111);
}

// Declare a Timer
void declareTimer(uint8_t id, uint32_t duration_us, TimerCallback callback) {
	// Convert microseconds to Timer1 ticks (with prescaler 8)
	timers[id].ticks = (duration_us * 2) / 8 - 1; // Subtract 1 because we start counting at 0
	timers[id].callback = callback;
	timers[id].is_active = 0; // Initially inactive
}

// Start a Timer
void startTimer(uint8_t id) {
	timers[id].is_active = 1; // Activate the timer
}

// Cancel a Timer
void cancelTimer(uint8_t id) {
	timers[id].is_active = 0; // Deactivate the timer
}

// 1-second Event
void event_1s() {
	if (led_counter != C_MAX) {
		led_counter += 1;
		} else {
		led_counter = 0;
	}
}

// 50-ms Event
void event_50ms() {
	// Button A3 (start)
	if (!(PIND & (1 << DDD2))) {
		startTimer(0);
	}

	// Button A4 (stop/reset)
	if (!(PINC & (1 << DDC0))) {
		cancelTimer(0);
		led_counter = load_dip(); // Set counter to DIP switch value
	}

	PORTB = (PORTB & ~(0b00000111)) | led_counter;
}

// Timer1 Compare Match Interrupt (Ticks Mode)
ISR(TIMER1_COMPA_vect) {
	timer1_ticks += 1;
}

// Configure Inputs and Outputs
void port_io_init() {
	// Configure PB0, PB1, and PB2 as output for the counter display
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);

	// Configure PD5-PD7 (DIP Switch) and buttons (PD2, PC0) as inputs
	DDRD &= ~((1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2));
	DDRC &= ~(1 << DDC0);

	// Enable pull-ups for DIP switches and buttons
	PORTD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2);
	PORTC |= (1 << DDC0);
}

// Initialize Timer1 Interrupts for Compare Match Mode
void timer_interrupt_init() {
	// Configure Timer1 in CTC mode
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS11);  // CTC mode, Prescaler 8

	// Set Compare Register to define tick rate (1 microsecond = 2 ticks)
	OCR1A = 1;  // Match after 2 timer ticks (1 microsecond)

	// Enable Timer1 Compare Match A interrupt
	TIMSK1 = (1 << OCIE1A);

	// Clear Timer1 counter
	TCNT1 = 0;
}

int main() {
	port_io_init();
	timer_interrupt_init();

	declareTimer(0, 1000000, event_1s);  // 1-second timer
	declareTimer(1, 50000, event_50ms);  // 50-ms timer
	startTimer(1);                       // Start 50-ms timer

	// Enable Interrupts
	sei();

	while (1) {
		
		for (uint8_t i = 0; i < MAX_TIMERS; i++) {
			if (timers[i].is_active) {              // Check if timer is active
				if (timer1_ticks % timers[i].ticks == 0) {      
					timers[i].callback();          // Execute the callback
					timers[i].is_active = 0;       // Deactivate after execution
				}
			}
		}	
		
	}
}

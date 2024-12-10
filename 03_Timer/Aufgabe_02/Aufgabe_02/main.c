#define F_CPU 16000000UL
#define MAX_TIMERS 5
#define C_MAX 7

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t led_counter = 0;
volatile uint32_t virtual_timer_ticks = 0;  // Global counter (1 tick = 1 ms)

// Structure to represent each virtual timer
typedef struct {
	uint32_t timeout;           // Timer timeout in ticks (ms)
	uint32_t last_tick;         // Last tick when the timer was last triggered
	void (*callback)(void);     // Function to call when the timer expires
	uint8_t active;             // Timer active flag (1 = active, 0 = inactive)
} VirtualTimer;

// Array of virtual timers
VirtualTimer timers[MAX_TIMERS];

// Function to declare a timer
void declareTimer(uint8_t timer_index, uint32_t timeout_us, void (*callback)(void)) {
	if (timer_index < MAX_TIMERS) {
		timers[timer_index].timeout = timeout_us/4;       // Set the timeout value 
		timers[timer_index].callback = callback;         // Set the callback function
		timers[timer_index].active = 0;                  // Timer is inactive by default
	}
}

// Function to start or restart a timer
void startTimer(uint8_t timer_index) {
	timers[timer_index].last_tick = virtual_timer_ticks; // Set last tick to current time
	timers[timer_index].active = 1;                        // Mark timer as active
}

// Function to cancel (stop) a timer
void cancelTimer(uint8_t timer_index) {
	timers[timer_index].active = 0;  // Deactivate the timer
}





// Load values from DIP switch
uint8_t load_dip() {
	return ((PIND >> 5) & 0b00000111);
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









// Timer1 Compare Match ISR
ISR(TIMER1_COMPA_vect) {
	virtual_timer_ticks++; 
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
	
	// Init off
	// PORTB |= (1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2);
}

// Initialize Timer1 Interrupts for Compare Match Mode
void timer_interrupt_init() {
	// Set Timer1 to CTC mode
	TCCR1B |= (1 << WGM12); 
	
	// Set the compare value
	// Interrupts triggers every 1 µs!
	OCR1A = 15;  // For 16 MHz with no prescaler 
	
	// Enable Timer1 Compare Match A interrupt
	TIMSK1 |= (1 << OCIE1A); // Enable interrupt on compare match

	// Prescaler of 0
	TCCR1B |= (1 << CS10); 
}

int main() {
	port_io_init();
	timer_interrupt_init();
	
	sei(); // Enable Interrupts

	declareTimer(0, 1000000, event_1s);  // 1-second timer
	declareTimer(1, 50000, event_50ms);  // 50ms timer
	startTimer(1);  

	while (1) {

		// Check each virtual timer
		for (uint8_t i = 0; i < MAX_TIMERS; i++) {
			if (timers[i].active && virtual_timer_ticks - timers[i].last_tick >= timers[i].timeout) {
				// Timer expired -> call callback
				if (timers[i].callback) {
					timers[i].callback();
				}
			
				timers[i].last_tick = virtual_timer_ticks;
			}
		}
	
	}
}

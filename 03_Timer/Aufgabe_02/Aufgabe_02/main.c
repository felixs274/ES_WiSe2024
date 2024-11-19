/*
 * Aufgabe_02.c
 *
 * Created: 12.11.2024 23:46:13
 * Author : Felix
 */ 

#define F_CPU 16000000UL
#define TIMER_PRESCALER 1024
#define C_MAX 7
#define MAX_TIMERS 5

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


// Global Variables
volatile static uint32_t timer0_ticks = 0;
volatile static uint8_t led_counter = 0;


// Typedef for Callback function
typedef void (*TimerCallback)(void); 

// Configure Software Timer
typedef struct {
	uint32_t duration; // duration in 탎 - automatically converted to Timer0 ticks in declareTimer 
	uint32_t ticks; // current ticks
	TimerCallback callback;
	uint8_t is_active;
} Timer;

// Array of Timer
static Timer timers[MAX_TIMERS] = {0};


// Load values from DIP switch
uint8_t load_dip() { 
	return ((PIND >> 5) & 0b00000111);
}



void declareTimer(uint8_t id, uint32_t duration_us, TimerCallback callback){ // 1.000.000탎 = 1s
	timers[id].ticks = (uint32_t)(duration_us/16384); // Calculates given 탎 into Timer0 overflow ticks -> (16mHz * 탎 / 1.000.000) / (256*1024) = 16 * 탎 / 262144 = 탎 / 16384
	timers[id].callback = callback;
	timers[id].is_active = 0; // Initially inactive
}

void startTimer(uint8_t id){
	timers[id].is_active = 1; // Activate
}

void cancelTimer(uint8_t id) {
	timers[id].is_active = 0; // Deactivate
}


void event_1s(){
	if(led_counter != C_MAX){
		led_counter += 1;
	} else {
		led_counter = 0;
	}
}

void event_50ms(){
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


// Timer0 Interrupt function
ISR(TIMER0_OVF_vect) {
	//cli();
	for (uint8_t i = 0; i < MAX_TIMERS; i++) { // Loop through Timers
		if(timers[i].is_active == 1){ // Checks if timer is active
			if(timer0_ticks % timers[i].ticks == 0){
				timers[i].callback();
			}
		}
	}
	timer0_ticks += 1;
	//sei();
}


// Configure Inputs and Outputs
void port_io_init(){
	// Configure PB0, PB1, and PB2 as output for the counter display
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);

	// Configure PD5-PD7 (DIP Switch) and buttons (PD2, PC0) as inputs
	DDRD &= ~((1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2));
	DDRC &= ~(1 << DDC0);

	// Enable pull-ups for DIP switches and buttons
	PORTD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2);
	PORTC |= (1 << DDC0);
}

// Initialize Timer Interrupts
void timer_interrupt_init(){
	// Set Timer0
	TCCR0A = 0x00;    // Normal mode
	//TCCR0B |= (1 << CS01);  // Prescaler 8
	//TCCR0B |= (1 << CS02);  // Prescaler 256
	TCCR0B |= (1 << CS02) | (1 << CS00); // Prescaler  1024

	// Enable overflow interrupt for Timer0
	TIMSK0 = (1 << TOIE0);
}


int main(){
	
	port_io_init();
	timer_interrupt_init();
	
	declareTimer(0, 1000000, event_1s);
	declareTimer(1, 50000, event_50ms);
	startTimer(1);
	
	// Enable Interrupts
	sei();
	
	while(1){
		// Loop
	}
	
}
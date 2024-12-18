#define F_CPU 16000000UL

// Timer defines
#define MAX_TIMERS 2
#define C_MAX 7

// UART defines
#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)

// Ring buffer parameters
#define RINGBUFFER_SIZE 32
#define RINGBUFFER_HIGH_LIMIT 20
#define RINGBUFFER_LOW_LIMIT 4

// UART Flowcontrol
#define XON  0x11
#define XOFF 0x13


#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


// Global Variables for LEDs and Timer
volatile uint8_t led_counter = 0; // Current value of LEDs
volatile uint32_t virtual_timer_ticks = 0; // Counts µs ticks
volatile uint8_t start_time = 5; // Starttime (0-7)

// Global Variables for UART
volatile uint8_t flowcontrol = 1; // 0 = XOFF, 1 = XON



// ######################################################################################
// MENU TEXTS
// ######################################################################################

const char menu_str[] PROGMEM =
"\r\n----- Stoppuhr-Menu -----\r\n"
"a: Stoppuhr starten\r\n"
"b: Stoppuhr stoppen und Zeit ausgeben\r\n"
"c: Startzeit einstellen\r\n"
"d: Aktuell eingestellte Startzeit anzeigen\r\n"
"h: Dieses Menu anzeigen\r\n"
"--------------------------\r\n";

const char prompt_str[] PROGMEM = "\r\nEingabe: ";
const char start_msg[] PROGMEM = "\r\nStoppuhr gestartet.\r\n";
const char stop_msg_1[] PROGMEM = "\r\nStoppuhr gestoppt.\r\nGestoppte Zeit: ";
const char stop_msg_2[] PROGMEM = "s\r\nLED Counter Wert: ";
const char set_time_msg[] PROGMEM = "\r\nGeben Sie die Startzeit (0-7) ein: ";
const char show_time_msg[] PROGMEM = "\r\nAktuell eingestellte Startzeit: ";
const char invalid_time_msg[] PROGMEM = "\r\nUngueltige Zeit! Bitte zwischen 0 und 7 eingeben.\r\n";
const char unknown_cmd_msg[] PROGMEM = "\r\nUnbekannter Befehl!\r\n";



// ######################################################################################
// STOPWATCH VARIABLES
// ######################################################################################

volatile uint8_t stopwatch_active = 0;
volatile uint32_t stopwatch_counter = 0;


// ######################################################################################
// TIMER FUNCTIONS
// ######################################################################################

// Structure to represent each virtual timer
typedef struct {
	uint32_t timeout;           // Timer timeout in ticks
	uint32_t last_tick;         // When the timer was last triggered
	void (*callback)(void);     // Function to call when the timer expires
	uint8_t active;             // Timer active flag (1 = active, 0 = inactive)
} VirtualTimer;

// Array of virtual timers
VirtualTimer timers[MAX_TIMERS];

// Function to declare a timer
void declareTimer(uint8_t timer_index, uint32_t timeout_us, void (*callback)(void)) {
	if (timer_index < MAX_TIMERS) {
		timers[timer_index].timeout = timeout_us/256; // Set the timeout value /265 because of new prescaler and compare value
		timers[timer_index].callback = callback;
		timers[timer_index].active = 0; // Timer is inactive by default
	}
}

// Function to start or restart a timer
void startTimer(uint8_t timer_index) {
	timers[timer_index].last_tick = virtual_timer_ticks; // Set last tick to current time
	timers[timer_index].active = 1;  // Set timer as active
}

// Function to cancel (stop) a timer
void cancelTimer(uint8_t timer_index) {
	timers[timer_index].active = 0;  // Deactivate the timer
}



// ######################################################################################
// RINGBUFFER FUNCTIONS
// ######################################################################################

typedef struct {
	uint8_t buffer[RINGBUFFER_SIZE];
	uint8_t head; // Pointer to the next write position in the buffer
	uint8_t tail; // Pointer to the next read position in the buffer
	uint8_t size; // Tracks the number of elements currently in the buffer
} RingBuffer;

volatile RingBuffer rb;

// Initialize an empty ring buffer
void ringBufferInit() {
	rb.head = rb.tail = 0;
	rb.size = 0;
}

// Write data to the buffer
void ringBufferWrite(uint8_t data) {
	rb.buffer[rb.head] = data; // Write the data to the buffer at the head position
	rb.head = (rb.head + 1) % RINGBUFFER_SIZE; // Move the head pointer to the next position, wrapping around if needed
	rb.size++; // Increment the size of the buffer
}

// Read data from the buffer
uint8_t ringBufferRead() {
	uint8_t data = 0;
	if (rb.size > 0) { // Check if the buffer is not empty
		data = rb.buffer[rb.tail]; // Read the data from the buffer at the tail position
		rb.tail = (rb.tail + 1) % RINGBUFFER_SIZE; // Move the tail pointer to the next position, wrapping around if needed
		rb.size--; // Decrement the size of the buffer
	}
	return data;
}




// ######################################################################################
// UART FUNCTIONS
// ######################################################################################

// Converts an ASCII char to a single digit uint8_t
uint8_t ascii_to_int(uint8_t c){
	return (c-48);
}

// Converts a single digit uint8_t to an ASCII char
uint8_t int_to_ascii(uint8_t c){
	return (c+48);
}

// Send single char over UART
void USART_Transmit(unsigned char data){
	while(!(UCSR0A & (1<<UDRE0))) ;
	UDR0 = data;
}

// Read received char from Ringbuffer
unsigned char USART_Receive(){
	while(rb.size == 0); // Wait until there is something in the Buffer
	uint8_t data = ringBufferRead();
	if(rb.size <= RINGBUFFER_LOW_LIMIT && flowcontrol == 0) {
		USART_Transmit(XON);
		flowcontrol = 1;
	}
	return data;
}

// Function to send String from RAM
void USART_puts(const char *str) {
	while(*str) {
		USART_Transmit(*str++);
	}
}

//Function to send String from PROGMEM
void USART_puts_P(const char *str) {
	char c;
	while((c = pgm_read_byte(str++))) {
		USART_Transmit(c);
	}
}

// Show start menu
void showMenu() {
	USART_puts_P(menu_str);
}

uint8_t flip_int_first_3bit(uint32_t val){
	// Extract the lowest 3 bits
	uint8_t lowest3 = val & 0x07;
	// Flip (invert) them
	// The remaining 5 bits are discarded by using the mask, so they are zero.
	uint8_t flipped = (~lowest3) & 0x07;
	
	return flipped;
}

uint8_t flip_3bit_ascii_for_counter(uint8_t a){
	
	return int_to_ascii(flip_int_first_3bit(ascii_to_int(a)));
	
}

// Reads the Starttime from User. 
uint8_t USART_readNumber() {

	uint8_t val = USART_Receive();

	if (val >= '0' && val <= '7') {
		// Accept only digits from 0 to 7
		USART_Transmit(val);
		return flip_3bit_ascii_for_counter(val);
	}
	
	if (val == '\r' || val == '\n') {
		// End of entry
		USART_Transmit('\r');
		USART_Transmit('\n');
		val = '0';
		return flip_3bit_ascii_for_counter(val);
	}
	
	// Ignore characters outside '0'-'7'
	USART_puts_P(invalid_time_msg);
	USART_puts_P(set_time_msg);
	USART_readNumber();
	return 0;
}

// Process menu
void processCommand(uint8_t cmd) {
	switch (cmd) {
		case 'a': // Start
			led_counter = start_time; // Reset Stoppwatch to Starttime
			startTimer(0); // Start LED counter timer
			stopwatch_active = 1;
			USART_puts_P(start_msg);
			break;
		case 'b': // Stop
			cancelTimer(0); // Stop LED counter timer
			stopwatch_active = 0;
			char b_buffer_1[4]; char b_buffer_2[4];
			USART_puts_P(stop_msg_1);
			USART_puts(itoa(stopwatch_counter, b_buffer_1, 10));
			USART_puts_P(stop_msg_2);
			USART_puts(itoa(flip_int_first_3bit(led_counter), b_buffer_2, 10));
			USART_Transmit('\r');
			USART_Transmit('\n');
			stopwatch_counter = 0;
			break;
		case 'c': { // Set Starttime
			USART_puts_P(set_time_msg);
			start_time = (uint8_t) USART_readNumber();
			break;
		}
		case 'd': // Show current Starttime
			USART_puts_P(show_time_msg);
			char d_buffer_1[4];
			USART_puts(itoa(flip_int_first_3bit(start_time), d_buffer_1, 10));
			USART_Transmit('\r');
			USART_Transmit('\n');
			break;
		case 'h': // Show Menu
			showMenu();
			break;
		default:
			USART_puts_P(unknown_cmd_msg);
			break;
	}
	USART_puts_P(prompt_str);
}



// ######################################################################################
// CALLBACK FUNCTIONS
// ######################################################################################

// 1-second Event
void event_count_leds() {
	
	if(stopwatch_active == 1) stopwatch_counter += 1;
	
	if (led_counter != C_MAX) {
		led_counter += 1;
	} else {
		led_counter = 0;
	}
}




// ######################################################################################
// INTERRUPTS
// ######################################################################################


// Timer1 Compare Match ISR
ISR(TIMER1_COMPA_vect) {
	virtual_timer_ticks++;
	
	// Check each virtual timer
	for (uint8_t i = 0; i < MAX_TIMERS; i++) {
		if (timers[i].active && virtual_timer_ticks - timers[i].last_tick >= timers[i].timeout) {
			// Timer expired -> call callback
			// USART_Transmit('-'); // Debug
			timers[i].callback();
			timers[i].last_tick = virtual_timer_ticks;
		}
	}
	
	PORTB = (PORTB & ~(0b00000111)) | led_counter; // Update LEDs 
}

// UART RX ISR
ISR(USART_RX_vect) {
	uint8_t data = UDR0; // The received character is ready here.
	ringBufferWrite(data);
	
	if(rb.size >= RINGBUFFER_HIGH_LIMIT && flowcontrol == 1) {
		USART_Transmit(XOFF);
		flowcontrol = 0;
	}
}






// ######################################################################################
// INITS
// ######################################################################################

// Configure Inputs and Outputs
void port_io_init() {
	// Configure PB0, PB1, and PB2 as output for the LED counter display
	DDRB |= (1 << DDB2) | (1 << DDB1) | (1 << DDB0);

	/* No need for inputs from 3.2
	// Configure PD5-PD7 (DIP Switch) and buttons (PD2, PC0) as inputs
	DDRD &= ~((1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2));
	DDRC &= ~(1 << DDC0);

	// Enable pull-ups for DIP switches and buttons
	PORTD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD2);
	PORTC |= (1 << DDC0);
	*/
	
}

// Initialize Timer1 Interrupts for Compare Match Mode
void timer_interrupt_init() {
	// Set Timer1 to CTC mode
	TCCR1B |= (1 << WGM12);
	
	// Set the compare value
	OCR1A = 63;  // For 16 MHz with 64 prescaler
	
	// Enable Timer1 Compare Match A interrupt
	TIMSK1 |= (1 << OCIE1A); // Enable interrupt on compare match

	// Prescaler of 64
	TCCR1B |= (1 << CS10) | (1 << CS11);
}

void USART_Init() {
	UBRR0H = (BAUD_CONST >> 8);
	UBRR0L = BAUD_CONST;
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
}


// ######################################################################################
// MAIN
// ######################################################################################

int main() {
	port_io_init();
	timer_interrupt_init();
	USART_Init();
	ringBufferInit();
	
	sei(); // Enable Interrupts

	declareTimer(0, 1000000, event_count_leds);  // 1-second timer to count LEDs

	// Needed UART settings
	USART_Transmit(XON);
	flowcontrol = 1;

	// Show start menu and entry String
    showMenu();
    USART_puts_P(prompt_str);
	
	led_counter, start_time = flip_3bit_ascii_for_counter(start_time);
	PORTB = (PORTB & ~(0b00000111)) | led_counter; // Update LEDs 

	while (1) {
		
		uint8_t c = USART_Receive(); // Read RingBuffer
		USART_Transmit(c);
		processCommand(c); // Process Menu input
		
	}
}
/*
 * Aufgabe_01.c
 *
 * Created: 19.11.2024 14:27:28
 * Author : Felix
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// UART
#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU / (BAUDRATE * 16UL))) - 1)

#define RING_BUFFER_SIZE 32
#define XON 0x11
#define XOFF 0x13

volatile unsigned char ring_buffer[RING_BUFFER_SIZE];
volatile uint8_t write_index = 0;
volatile uint8_t read_index = 0;

volatile uint8_t flow_control = 1; // 1 = XOn (ready to send), 0 = XOff (paused)
volatile uint8_t xoff_sent = 0; // Tracks whether XOff has already been sent

volatile char greeter_state; // a, b, c, d, x

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


// Function to initialize UART
void USART_Init() {
	UBRR0H = (BAUD_CONST >> 8);
	UBRR0L = BAUD_CONST;
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable RX interrupt
}

// Function to transmit a single character via UART
void USART_Transmit(unsigned char data) {
	// Wait if flow control is in XOff state
	while (!flow_control);
	while (!(UCSR0A & (1 << UDRE0))); // Wait until the data register is empty
	UDR0 = data; // Send the data
}

// UART interrupt service routine for receiving data
ISR(USART_RX_vect) {
	unsigned char data = UDR0; // Read received data

	if (data == XON) {
		flow_control = 1; // Resume transmission
		xoff_sent = 0;    // Reset XOff tracking
		return;
	}
	if (data == XOFF) {
		flow_control = 0; // Pause transmission
		return;
	}

	uint8_t next_index = (write_index + 1) % RING_BUFFER_SIZE;

	// Store data in the buffer only if it is not full
	if (next_index != read_index) {
		ring_buffer[write_index] = data;
		write_index = next_index;
	}

	// Check buffer fill level for flow control
	uint8_t buffer_usage;
	if (write_index >= read_index) {
		buffer_usage = write_index - read_index;
		} else {
		buffer_usage = RING_BUFFER_SIZE - read_index + write_index;
	}


	if (buffer_usage >= (RING_BUFFER_SIZE * 8 / 10) && !xoff_sent) { // 80% full
		USART_Transmit(XOFF);
		xoff_sent = 1;
		} else if (buffer_usage <= (RING_BUFFER_SIZE * 5 / 10) && xoff_sent) { // 50% full
		USART_Transmit(XON);
		xoff_sent = 0;
	}
}

// Function to receive a single character from the ring buffer
unsigned char USART_Receive() {
	// Wait until data is available in the buffer
	while (read_index == write_index);

	unsigned char data = ring_buffer[read_index];
	read_index = (read_index + 1) % RING_BUFFER_SIZE;
	return data;
}


uint8_t greeter_menu(){
	
	if(greeter_state == "a"){ 
		const char header[] = "Timer Einstellungen";
		const char set1[] = "(a) Timer starten";
		const char set2[] = "(b) Timer stoppen";
		const char set3[] = "(c) Startzeit einstellen";
		const char set4[] = "(d) Aktuelle Startzeit anzeigen";	
	}
	
	for (int f = 0; header[f] != '\0'; f++)
		USART_Transmit(header[f]);
	
	for (int f = 0; set1[f] != '\0'; f++)
		USART_Transmit(set1[f]);	
	

}

char greeter_state_ch(){
	greeter_state =  USART_Receive();
	if(greeter_state == "a" || greeter_state == "b" || greeter_state == "c" || greeter_state == "d")
		return greeter_state;
	greeter_state = "x";
}


int main(void) {

	// Set Timer0
	TCCR0A = 0x00;    // Normal mode
	TCCR0B = 0x05;    // Prescaler of 1204

	// Enable overflow interrupt for Timer0
	TIMSK0 = (1 << TOIE0);

	USART_Init();

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




	char name[10];
	int g = 0;
	const char message[] = "Hier ATmega. Wer da?";
	

	// Send the initial message
	for (int f = 0; message[f] != '\0'; f++)
	USART_Transmit(message[f]);

	while (1) {
		// Receive a character from the ring buffer
		name[g] = USART_Receive();

		if (name[g] != 0x0d) { // Check for the carriage return (CR) character
			USART_Transmit(name[g]); // Echo the received character
			g++;
			continue;
		}

		// Send the response
		USART_Transmit(0x0d);
		USART_Transmit('H');
		USART_Transmit('i');
		USART_Transmit(' ');

		// Convert the received name to uppercase and send it back
		for (int f = 0; f < g; f++) {
			char upper_char = name[f];
			if (upper_char >= 'a' && upper_char <= 'z') {
				upper_char -= 32; // Convert to uppercase
			}
			USART_Transmit(upper_char);
		}

		g = 0; // Reset the buffer
		USART_Transmit(0x0d);
	}




	return 0;
}

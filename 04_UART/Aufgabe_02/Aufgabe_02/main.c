/*
 * Aufgabe_02.c
 *
 * Created: 19.11.2024 13:56:23
 * Author : scf32306
 */ 

#define F_CPU 16000000UL

#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU / (BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define RING_BUFFER_SIZE 32
#define XON 0x11
#define XOFF 0x13

volatile unsigned char ring_buffer[RING_BUFFER_SIZE];
volatile uint8_t write_index = 0;
volatile uint8_t read_index = 0;

volatile uint8_t flow_control = 1; // 1 = XOn (ready to send), 0 = XOff (paused)
volatile uint8_t xoff_sent = 0; // Tracks whether XOff has already been sent

// Function to initialize UART
void USART_Init() {
	UBRR0H = (BAUD_CONST >> 8);
	UBRR0L = BAUD_CONST;
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable RX interrupt
	sei(); // Enable global interrupts
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

int main(void) {
	char name[10];
	int g = 0;
	const char message[] = "Hier ATmega. Wer da?";
	USART_Init();

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
}

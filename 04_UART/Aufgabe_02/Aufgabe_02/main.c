#define F_CPU 16000000UL

#define BAUDRATE 9600
#define BAUD_CONST (((F_CPU/(BAUDRATE*16UL)))-1)

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


// Ring buffer parameters
#define RINGBUFFER_SIZE 32
#define RINGBUFFER_HIGH_LIMIT 20
#define RINGBUFFER_LOW_LIMIT 4

#define XON  0x11
#define XOFF 0x13

uint8_t flowcontrol = 1; // 0 = XOFF, 1 = XON

typedef struct {
	uint8_t buffer[RINGBUFFER_SIZE];
	uint8_t head; // Pointer to the next write position in the buffer
	uint8_t tail; // Pointer to the next read position in the buffer
	uint8_t size; // Tracks the number of elements currently in the buffer
} RingBuffer32;

volatile RingBuffer32 rb;

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







void USART_Transmit(unsigned char data){
	while(!(UCSR0A & (1<<UDRE0))) ;
	UDR0 = data;
}


unsigned char USART_Receive(){
	while(rb.size == 0);
	uint8_t data = ringBufferRead();
	if(rb.size <= RINGBUFFER_LOW_LIMIT && flowcontrol == 0) {
		USART_Transmit(XON);
		flowcontrol = 1;
	}
	return data;
}




ISR(USART_RX_vect) {
	uint8_t data = UDR0; // The received character is ready here.
	ringBufferWrite(data);
	
	if(rb.size >= RINGBUFFER_HIGH_LIMIT && flowcontrol == 1) {
		USART_Transmit(XOFF);
		flowcontrol = 0;
	}
}




void USART_Init() {
	UBRR0H = (BAUD_CONST >> 8);
	UBRR0L = BAUD_CONST;
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); 
	sei(); // Enable global interrupts
}

int main(void){
	char name[512];
	int g = 0;
	const char meldung[]="Hier ATmega. Wer da?";
	
	ringBufferInit();
	USART_Init();
	USART_Transmit(XON);
	flowcontrol = 1;
	
	for(int f=0;meldung[f]!='\0';f++)
		USART_Transmit(meldung[f]);
		
	while (1) {
		name[g] = USART_Receive();
		
		_delay_ms(100);
	
		if (name[g]!=0x0d){ // If not carriage return
			USART_Transmit(name[g]);
			g++;
			continue;
		}
		USART_Transmit(0x0d);
		USART_Transmit('H');
		USART_Transmit('i');
		USART_Transmit(' ');
		for(int f=0;f<g;f++)
			USART_Transmit(name[f]);
		g = 0;
		USART_Transmit(0x0d);
    }
}


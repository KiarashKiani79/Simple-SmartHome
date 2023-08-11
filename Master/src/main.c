#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <LCD.h>
#include <stdbool.h>


long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Keypad
unsigned char column, row;
unsigned char keys[4][3] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};

bool isLogined = false;


unsigned char read_key() {
    do { 
	    PORTB &= 0x0F; 
	    row = PINB & 0x0F; 
    } while(row != 0x0F); 
  
  	do { 
		_delay_ms(20); 
		row = PINB & 0x0F; 
	} while(row == 0x0F); 
   
  	while(1) {
		PORTB = 0xEF;     //0b11101111 
		row = PINB & 0x0F; 
		if (row != 0x0F) { 
			column = 0; 
			break; 
		}

		PORTB = 0xDF;     //0b11011111 
		row = PINB & 0x0F; 
		if (row != 0x0F) { 
			column = 1; 
			break; 
		}

		PORTB = 0xBF;     //0b10111111 
		row = PINB & 0x0F; 
		if (row != 0x0F) { 
			column = 2; 
			break; 
		} 
  	} 
	
    if (row == 0x0E) return keys[0][column];
	else if (row == 0x0D) return keys[1][column];  
	else if (row == 0x0B) return keys[2][column];  
	else return keys[3][column]; 
}

int light = -10; // Initial value; set to -10 because it will never be used.
int lightOld = -10;
char temperature = -10; // Initial value; set to -10 because it will never be used.
char temperatureOld = -10; // Initial value; set to -10 because it will never be used.

void sendTemp() {
    temperature = ADCW >> 7;
    if (temperature != temperatureOld) {
        while (((UCSRA >> UDRE) & 1) == 0);
        UDR = 'T';
        while (((UCSRA >> UDRE) & 1) == 0);
        UDR = temperature; // Send the temp value.
        temperatureOld = temperature;
    }
}

void sendLight() {
    light = map(ADCW, 0, 123, 0, 100);
    if (light != lightOld) {
        while (((UCSRA >> UDRE) & 1) == 0);
        UDR = 'L';
        while (((UCSRA >> UDRE) & 1) == 0);
        UDR = light; // Send the light value.
        lightOld = light;
    }
}

void sendKey(unsigned char key) {
    while (((UCSRA >> UDRE) & 1) == 0);
    UDR = 'P';
    while (((UCSRA >> UDRE) & 1) == 0);
    UDR = key; // Send the key value.
}

int main() {
    
    // Keypad
    DDRB = 0xF0; 
	PORTB = 0xFF; 

    DDRC = 0x00;

    PORTC = 0x01;


    // ADC
    ADMUX = (0<<REFS1) | (1<<REFS0) | (1 << ADLAR);
    ADCSRA = (1 << ADEN) | (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    PORTD |= (1 << PORTD3) | (1 << PORTD2);
    DDRD |= (1 << PD4);
	GICR |= (1 << INT1) | (1 << INT0);

    MCUCR |= (1 << ISC01) | (1 << ISC00) | (1 << ISC11) | (1 << ISC10);
	sei();

    //USART
    //UBRR = (8MHz/16(9600))-1 = 51.08 = 51 = 0x33
    //communication parameters : 8
    UBRRH = 0x00;
    UBRRL = 0x33;

    UCSRB = (1 << TXEN) | (0 << UCSZ2 );
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);

    bool temp = true;
    
    
    while (1) {
        if (isLogined) {
            if (((PINC >> PINC0) & 1) == 0) {
                temp = !temp;
                while (((PINC >> PINC0) & 1) == 0);
            }
            _delay_ms(5);
            //select between adc01 or adc00
            if (temp) {
                ADMUX = 0x60;
                ADMUX |= 0b00100000;
                ADCSRA |= ((1 << ADSC) | (1 << ADIF));
                while ((ADCSRA & (1 << ADIF)) == 0);
                sendTemp();
            }  
            else {
                ADMUX = 0x61;
                ADMUX &= 0b11011111;
                ADCSRA |= ((1 << ADSC) | (1 << ADIF));
                while ((ADCSRA & (1 << ADIF)) == 0);
                _delay_ms(10);
                sendLight();
            }
        }
        else {
            unsigned char tempKey = read_key();
            sendKey(tempKey);
        }
        _delay_ms(100);
    }
}



ISR (INT0_vect) {
    isLogined = true;
}

ISR (INT1_vect) {
    PORTD ^= (1 << PD4);
}

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <LCD.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_LENGTH 10

void setMotorsSpeedTemp();

//////////////////////////////////////////////////////////////////

const char CORRECT_PASSWORD[] = "12";
char password[MAX_LENGTH+1] = "\0";
int passwordSize = 0;
volatile bool passwordShow = true;
volatile bool passwordChanged;

const char SuccessMessage[] = "Access is granted";
const char FailureMessage[] = "Wrong password";
const char WelcomeMessage[] = "Enter password";

void AddCharacter(const char character)
{
	if (passwordSize < MAX_LENGTH) {
		password[passwordSize] = character;
		password[passwordSize + 1] = '\0';
		passwordSize++;
	}
}

void DeleteLastCharacter()
{
	if (passwordSize > 0) {
		password[passwordSize - 1] = '\0';
		passwordSize--;
	}
}

void TogglePasswordShow()
{
	passwordShow = !passwordShow;
	passwordChanged = true;
}

bool passwordEntered = false;

void PrintLoginMessage(const bool isCorrect)
{
    // Clear screen
    LCD_cmd(0x01);
    if (isCorrect) {
        PORTD |= (1 << PORTD2);
        for (int i = 0; i < 18; i++) {
            LCD_write(SuccessMessage[i]);
        }
        passwordEntered = true;
        memset(password, 0, sizeof(password));
	    passwordSize = 0;
        _delay_ms(50);
        LCD_cmd(0x01);
    }

    else {
        for (int i = 0; i < 15; i++) {
            LCD_write(FailureMessage[i]);
        }
        memset(password, 0, sizeof(password));
	    passwordSize = 0;
	    _delay_ms(50);
        LCD_cmd(0x01);
	    for (int i = 0; i < 15; i++) {
            LCD_write(WelcomeMessage[i]);
        }
    }
}

bool SubmitPassword()
{
	if (strcmp(CORRECT_PASSWORD, password) == 0) return true;
	else return false;
}

void DisplayPassword()
{
	LCD_cmd(0x01);
	if (passwordSize == 0) {
		return;
	}
	if (passwordShow == true) {
        for (int i = 0; i < passwordSize; i++)
		    LCD_write(password[i]);
	}
	else {
		for (int i = 0; i < passwordSize; i++) {
			LCD_write('*');
		}
	}
	_delay_ms(10);
}
//////////////////////////////////////////////////////////////////

char temperature = -10;
char temperatureOld = -100;
int light = -10;
int lightOld = -100;
unsigned char mode;
unsigned char dutyCycleLight;

void readTemp() {  
    if (mode == 'T') {
        while (((UCSRA>>RXC) & 1) == 0);
        temperature = UDR;
        if (temperatureOld != temperature) {
            init_LCD();
            setMotorsSpeedTemp();
            int length = snprintf(NULL, 0, "%d", temperature);
            char *str = malloc(length + 1);
            snprintf(str, length + 1, "%d", temperature - 1);
            LCD_cmd(0x01);
            LCD_write('T');
            LCD_write(':');
            LCD_write(' ');
            for (int i = 0; i < length; i++) {
                LCD_write(str[i]);
            }
            temperatureOld = temperature;
        }
    }  
}

void readLight() {
    if (mode == 'L') {
        init_LCD();
        while (((UCSRA>>RXC) & 1) == 0 ){};
        light = UDR;
        if (lightOld != light) {
            int length = snprintf(NULL, 0, "%d", light);
            char *str = malloc(length+1);
            snprintf(str, length + 1, "%d", light);
            LCD_cmd(0x01);
            LCD_write('L');
            LCD_write(':');
            LCD_write(' ');
            for (int i = 0; i < length; i++) {
                LCD_write(str[i]);
            }
            lightOld = light;
        }
    }
}

void setMotorsSpeedTemp() {
    if (temperature <= 3) {
        PORTB |= 0b00010000;
    }
    else {
        PORTB &= 0b11101111;
    }

    if (temperature >= 55) {
        PORTB |= 0b00100000;
    }
    else {
        PORTB &= 0b11011111;
    }

    /* Set heater speed */
    if (temperature >= 3 && temperature < 5) { 
        OCR1A = 255 * 100 / 100;
        OCR1B = 0;
    }
    else if (temperature >= 5 && temperature < 10) { 
        OCR1A = 255 * 75 /100;
        OCR1B = 0;
    }
    else if (temperature >= 10 && temperature < 15) { 
        OCR1A = 255 * 50 /100;
        OCR1B = 0;
    }
    else if (temperature >= 15 && temperature < 20) { 
        OCR1A = 255 * 25 / 100;
        OCR1B = 0;
    }
    else if (temperature >= 20 && temperature < 25) {
        OCR1A = 0;
        OCR1B = 0;
    }

    /* Set cooler speed */
    else if (temperature >= 25 && temperature < 30) {
        OCR1A = 0;
        OCR1B = 255 * 50 / 100; 
    }
    else if (temperature >= 30 && temperature < 35) {
        OCR1A = 0;
        OCR1B = 255 * 60 /100; 
    }
    else if (temperature >= 35 && temperature < 40) { 
        OCR1A = 0;
        OCR1B = 255 * 70 /100; 
    }
    else if (temperature >= 40 && temperature < 45) { 
        OCR1A = 0;
        OCR1B = 255 * 80 /100; 
    }
    else if (temperature >= 45 && temperature < 50) { 
        OCR1A = 0;
        OCR1B = 255 * 90 /100; 
    }
    else if (temperature >= 50 && temperature < 55) { 
        OCR1A = 0;
        OCR1B = 255 * 100 /100; 
    }
    else {
        OCR1A = 0;
        OCR1B = 0;
    }
}

void setMotorSpeedLight() {
    if (light >= 75 && light < 100) { 
        OCR0 = 255 * 25 /100; 
    }
    else if (light >= 50 && light < 75) { 
        OCR0 = 255 * 50 /100;
    }
    else if (light >= 25 && light < 50) { 
        OCR0 = 255 * 75 /100;
    }
    else if (light >= 0 && light < 25) { 
        OCR0 = 255 * 100 /100;
    }
}

unsigned char key;
void readChar() {
    if (mode == 'P') {    
        while (((UCSRA>>RXC) & 1) == 0 );
        key = UDR;
    }
}

int main (){

    DDRA = 0xff;
    DDRC = 0x07;

    DDRD = (1 << DDD2) | (1 << DDD4) | (1 << DDD5);
    DDRB |= (1<<DDB3) | (1 << DDB4) | (1 << DDB5);

    // PORTD |= (1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5);

    GICR |= (1 << INT1);

    MCUCR |= (0 << ISC11) | (1 << ISC10);
	sei();

    //USART
    //UBRR = (8MHz/16(9600))-1 = 51.08 = 51 = 0x33
    //communication parameters : 8
    UBRRH = 0x00;
    UBRRL = 0x33;

    UCSRB = (1 << RXEN) | (0 << UCSZ2 );
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);

    init_LCD();

    //TIMER COUNTER CONTROL
    TCCR0 = (1<<WGM00) | (1<<WGM01) | (1<<COM01) | (0<<COM00) | (1<<CS01);

    TCCR1A |= (0<<WGM11) | (1<<WGM10);               // Fast PWM
    TCCR1A |= (1<<COM1A1) | (0<<COM1A0) | (1<<COM1B1) | (0<<COM1B0);   // clear OC1A and OC1B on compare match (both are non-inverted)
    TCCR1B |= (0<<WGM13) | (1<<WGM12);               // Fast PWM
    TCCR1B |= (0<<CS12) | (1<<CS11) | (0<<CS10);     // prescaling=8

    LCD_cmd(0x01);
	    for (int i = 0; i < 15; i++) {
            LCD_write(WelcomeMessage[i]);
        }

    while(1) {
        while (((UCSRA>>RXC) & 1) == 0);
        unsigned char tempMode = UDR;

        mode = (tempMode == 'T') ? 'T' :
            (tempMode == 'L') ? 'L' :
            (tempMode == 'P') ? 'P' :
            mode;

        if (mode == 'T') {
            readTemp();
            setMotorsSpeedTemp();
        }

        else if (mode == 'L') {
            readLight();
            setMotorSpeedLight();
        }
        else if (mode == 'P') {
            readChar();

		    if (key != '#' && key != '*') {
			    AddCharacter(key);
			    DisplayPassword();
		    }
		    else if (key == '#') {
			    DeleteLastCharacter();
			    DisplayPassword();
		    }
		    else if (key == '*') {
			    bool IsPasswordCorrect = SubmitPassword();
			    PrintLoginMessage(IsPasswordCorrect);
		    }
		    _delay_ms(10);
        }
        _delay_ms(80);
    }
}

ISR (INT1_vect) {
    TogglePasswordShow();
}

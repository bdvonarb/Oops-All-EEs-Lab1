#include <Arduino.h>

#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define D9 3
#define D10 1

#define SER D0
#define Sclk D1
#define Rclk D2

#define dig1form 0x0080
#define dig2form 0x0040
#define dig3form 0x0020
#define dig4form 0x0010
#define dispONform 0x08

uint16_t dispData[4] = {dig1form, dig2form, dig3form, dig4form};

bool dispOn = false;

uint16_t timeC = 0;

void writeToReg(uint16_t trans);

void clock(uint8_t pin);

uint8_t segMap(uint8_t digit, bool dp);

void displayNum(uint16_t num);

void setup() {
    pinMode(SER, OUTPUT);
    pinMode(Sclk, OUTPUT);
    pinMode(Rclk, OUTPUT);

    //pinMode(13, OUTPUT);

    //dispData[0] = (segMap(1,0) << 8) | dig1form;
    //dispData[1] = (segMap(2,0) << 8) | dig2form;
    //dispData[2] = (segMap(3,0) << 8) | dig3form;
    //dispData[3] = (segMap(4,0) << 8) | dig4form;

    displayNum(1234);
    
}

void loop() {
    writeToReg(dispData[0]);
    writeToReg(dispData[1]);
    writeToReg(dispData[2]);
    writeToReg(dispData[3]);
    timeC++;
    displayNum(timeC);
    if(timeC > 12000) {
        timeC = 0;
    }
}

void writeToReg(uint16_t trans) {
    shiftOut(SER,Sclk,LSBFIRST, lowByte(trans) | (dispONform * dispOn));
    shiftOut(SER,Sclk,LSBFIRST, highByte(trans));
    clock(Rclk);
}

void clock(uint8_t pin) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(100);
    digitalWrite(pin, LOW);
}

void displayNum(uint16_t num) {
    //break up num into individual digits
    if(num > 9999) {
        //Display: Er.H1
        dispData[0] = dig1form | (segMap(14,0) << 8);
        dispData[1] = dig2form | (segMap(16,1) << 8);
        dispData[2] = dig3form | (segMap(17,0) << 8);
        dispData[3] = dig4form | (segMap(1,0) << 8);
    } else {
        dispData[0] = dig1form | (segMap(num/1000,0) << 8);
        num -= (num / 1000) * 1000;
        dispData[1] = dig2form | (segMap(num/100,0) << 8);
        num -= (num / 100) * 100;
        dispData[2] = dig3form | (segMap(num/10,0) << 8);
        dispData[3] = dig4form | (segMap(num % 10,0) << 8);
    }
}


uint8_t segMap(uint8_t digit, bool dp) {
    uint8_t map = 0;
    switch(digit) {
        case 0:
            map = 0b11111100;
            break;
        case 1:
            map = 0b01100000;
            break;
        case 2:
            map = 0b11011010;
            break;
        case 3:
            map = 0b11110010;
            break;
        case 4:
            map = 0b01100110;
            break;
        case 5:
            map = 0b10110110;
            break;
        case 6:
            map = 0b10111110;
            break;
        case 7:
            map = 0b11100000;
            break;
        case 8:
            map = 0b11111110;
            break;
        case 9:
            map = 0b11110110;
            break;
        case 10: //A
            map = 0b11101110;
            break;
        case 11: //b
            map = 0b00111110;
            break;
        case 12: //c
            map = 0b00011010;
            break;
        case 13: //d
            map = 0b01111010;
            break;
        case 14: //E
            map = 0b10011110;
            break;
        case 15: //F
            map = 0b10001110;
            break;
        case 16: //r
            map = 0b00001010;
            break;
        case 17: //H
            map = 0b01101110;
            break;
        case 18: //L
            map = 0b00011100;
            break;
        case 19: //o
            map = 0b00111010;
            break;
    }
    return map | dp;
}




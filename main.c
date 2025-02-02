#include <wiringPi.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>

#define DS 0 // data pin
#define STCP 3 // latch pin
#define SHCP 2 // clock pin
#define BUTTON 6
#define LED 26

#define LSBFIRST 0
#define MSBFIRST 1

#define DEBOUNCE 50
#define COUNTDOWN 1000
#define RESET 3000

volatile int running = 1;

// CA 7 segment display
const unsigned char segments[] = {
    0b10110000, // 3
    0b10100100, // 2
    0b11111001, // 1
    // 0b11000000, // 0
    // 0b10011001, // 4
    // 0b10010010, // 5
    // 0b10000010, // 6
    // 0b11111000, // 7
    // 0b11000000, // 8
    // 0b10011000, // 9
};

void shiftOut(int data_pin, int clock, int bit, unsigned char data) {
    for (int i = 0; i < 8; i++) {
        if (bit == LSBFIRST) {
            digitalWrite(data_pin, (data >> i) & 0x01);
        } else {
            digitalWrite(data_pin, (data >> (7 - i)) & 0x01);
        }

        digitalWrite(clock, HIGH);
        delayMicroseconds(10);
        digitalWrite(clock, LOW);
        delayMicroseconds(10);
    }
}

const unsigned char cleanup = 0b11111111;

void handle_cleanup() {
    digitalWrite(STCP, LOW);
    shiftOut(DS, SHCP, MSBFIRST, cleanup);
    digitalWrite(STCP, HIGH);
    digitalWrite(LED, LOW);

    pinMode(DS, INPUT);
    pinMode(SHCP, INPUT);
    pinMode(STCP, INPUT);
    pinMode(BUTTON, INPUT);
    pinMode(LED, INPUT);
}

void handle_signal(int signal) {
    running = 0;
    handle_cleanup();
}

int read_button() {
    static unsigned long last = 0;
    if (millis() - last > DEBOUNCE) {
        if (digitalRead(BUTTON) == LOW) {
            last = millis();
            return 1;
        }
    }

    return 0;
}

int main() {
    if(wiringPiSetup() == -1) {
        printf("wiringpi setup failed\n");
        return 1;
    }

    pinMode(LED, OUTPUT);
    pinMode(BUTTON, INPUT);
    pullUpDnControl(BUTTON, PUD_UP);
    
    pinMode(DS, OUTPUT);
    pinMode(SHCP, OUTPUT);
    pinMode(STCP, OUTPUT);

    digitalWrite(STCP, LOW);
    shiftOut(DS, SHCP, MSBFIRST, cleanup);
    digitalWrite(STCP, HIGH);
    digitalWrite(LED, LOW);

    signal(SIGINT, handle_signal);

    printf("Test Reaction Time\n");
    printf("Press Button when the LED lights up!\n");

    while (running) {

        for (int i = 0; i < 3; i++) {
            unsigned char val = segments[i];
            digitalWrite(STCP, LOW);
            shiftOut(DS, SHCP, MSBFIRST, val);
            digitalWrite(STCP, HIGH);
            delayMicroseconds(10);
            digitalWrite(STCP, LOW);
            delay(COUNTDOWN);
        }

        digitalWrite(LED, HIGH);
        unsigned long start = millis();
        digitalWrite(STCP, LOW);
        shiftOut(DS, SHCP, MSBFIRST, 0b11000000);
        digitalWrite(STCP, HIGH);
        delayMicroseconds(10);
        digitalWrite(STCP, LOW);

        while (read_button() == 0 && running) {
            delay(10);
        }


        unsigned long end = millis();
        unsigned long reaction = end - start;
        digitalWrite(LED, LOW);
        printf("Reaction time of %ld\n", reaction);

        delay(RESET);
    }

    printf("Stopped Program\n");
    handle_cleanup();

    return 0;
}
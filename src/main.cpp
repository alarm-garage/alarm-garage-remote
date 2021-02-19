#include <Arduino.h>
#include <RF24.h>
#include "printf.h"

#define AG_PIN_CE 10
#define AG_PIN_CSN 9
#define AG_PIN_BUTT 12

RF24 radio(AG_PIN_CE, AG_PIN_CSN);

const byte address[6] = "1tran";

void initRadio() {
    if (!radio.begin()) {
        Serial.println("Could not initialize radio!!!");
        for (int i = 0; i < 5; i++) {
            delay(200);
        }
    } else Serial.println("Radio initialized");
}

void setup() {
    Serial.begin(9600);
    delay(200);
    Serial.println("\n---\nStarting...\n");

    pinMode(AG_PIN_BUTT, INPUT);
    digitalWrite(AG_PIN_BUTT, HIGH);

    printf_begin();

    initRadio();
    radio.setAutoAck(true);
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(109);
    radio.openWritingPipe(address);
    radio.stopListening();
    radio.printDetails();
}

void loop() {
    bool pressed = digitalRead(AG_PIN_BUTT) == 0;

    if (pressed) {
        char concatenated_string[31];

        sprintf(concatenated_string, "t=%lu", millis());

        Serial.print("Sending data: ");
        Serial.println(concatenated_string);


        if (!radio.write(concatenated_string, sizeof(concatenated_string))) {
            Serial.println("Could not sent data!");
        }

        delay(100);
    }
}

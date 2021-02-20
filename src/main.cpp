#include <Arduino.h>
#include <LowPower.h>
#include <RF24.h>
#include "printf.h"

#define AG_PIN_CE 10
#define AG_PIN_CSN 9
#define AG_PIN_BUTT 3

RF24 radio(AG_PIN_CE, AG_PIN_CSN);

const byte address[6] = "1tran";

void wakeUp() {
    // no-op
}

bool initRadio() {
    if (!radio.begin()) {
        Serial.println("Could not initialize radio!!!");
        return false;
    }

    Serial.println("Radio initialized");

    radio.setAutoAck(true);
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(109);
    radio.openWritingPipe(address);
    radio.stopListening();
    radio.printDetails();

    return true;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n---\nStarting...\n");

    pinMode(AG_PIN_BUTT, INPUT);
    digitalWrite(AG_PIN_BUTT, HIGH);

    printf_begin();

    if (initRadio()) {
        Serial.println("Started!");
        delay(300); // to complete the output
    } else {
        // TODO
    }
}

void loop() {
    // Allow wake up pin to trigger interrupt on low.
    attachInterrupt(1, wakeUp, LOW);
    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    delay(50); // wait for the sleep

    // Disable external pin interrupt on wake up pin.
    detachInterrupt(1);

    char concatenated_string[31];

    sprintf(concatenated_string, "t=%lu", millis());

    Serial.print("Sending data: ");
    Serial.println(concatenated_string);

    if (!radio.write(concatenated_string, sizeof(concatenated_string))) {
        Serial.println("Could not sent data!");
    } else Serial.println("ACK");

    delay(100);
}

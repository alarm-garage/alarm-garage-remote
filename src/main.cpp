#include <Arduino.h>
#include <LowPower.h>
#include <EEPROM.h>

#include <RF24.h>
#include "printf.h"

#include <Crypto.h>
#include <Ascon128.h>
#include "Constants.h"

#include <proto/pb_encode.h>
#include <proto/alarm.pb.h>

RF24 radio(AG_PIN_CE, AG_PIN_CSN);
Ascon128 cipher;

const byte address[6] = "1tran";
const byte iv[AG_SIGNAL_IV_SIZE] = AG_SIGNAL_IV;

typedef struct ClientState {
    char clientId[5];
    byte key[AG_SIGNAL_KEY_SIZE];
    byte authdata[AG_SIGNAL_AUTH_SIZE];
    uint32_t lastCode;
} ClientState;

ClientState clientState;

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
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(109);
    radio.openWritingPipe(address);
    radio.stopListening();
    radio.printDetails();

    return true;
}

bool encrypt_signal(_protocol_RemoteSignal *output,
                    const byte authData[AG_SIGNAL_AUTH_SIZE],
                    const _protocol_RemoteSignalPayload payload) {
    cipher.clear();

    uint8_t pb_buffer[16];
    pb_ostream_t stream = pb_ostream_from_buffer(pb_buffer, sizeof(pb_buffer));

    if (!pb_encode(&stream, protocol_RemoteSignalPayload_fields, &payload)) {
        Serial.println("Could not encode the signal payload!");
        return false;
    }

    Serial.print("Signal payload encoded into ");
    size_t bytes = stream.bytes_written;
    Serial.print(bytes);
    Serial.print(" bytes\n");

    if (!cipher.setKey(clientState.key, AG_SIGNAL_KEY_SIZE)) {
        Serial.print("setKey ");
        return false;
    }
    if (!cipher.setIV(iv, AG_SIGNAL_IV_SIZE)) {
        Serial.print("setIV ");
        return false;
    }

    cipher.addAuthData(authData, AG_SIGNAL_AUTH_SIZE);
    cipher.encrypt(output->payload.bytes, pb_buffer, AG_SIGNAL_DATA_SIZE);
    output->payload.size = AG_SIGNAL_DATA_SIZE;
    cipher.computeTag(output->auth_tag.bytes, AG_SIGNAL_AUTH_TAG_SIZE);
    output->auth_tag.size = AG_SIGNAL_AUTH_TAG_SIZE;

    return true;
}

void sendSignal() {
    _protocol_RemoteSignal signal = protocol_RemoteSignal_init_zero;

    strcpy(signal.client_id, clientState.clientId);

    _protocol_RemoteSignalPayload payload = protocol_RemoteSignalPayload_init_zero;
    payload.code = ++clientState.lastCode;
    strcpy(reinterpret_cast<char *>(payload.random.bytes), "abcdefghij"); // TODO make truly random
    payload.random.size = 10;

    if (!encrypt_signal(&signal, clientState.authdata, payload)) {
        Serial.println("Could not encrypt signal payload!");
        return;
    }

    printf("Sending code %d\n", clientState.lastCode);

    uint8_t pb_buffer[64];
    pb_ostream_t stream = pb_ostream_from_buffer(pb_buffer, sizeof(pb_buffer));

    if (!pb_encode(&stream, protocol_RemoteSignal_fields, &signal)) {
        Serial.println("Could not encode the signal!");
        return;
    }

    size_t bytes = stream.bytes_written;

    EEPROM.put(0, clientState);

    if (!radio.write(pb_buffer, bytes)) {
        Serial.println("Could not sent data!");
    } else Serial.println("ACK");
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n---\nStarting...\n");

    pinMode(AG_PIN_BUTT, INPUT_PULLUP);
    digitalWrite(AG_PIN_BUTT, HIGH);

    printf_begin();

    if (initRadio()) {
        Serial.println("Started!");
        delay(300); // to complete the output
    } else {
        // TODO display
        return;
    }

    EEPROM.get(0, clientState);
}

void loop() {
    // TODO make this much better!
    while (digitalRead(AG_PIN_BUTT) == LOW); // prevent sending two events

    // Allow wake up pin to trigger interrupt on low.
    attachInterrupt(1, wakeUp, LOW);
    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    delay(50); // wait for the sleep

    // Disable external pin interrupt on wake up pin.
    detachInterrupt(1);

    sendSignal();

    delay(100);
}

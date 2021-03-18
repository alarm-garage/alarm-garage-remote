#include <Arduino.h>
#include <EEPROM.h>

#include <RF24.h>
#include "printf.h"

#include <Crypto.h>
#include <Ascon128.h>
#include "Constants.h"

#include <proto/pb_encode.h>
#include <proto/alarm.pb.h>
#include <proto/pb_decode.h>

RF24 radio(AG_PIN_CE, AG_PIN_CSN);
Ascon128 cipher;

const byte addressSend[6] = "1tran";
const byte addressReceive[6] = "2tran";
const byte iv[AG_SIGNAL_IV_SIZE] = AG_SIGNAL_IV;

typedef struct ClientState {
    char clientId[5];
    byte key[AG_SIGNAL_KEY_SIZE];
    byte authData[AG_SIGNAL_AUTH_SIZE];
    uint32_t lastCode;
} ClientState;

ClientState clientState;

void wakeUp() {
    // no-op
}

void fillWithRandom(byte *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] = random(1, 255);
    }
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
    radio.openWritingPipe(addressSend);
    radio.stopListening();
    radio.openReadingPipe(1, addressReceive);
    radio.printDetails();

    return true;
}

bool encryptSignalPayload(_protocol_RemoteSignal *output,
                          const byte *authData,
                          const _protocol_RemoteSignalPayload payload) {
    cipher.clear();

    uint8_t pb_buffer[protocol_RemoteSignalPayload_size];
    pb_ostream_t stream = pb_ostream_from_buffer(pb_buffer, sizeof(pb_buffer));

    if (!pb_encode(&stream, protocol_RemoteSignalPayload_fields, &payload)) {
        printf("Could not encode the signal payload: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    const size_t size = stream.bytes_written;

    if (!cipher.setKey(clientState.key, AG_SIGNAL_KEY_SIZE)) {
        return false;
    }
    if (!cipher.setIV(iv, AG_SIGNAL_IV_SIZE)) {
        return false;
    }

    cipher.addAuthData(authData, AG_SIGNAL_AUTH_SIZE);
    cipher.encrypt(output->payload.bytes, pb_buffer, size);
    output->payload.size = size;
    cipher.computeTag(output->auth_tag.bytes, AG_SIGNAL_AUTH_TAG_SIZE);
    output->auth_tag.size = AG_SIGNAL_AUTH_TAG_SIZE;

    return true;
}

bool decryptSignalResponse(byte *buff, const _protocol_RemoteSignalResponse *response) {
    cipher.clear();

    if (!cipher.setKey(clientState.key, AG_SIGNAL_KEY_SIZE)) {
        return false;
    }
    if (!cipher.setIV(iv, AG_SIGNAL_IV_SIZE)) {
        return false;
    }

    cipher.addAuthData(clientState.authData, AG_SIGNAL_AUTH_SIZE);
    cipher.decrypt(buff, response->payload.bytes, response->payload.size);

    return cipher.checkTag(response->auth_tag.bytes, AG_SIGNAL_AUTH_TAG_SIZE);
}

boolean waitForResponse(uint32_t expectedCode) {
    unsigned long start = millis();

    while (!radio.available() && millis() < start + AG_SIGNAL_RESPONSE_TIMEOUT) {}

    if (radio.available()) {
        const uint8_t size = radio.getDynamicPayloadSize();

        byte buff[size];
        radio.read(buff, size);

        pb_istream_t stream = pb_istream_from_buffer(buff, size);

        _protocol_RemoteSignalResponse signalResponse = protocol_RemoteSignalResponse_init_zero;

        if (!pb_decode(&stream, protocol_RemoteSignalResponse_fields, &signalResponse)) {
            printf("Decoding signal response failed: %s\n", PB_GET_ERROR(&stream));
            return false;
        }

        if (memcmp(clientState.clientId, signalResponse.client_id, 5) != 0) {
            Serial.println("Response has different ClientId than ours!");
            return false;
        }

        if (!decryptSignalResponse(buff, &signalResponse)) {
            Serial.println("Could not decrypt the response!");
            return false;
        }

        pb_istream_t stream2 = pb_istream_from_buffer(buff, signalResponse.payload.size); // size encrypted == size decrypted

        _protocol_RemoteSignalResponsePayload signalResponsePayload = protocol_RemoteSignalResponsePayload_init_zero;

        if (!pb_decode(&stream2, protocol_RemoteSignalResponsePayload_fields, &signalResponsePayload)) {
            printf("Decoding signal response payload failed: %s\n", PB_GET_ERROR(&stream2));
            return false;
        }

        return signalResponsePayload.code == expectedCode && signalResponsePayload.success;
    } else {
        Serial.println("Response wasn't received in a time!");
        return false;
    }
}

bool sendSignal() {
    _protocol_RemoteSignal signal = protocol_RemoteSignal_init_zero;

    strcpy(signal.client_id, clientState.clientId);

    _protocol_RemoteSignalPayload payload = protocol_RemoteSignalPayload_init_zero;
    payload.code = ++clientState.lastCode;
    fillWithRandom(payload.random.bytes, 6);
    payload.random.size = 6;

    if (!encryptSignalPayload(&signal, clientState.authData, payload)) {
        Serial.println("Could not encrypt signal payload!");
        return false;
    }

    printf("Sending code %lu\n", clientState.lastCode);

    uint8_t pb_buffer[AG_RADIO_MAX_PAYLOAD_SIZE];
    pb_ostream_t stream = pb_ostream_from_buffer(pb_buffer, sizeof(pb_buffer));

    if (!pb_encode(&stream, protocol_RemoteSignal_fields, &signal)) {
        printf("Could not encode the signal: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    const size_t size = stream.bytes_written;

    EEPROM.put(0, clientState);

    if (!radio.write(pb_buffer, size)) {
        Serial.println("Could not sent data!");
        return false;
    } else {
        radio.startListening(); // this won't be turned back... it will be powered off instead ;-)

        return waitForResponse(payload.code);
    }
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

    if (sendSignal()) {
        Serial.println("Everything allright!");
    } else {
        Serial.println("Some error has occurred!");
    }

    Serial.println("FINISH");

    // TODO power off
}

void loop() {
    // no-op
    delay(1000);
}

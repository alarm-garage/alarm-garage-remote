#ifndef ALARM_GARAGE_CONSTANTS_H
#define ALARM_GARAGE_CONSTANTS_H

#define AG_PIN_CE 10
#define AG_PIN_CSN 9
#define AG_PIN_BUTT 3

#define AG_RADIO_MAX_PAYLOAD_SIZE 32

#define AG_SIGNAL_AUTH_SIZE 4
#define AG_SIGNAL_AUTH_TAG_SIZE 8
#define AG_SIGNAL_KEY_SIZE 16
#define AG_SIGNAL_IV_SIZE 16
#define AG_SIGNAL_IV {0x91, 0x5f, 0xf8, 0xff, 0xca, 0xd8, 0xae, 0x1d, 0xf4, 0x45, 0xeb, 0x03, 0xe2, 0x18, 0xfd, 0x25}

#define AG_SIGNAL_RESPONSE_TIMEOUT 500

#endif //ALARM_GARAGE_CONSTANTS_H

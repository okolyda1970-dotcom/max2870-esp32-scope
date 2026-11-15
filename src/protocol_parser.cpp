#include "protocol_parser.h"

ProtocolParser parser;

void ProtocolParser::init() {
    rxIdx = 0;
    textBuf = "";
}

bool ProtocolParser::validateChecksum(uint8_t* buf, int len) {
    uint32_t calc = 0;
    for(int i=0; i<len; i++) calc += buf[i];
    uint32_t got;
    memcpy(&got, buf + len, 4);
    return calc == got;
}



ProtocolParser::EventType ProtocolParser::processInput() {
    while (Serial.available()) {
        uint8_t b = (uint8_t)Serial.read();

        // --- ОТЛАДКА: Каждая полученная буква/байт вызывает микро-вспышку ---
        // Если вы видите мерцание при отправке команды из Java -> Данные летят!
        digitalWrite(STATUS_LED, HIGH);
        delayMicroseconds(50); // Очень короткая вспышка
        digitalWrite(STATUS_LED, LOW);

        // 1. Проверка на STOP (приоритет №1)
        if (b == STOP_HEADER) {
            rxIdx = 0; 
            textBuf = ""; 
            // Специальная длинная вспышка для стопа
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            return EVENT_STREAM_STOP;
        }

        // 2. Текстовый режим
        if (rxIdx == 0) {
            if (b == '\n') {
                if (!textBuf.isEmpty()) {
                    lastTextCmd = textBuf;
                    textBuf = "";
                    // Двойная вспышка для текста
                    blinkLed(2);
                    return EVENT_TEXT_COMMAND;
                }
            } else {
                textBuf += (char)b;
            }
            continue;
        }

        // 3. Бинарный режим
        rxBuf[rxIdx++] = b;

        if (rxIdx == CTRL_SIZE) {
            if (validateChecksum(rxBuf, CTRL_SIZE - 4)) {
                memcpy(&lastCtrlPkt, rxBuf, sizeof(ControlPacket));
                rxIdx = 0;
                
                // Тройная яркая вспышка для успеха
                blinkLed(3); 
                
                return EVENT_CONTROL_PACKET;
            } else {
                // Тревожная серия вспышек для ошибки чек-суммы
                for(int i=0; i<5; i++) {
                    digitalWrite(STATUS_LED, HIGH);
                    delayMicroseconds(50);
                    digitalWrite(STATUS_LED, LOW);
                    delayMicroseconds(50);
                }
                rxIdx = 0; // Сброс буфера
            }
        }
    }
    return EVENT_NONE;
}

// ЕДИНСТВЕННОЕ ОПРЕДЕЛЕНИЕ blinkLed
void ProtocolParser::blinkLed(int count) {
    for(int i=0; i<count; i++) {
        digitalWrite(STATUS_LED, HIGH);
        delay(50); // Используем delay(50ms), чтобы вспышки были видимыми глазу
        digitalWrite(STATUS_LED, LOW);
        delay(50);
    }
}
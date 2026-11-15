#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include "config.h"

class ProtocolParser {
public:
    enum EventType {
        EVENT_NONE,             // Ничего нового
        EVENT_CONTROL_PACKET,   // Получен бинарный пакет 0xAA...
        EVENT_TEXT_COMMAND,     // Получена текстовая команда (SWEEP_START...)
        EVENT_STREAM_STOP       // Получен байт 0xDD (Стоп)
    };

    void init();
    
    // Главный метод обработки ввода. Вызывается из loop()
    EventType processInput();

    // Геттеры для доступа к данным после получения события
    ControlPacket getControlPacket() const { return lastCtrlPkt; }
    String getTextCommand() const { return lastTextCmd; }

private:
    uint8_t rxBuf[CTRL_SIZE];
    uint8_t rxIdx = 0;
    String textBuf = "";
    
    ControlPacket lastCtrlPkt;
    String lastTextCmd;

    bool validateChecksum(uint8_t* buf, int len);
    
    // --- ДОБАВЛЕНО ЭТО ОБЪЯВЛЕНИЕ ---
    void blinkLed(int count); 
};

extern ProtocolParser parser;

#endif
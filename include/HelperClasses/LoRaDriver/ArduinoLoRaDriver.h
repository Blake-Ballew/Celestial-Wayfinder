#include "LoraDriverInterface.h"
#include <LoRa.h>

class ArduinoLoRaDriver : public LoraDriverInterface
{
public:
    ArduinoLoRaDriver(SPIClass *spi, int cs, int reset, int dio0, uint32_t loraFrequency, uint32_t spiFrequency = 1000000) :
        _spi(spi),
        _spiFrequency(spiFrequency),
        _cs(cs),
        _reset(reset),
        _dio0(dio0),
        _loraFrequency(loraFrequency)
    {

    }

    bool Init()
    {
        if (_spi == nullptr)
        {
            return false;
        }

        #if DEBUG == 1
        Serial.println("Initializing LoRa...");
        Serial.print("CS: ");
        Serial.println(_cs);
        Serial.print("RESET: ");
        Serial.println(_reset);
        Serial.print("DIO0: ");
        Serial.println(_dio0);
        #endif

        LoRa.setPins(_cs, _reset, _dio0);
        LoRa.setSPI(*_spi);

        #if DEBUG == 1
        Serial.print("Beginning LoRa...");
        #endif
        auto result = LoRa.begin(_loraFrequency) == 1;

        #if DEBUG == 1
        if (result)
        {
            Serial.println("Success");
        }
        else
        {
            Serial.println("Failed");
        }
        #endif

        LoRa.setSpreadingFactor(7);
        LoRa.setSignalBandwidth(125E3);
        LoRa.setSPIFrequency(_spiFrequency);
        return result;
    }

    bool ReceiveMessage(JsonDocument &doc, size_t timeout = 0)
    {
        auto startTime = xTaskGetTickCount();

        do
        {
            size_t msgSize = 0;
            uint8_t buffer[256];
            memset(buffer, 0, sizeof(buffer));

            if (LoRa.parsePacket() == 0) continue;

            while (LoRa.available())
            {
                buffer[msgSize] = (uint8_t)(LoRa.read() & 0xFF);
                msgSize++;
            }

            if (msgSize == 0) continue;

            #if DEBUG == 1
            Serial.print("Message of length ");
            Serial.print(msgSize);
            Serial.print(" received: ");
            for (auto i = 0; i < msgSize; i++)
            {
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            #endif

            auto result = deserializeMsgPack(doc, (const uint8_t *)buffer, sizeof(buffer));

#if DEBUG == 1
            Serial.print("Deserialization result: ");
            Serial.println(result.code());
#endif
            return result.code() == DeserializationError::Ok;
        }
        while ((xTaskGetTickCount() - startTime) < timeout);

        return false;
    }

    bool SendMessage(JsonDocument &doc)
    {
        auto msgSize = measureMsgPack(doc);
        // uint8_t buffer[msgSize];
        // memset(buffer, 0, msgSize);

        // serializeMsgPack(doc, buffer, sizeof(buffer));

        #if DEBUG == 1
        Serial.print("Sending message: ");
        serializeJson(doc, Serial);
        Serial.println();
        #endif

        if (LoRa.beginPacket())
        {
            serializeMsgPack(doc, LoRa);
            return LoRa.endPacket() == 1;
        }
        
        return false;
    }

    void SetTXPower(int txPower)
    {
        LoRa.setTxPower(txPower);
    }

    void SetFrequency(uint32_t frequency)
    {
        LoRa.setFrequency(frequency);
        _loraFrequency = frequency;
    }

    void SetSpreadingFactor(int sf)
    {
        LoRa.setSpreadingFactor(sf);
    }

    void SetSignalBandwidth(uint32_t sbw)
    {
        LoRa.setSignalBandwidth(sbw);
    }

protected:
    SPIClass *_spi = nullptr;
    uint32_t _spiFrequency;

    int _cs;
    int _reset;
    int _dio0;

    uint32_t _loraFrequency;

};
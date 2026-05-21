#include "LoraDriverInterface.h"
#include <LoRa.h>
#include <esp_log.h>

static const char *TAG_LORA = "LORA";

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
        ESP_LOGI(TAG_LORA, "Initializing LoRa...");
        if (_spi == nullptr)
        {
            ESP_LOGW(TAG_LORA, "No valid Spi bus detected");
            return false;
        }

        ESP_LOGI(TAG_LORA, "CS: %d", _cs);
        ESP_LOGI(TAG_LORA, "RESET: %d", _reset);
        ESP_LOGI(TAG_LORA, "DIO0: %d", _dio0);

        LoRa.setPins(_cs, _reset, _dio0);
        LoRa.setSPI(*_spi);

        ESP_LOGI(TAG_LORA, "Beginning LoRa...");

        auto result = LoRa.begin(_loraFrequency) == 1;

        if (result)
        {
            ESP_LOGI(TAG_LORA, "Success");
        }
        else
        {
            ESP_LOGE(TAG_LORA, "Failed");
        }

        LoRa.setCodingRate4(8);
        LoRa.setSpreadingFactor(7);
        LoRa.setSignalBandwidth(125E3);
        LoRa.setSPIFrequency(_spiFrequency);
        return result;
    }

    bool ReceiveMessage(uint8_t* buffer, size_t& outLen, size_t timeout) override
    {
        auto startTime = xTaskGetTickCount();

        do
        {
            if (LoRa.parsePacket() == 0) { continue; }

            outLen = 0;
            while (LoRa.available())
            {
                buffer[outLen++] = (uint8_t)(LoRa.read() & 0xFF);
            }

            return outLen > 0;
        }
        while ((xTaskGetTickCount() - startTime) < timeout);

        return false;
    }

    bool SendMessage(const uint8_t* buffer, size_t len) override
    {
        if (LoRa.beginPacket())
        {
            LoRa.write(buffer, len);
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
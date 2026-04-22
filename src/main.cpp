#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "EventDeclarations.h"
#include <ESP32Encoder.h>
#include "globalDefines.h"

#include <ArduinoJson.hpp>

#include "LoraManager.h"
#include "NavigationManager.h"

#include "CompassUtils.h"

#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "RingPoint.hpp"
#include "IlluminateButton.hpp"
#include "RingPulse.hpp"

#include "MessagePing.h"

#include <unordered_map>

#if HARDWARE_VERSION == 1
    #include "Bootstrap/V1/BootstrapMicrocontroller.hpp"
    #include "Bootstrap/V1/BootstrapLeds.hpp"
    #include "Bootstrap/V1/BootstrapNavigation.hpp"
    #include "Bootstrap/V1/BootstrapDisplay.hpp"
#elif HARDWARE_VERSION == 2
    #include "Bootstrap/V2/BootstrapMicrocontroller.hpp"
    #include "Bootstrap/V2/BootstrapLeds.hpp"
    #include "Bootstrap/V2/BootstrapNavigation.hpp"
    #include "Bootstrap/V2/BootstrapDisplay.hpp"
#elif HARDWARE_VERSION == 3
    #include "Bootstrap/V3/BootstrapMicrocontroller.hpp"
    #include "Bootstrap/V3/BootstrapLeds.hpp"
    #include "Bootstrap/V3/BootstrapNavigation.hpp"
    #include "Bootstrap/V3/BootstrapDisplay.hpp"
#else
    #error "Unknown HARDWARE_VERSION. Must be 1, 2, or 3."
#endif

extern "C"
{
#include "bootloader_random.h"
}

namespace
{
  const uint8_t CPU_CORE_LORA = 1;
  const uint8_t CPU_CORE_APP = 0;
  const uint8_t RF95_TX_PWR = 20;

  #if HARDWARE_VERSION < 3
  const uint8_t LORA_CS = 15;
  const uint8_t LORA_RST = -1;
  const uint8_t LORA_DIO0 = 18;
  #else
  const uint8_t LORA_CS = 39;
  const uint8_t LORA_RST = 38;
  const uint8_t LORA_DIO0 = 48;
  const uint8_t LORA_SCK = 40;
  const uint8_t LORA_MISO = 42;
  const uint8_t LORA_MOSI = 41;

  #endif
}

#if HARDWARE_VERSION < 3
SPIClass loraSpi(HSPI);
#else
SPIClass loraSpi;
#endif

ArduinoLoRaDriver CompassUtils::ArduinoLora(&loraSpi, LORA_CS, LORA_RST, LORA_DIO0, 915E6);
LoraManager loraManager(&CompassUtils::ArduinoLora);

void enableInterruptsHandler();
void disableInterruptsHandler();
void enterUselessLoop();
void Bootstrap();

void setup()
{
  Serial.begin(115200);
  vTaskDelay(1000);

  bootloader_random_enable();
  
  // Boostrap hardware modules and utilities
  Bootstrap();
  
  vTaskDelay(300);

  // Initialize Lora Module
  #if HARDWARE_VERSION == 3
  loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  #endif
  auto success = loraManager.Init();

  if (!success)
  {
    ESP_LOGE(TAG, "Failed to initialize Lora module");
  }

  System_Utils::init();

  displayCommandQueue = DisplayModule::Utilities::getDisplayCommandQueue();

  // Register message types
  ESP_LOGI(TAG, "Registering message types");
  MessageBase::SetMessageType(0x01);
  MessagePing::SetMessageType(0x02);

  LoraUtils::RegisterMessageDeserializer(MessageBase::MessageType(), MessageBase::MessageFactory);
  LoraUtils::RegisterMessageDeserializer(MessagePing::MessageType(), MessagePing::MessageFactory);

  CompassUtils::InitializeDisplayManager();
  // System_Utils::registerTask(Display_Manager::processCommandQueue, "displayTask", 12000, nullptr, 2, CPU_CORE_APP);

  // Bind the radio send and receive tasks and then register them
  ESP_LOGI(TAG, "Registering radio tasks");

  int radioTaskID = System_Utils::registerTask(CompassUtils::BoundRadioTask, "radio-task", 4096, &loraManager, 3, CPU_CORE_LORA);
  int sendQueueTaskID = System_Utils::registerTask(CompassUtils::BoundSendQueueTask, "send-queue-task", 4096, &loraManager, 2, CPU_CORE_LORA);

  LoraUtils::MessageReceived() += CompassUtils::PassMessageReceivedToDisplay;

  // Initialize RPC
  CompassUtils::InitializeRpc(1, CPU_CORE_LORA);

  CompassUtils::WireFunctions();

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  System_Utils::getEnableInterrupts() += enableInterruptsHandler;
  System_Utils::getDisableInterrupts() += disableInterruptsHandler;

  System_Utils::enableInterruptsInvoke();

}

void loop()
{
  vTaskDelay(600000 / portTICK_PERIOD_MS);
}

void enableInterruptsHandler() 
{
  // attachInterrupt(BUTTON_SOS_PIN, buttonSOSISR, FALLING);
  attachInterrupt(BUTTON_1_PIN, button1ISR, FALLING);
  attachInterrupt(BUTTON_2_PIN, button2ISR, FALLING);
  attachInterrupt(BUTTON_3_PIN, button3ISR, FALLING);
  attachInterrupt(BUTTON_4_PIN, button4ISR, FALLING);
  inputEncoder->resumeCount();
}

void disableInterruptsHandler() 
{
  detachInterrupt(BUTTON_SOS_PIN);
  detachInterrupt(BUTTON_1_PIN);
  detachInterrupt(BUTTON_2_PIN);
  detachInterrupt(BUTTON_3_PIN);
  detachInterrupt(BUTTON_4_PIN); 
  inputEncoder->pauseCount();
}

void enterUselessLoop()
{
  auto counter = 0;

  while(true)
  {
    ESP_LOGI("SETUP", "Infinite loop: %d\n", counter);
    vTaskDelay(3000);
  }
}

void Bootstrap()
{
  ESP_LOGI(TAG, "Initializing Hardware Version %d", HARDWARE_VERSION);

  CompassUtils::InitializeSettings();

  BootstrapMicrocontroller::Initialize();
  BootstrapLeds::Initialize();
  BootstrapNavigation::Initialize();
  BootstrapDisplay::Inititalize();
}


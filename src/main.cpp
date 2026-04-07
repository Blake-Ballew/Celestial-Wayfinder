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

#include "HelperClasses/Compass/QMC5883L.h"
#include "HelperClasses/Compass/LSM303AGR.h"

#include "TinyGPS++.h"

#include "ScrollWheel.hpp"
#include "SolidRing.hpp"
#include "RingPoint.hpp"
#include "IlluminateButton.hpp"
#include "RingPulse.hpp"

#include "MessagePing.h"

#include <unordered_map>

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

ESP32Encoder encoder(true, enc_cb);

ArduinoLoRaDriver CompassUtils::ArduinoLora(&loraSpi, LORA_CS, LORA_RST, LORA_DIO0, 915E6);
LoraManager loraManager(&CompassUtils::ArduinoLora);

// Navigation Objects
CompassInterface *compass;
NavigationManager navigationManager;

// Filesytstem Manager. May not even need this

void enableInterruptsHandler();
void disableInterruptsHandler();
void enterUselessLoop();

void setup()
{
  Serial.begin(115200);
  vTaskDelay(1000);
  ESP_LOGI(TAG, "Initializing Hardware Version %d", HARDWARE_VERSION);


  bootloader_random_enable();

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
#if HARDWARE_VERSION == 1 || HARDWARE_VERSION == 2
  pinMode(BUTTON_4_PIN, INPUT);
#else
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);
#endif
  pinMode(BUZZER_PIN, OUTPUT);

#if HARDWARE_VERSION < 3
  auto BATT_SENSE_PIN = 39;
  pinMode(BATT_SENSE_PIN, INPUT);
#endif

#if HARDWARE_VERSION == 1
  auto KEEP_ALIVE_PIN = 5;
  pinMode(KEEP_ALIVE_PIN, OUTPUT);  
  digitalWrite(KEEP_ALIVE_PIN, HIGH);
#endif

  encoder.attachFullQuad(ENC_A, ENC_B);
  encoder.setFilter(1023);
  encoder.setCount(0);
  
  CompassUtils::InitializeSettings();

  // enterUselessLoop();

  // Initialize LED Module
  CompassUtils::InitializeLedManager(CPU_CORE_APP);

  vTaskDelay(300);

  #if HARDWARE_VERSION == 3
  auto wireSuccess = Wire.begin(18, 17);
  if (wireSuccess)
  {
    ESP_LOGI(TAG, "Successfully initialized display I2C");
  }
  else
  {
    ESP_LOGW(TAG, "Failed to init display I2C");
  }
  #endif

  // Intialize Navigation Module
  // Initialize Compass
#if HARDWARE_VERSION == 1
#if DEBUG == 1
  ESP_LOGI(TAG, "Using QMC5883L");
#endif
  QMC5883L *QMC5883Lcompass = new QMC5883L();
  QMC5883Lcompass->SetInvertX(true);

  compass = QMC5883Lcompass;
#endif
#if HARDWARE_VERSION == 2
#if DEBUG == 1
  ESP_LOGI(TAG, "Using LSM303AGR");
#endif
  compass = new LSM303AGR();
#endif

  ESP_LOGI(TAG, "Initializing Navigation Manager");

  // Initialize GPS Stream
  #if HARDWARE_VERSION == 3
  Serial2.setPins(5, 4);
  #endif
  Serial2.begin(9600);
  navigationManager.InitializeUtils(compass, Serial2);

  // Initialize Lora Module
  #if HARDWARE_VERSION == 3
  loraSpi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  #endif
  auto success = loraManager.Init();

#if DEBUG == 1
  if (!success)
  {
    ESP_LOGE(TAG, "Failed to initialize Lora module");
  }
#endif

  CompassUtils::InitializeDisplayManager();

  System_Utils::init();


  // Initialize modules
  CompassUtils::Bootstrap();

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

  // Register edits for Lora
  
  // CompassUtils::driver.spiWrite(0x1F, 0xFF); //increase RX timeout

  #if HARDWARE_VERSION == 2
  // CompassUtils::driver.spiWrite(0x0B, 0x0B);
  // CompassUtils::driver.spiWrite(0x0C, 0x20);
  // CompassUtils::driver.spiWrite(0x22, 0x52);
  // CompassUtils::driver.spiWrite(0x0D, 0x52);
  // CompassUtils::driver.spiWrite(0x1F, 0x64);
  #endif

#if DEBUG == 1
  for (int i = 0; i <= 0x39; i++)
  {
    // ESP_LOGD(TAG, "0x%02X: 0x%02X", i, CompassUtils::driver.spiRead(i));
  }
#endif

#if DEBUG == 1
  // FilesystemUtils::PrintSettingsFile();
#endif

  inputEncoder = &encoder;

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


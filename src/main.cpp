#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_base.h"
#include "EventDeclarations.h"
#include <ESP32Encoder.h>
#include "globalDefines.h"

#include "Display_Manager.h"
#include "Settings_Manager.h"
#include "LoraManager.h"
#include "NavigationManager.h"
#include "FilesystemManager.h"

#include "CompassUtils.h"
#include "HelperClasses/Compass/QMC5883L.h "
#include "TinyGPS++.h"

#include "ScrollWheel.h"
#include "SolidRing.h"
#include "RingPoint.h"
#include "Illuminate_Button.h"
#include "Ring_Pulse.h"

extern "C"
{
#include "bootloader_random.h"
}

#if DEBUG == 1
TaskHandle_t debugInputTaskHandle;
void sendDebugInputs(void *pvParameters);
#endif

namespace
{
  const uint8_t CPU_CORE_LORA = 1;
  const uint8_t CPU_CORE_APP = 0;
  const uint8_t RFM95_CS = 15;
  const uint8_t RFM95_Int = 18;
  const uint8_t RF95_TX_PWR = 20;
}

ESP32Encoder encoder(true, enc_cb);

// esp_event_loop_handle_t loop_handle;



RHHardwareSPI rh_spi;
RH_RF95 CompassUtils::driver(RFM95_CS, RFM95_Int, rh_spi);
LoraManager<RH_RF95> loraManager(&CompassUtils::driver, RFM95_CS, RFM95_Int, RF95_TX_PWR);

// Navigation Objects
QMC5883L *compass;
NavigationManager navigationManager;

// Filesytstem Manager. May not even need this
FilesystemManager filesystemManager;

void enableInterruptsHandler();
void disableInterruptsHandler();

void setup()
{
#if DEBUG == 1
  Serial.begin(115200);
  Serial.println("Initializing");
#endif

  bootloader_random_enable();

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(BUTTON_SOS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT);
  pinMode(KEEP_ALIVE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BATT_SENSE_PIN, INPUT);

  digitalWrite(KEEP_ALIVE_PIN, HIGH);

  encoder.attachFullQuad(ENC_A, ENC_B);
  encoder.setFilter(1023);
  encoder.setCount(0);

  // Initialize Filesystem and settings
  filesystemManager.Init();
  CompassUtils::InitializeSettings();

  // Initialize LED Module
  LED_Manager::init(NUM_LEDS, CPU_CORE_APP);

  // Intialize Navigation Module
  compass = new QMC5883L();
  compass->SetInvertX(true);
  Serial2.begin(9600);
  navigationManager.InitializeUtils(compass, Serial2);

  // Initialize Lora Module
  auto success = loraManager.Init();
  

  // TODO remove home window from here
  Display_Manager::init();
  CompassUtils::RegisterCallbacksDisplayManager(nullptr);

  System_Utils::init();

  

  // Initialize inputID to LED index mapping
  std::unordered_map<uint8_t, uint8_t> inputIdLedIdx = {
    {BUTTON_1, 22},
    {BUTTON_2, 19},
    {BUTTON_3, 18},
    {BUTTON_4, 17},
    {ENC_UP, 20},
    {ENC_DOWN, 21},
    {BUTTON_SOS, 16},
  };

  Serial.println("Initializing LED pins");
  LED_Manager::InitializeInputIdLedPins(inputIdLedIdx);
  LED_Manager::initializeButtonFlashAnimation();

  // Initialize other animations
  ScrollWheel *scrollWheel = new ScrollWheel();
  SolidRing *solidRing = new SolidRing();
  RingPoint *ringPoint = new RingPoint();
  Illuminate_Button *IlluminateButton = new Illuminate_Button(inputIdLedIdx);
  Ring_Pulse *ringPulse = new Ring_Pulse();

  LED_Utils::registerPattern(scrollWheel);
  LED_Utils::registerPattern(solidRing);
  LED_Utils::registerPattern(ringPoint);
  LED_Utils::registerPattern(IlluminateButton);
  LED_Utils::registerPattern(ringPulse);

  StaticJsonDocument<128> cfg;
  cfg["beginIdx"] = 0;
  cfg["endIdx"] = 15;

  scrollWheel->configurePattern(cfg);
  solidRing->configurePattern(cfg);
  ringPoint->configurePattern(cfg);
  ringPulse->configurePattern(cfg);

  displayCommandQueue = Display_Manager::getDisplayCommandQueue();

  // Register message types
  Serial.println("Registering message types");
  MessageBase::SetMessageType(0x01);
  MessagePing::SetMessageType(0x02);

  LoraUtils::RegisterMessageDeserializer(MessageBase::MessageType(), MessageBase::MessageFactory);
  LoraUtils::RegisterMessageDeserializer(MessagePing::MessageType(), MessagePing::MessageFactory);

  System_Utils::registerTask(Display_Manager::processCommandQueue, "displayTask", 12000, nullptr, 2, CPU_CORE_APP);

  // Bind the radio send and receive tasks and then register them
  Serial.println("Registering radio tasks");
  auto boundSendTask = [](void *pvParameters) {
    LoraManager<RH_RF95> *manager = (LoraManager<RH_RF95> *)pvParameters;
    manager->SendTask(pvParameters);
  };

  auto boundReceiveTask = [](void *pvParameters) {
    LoraManager<RH_RF95> *manager = (LoraManager<RH_RF95> *)pvParameters;
    manager->ReceiveTask(pvParameters);
  };

  System_Utils::registerTask(boundSendTask, "radioSend", 4096, &loraManager, 1, CPU_CORE_LORA);
  System_Utils::registerTask(boundReceiveTask, "radioReceive", 4096, &loraManager, 2, CPU_CORE_LORA);

  LoraUtils::MessageReceived() += CompassUtils::PassMessageReceivedToDisplay;

#if DEBUG == 1
  // System_Utils::registerTask(sendDebugInputs, "debugInputTask", 1024, nullptr, 1, 0);
  // xTaskCreate(sendDebugInputs, "debugInputTask", 8192, NULL, 1, &debugInputTaskHandle);
#endif

#if DEBUG == 1
  serializeJson(FilesystemUtils::SettingsFile(), Serial);
  Serial.println();
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
  attachInterrupt(BUTTON_SOS_PIN, buttonSOSISR, FALLING);
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

#if DEBUG == 1
// Takes in a numeric input over serial, shifts a bit by that much, and sends it to the inputTaskHandle
void sendDebugInputs(void *pvParameters)
{
  int input;
  while (1)
  {
    if (Serial.available() > 0)
    {
      input = Serial.parseInt();
      Serial.println();
      Serial.printf("Passing in input: %d\n", input);
      Serial.println();
      DisplayCommandQueueItem command;
      command.commandType = INPUT_COMMAND;
      command.commandData.inputCommand.inputID = input;
      xQueueSend(displayCommandQueue, &command, portMAX_DELAY);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
#endif



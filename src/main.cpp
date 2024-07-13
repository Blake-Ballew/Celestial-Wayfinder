#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_base.h"
#include "EventDeclarations.h"
#include <ESP32Encoder.h>
#include "Display_Manager.h"
#include "globalDefines.h"
#include "Settings_Manager.h"
#include "Network_Manager.h"
#include <FastLED.h>

extern "C"
{
#include "bootloader_random.h"
}

#if DEBUG == 1
TaskHandle_t debugInputTaskHandle;
void sendDebugInputs(void *pvParameters);
#endif

#define DEBUG 1

ESP32Encoder encoder(true, enc_cb);

// esp_event_loop_handle_t loop_handle;

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

  Settings_Manager::init();
  LED_Manager::init();
  Navigation_Manager::init();
  Network_Manager::init();
  Display_Manager::init();
  System_Utils::init();

  displayCommandQueue = Display_Manager::getDisplayCommandQueue();

  System_Utils::registerTask(Display_Manager::processCommandQueue, "displayTask", 8192, nullptr, 1, 0);
  System_Utils::registerTask(Network_Manager::listenForMessages, "radioTask", 8192, nullptr, 1, 1);

#if DEBUG == 1
  System_Utils::registerTask(sendDebugInputs, "debugInputTask", 1024, nullptr, 1, 1);
  // xTaskCreate(sendDebugInputs, "debugInputTask", 8192, NULL, 1, &debugInputTaskHandle);
#endif

#if DEBUG == 1
  Settings_Manager::writeSettingsToSerial();
  Serial.println();
  Settings_Manager::writeMessagesToSerial();
  Serial.println();
  Settings_Manager::writeCoordsToSerial();
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


  vTaskDelay(60000 / portTICK_PERIOD_MS);
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
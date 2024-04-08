#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_base.h"
#include "EventDeclarations.h"
#include <ESP32Encoder.h>
#include "OLED_Manager.h"
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
  Network_Manager::init(&inputTaskHandle);
  OLED_Manager::init();
  System_Utils::init(&OLED_Manager::display);

  // Register interrupt flags to inputIDs
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_BUTTON_1), BUTTON_1);
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_BUTTON_2), BUTTON_2);
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_BUTTON_3), BUTTON_3);
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_BUTTON_4), BUTTON_4);
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_ENCODER_UP), ENC_UP);
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_ENCODER_DOWN), ENC_DOWN);
  OLED_Manager::registerInput(BIT_SHIFT((uint32_t)EVENT_MESSAGE_RECEIVED), MESSAGE_RECEIVED);

  xTaskCreatePinnedToCore(OLED_Manager::processButtonPressEvent, "inputTask", 8192, NULL, 1, &inputTaskHandle, 0);
  xTaskCreatePinnedToCore(Network_Manager::listenForMessages, "radioTask", 8192, NULL, 1, &radioReadTaskHandle, 1);
#if DEBUG == 1
  xTaskCreate(sendDebugInputs, "debugInputTask", 8192, NULL, 1, &debugInputTaskHandle);
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

  attachInterrupt(BUTTON_SOS_PIN, buttonSOSISR, FALLING);
  attachInterrupt(BUTTON_1_PIN, button1ISR, FALLING);
  attachInterrupt(BUTTON_2_PIN, button2ISR, FALLING);
  attachInterrupt(BUTTON_3_PIN, button3ISR, FALLING);
  attachInterrupt(BUTTON_4_PIN, button4ISR, FALLING);

  /*vTaskDelay(5000 / portTICK_PERIOD_MS);
  uint32_t notification;
  notification = BIT_SHIFT((uint32_t)EVENT_BUTTON_1);
  xTaskNotify(inputTaskHandle, notification, eSetBits);*/

  // Network_Manager::listenForMessages(nullptr);
}

void loop()
{
  /*uint32_t notification;
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  notification = BIT_SHIFT((uint32_t)EVENT_ENCODER_DOWN);
  xTaskNotify(inputTaskHandle, notification, eSetBits);*/
  // Serial.print("Enc A: ");
  // Serial.print(digitalRead(ENC_A));
  // Serial.print(" Enc B: ");
  // Serial.println(digitalRead(ENC_B));

  vTaskDelay(60000 / portTICK_PERIOD_MS);
}

#if DEBUG == 1
// Takes in a numeric input over serial, shifts a bit by that much, and sends it to the inputTaskHandle
void sendDebugInputs(void *pvParameters)
{
  uint32_t notification;
  int input;
  while (1)
  {
    if (Serial.available() > 0)
    {
      input = Serial.parseInt();
      notification = BIT_SHIFT(input - 1);
      Serial.println();
      Serial.printf("Passing in input: %d\n", input);
      Serial.println();
      xTaskNotify(inputTaskHandle, notification, eSetBits);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
#endif
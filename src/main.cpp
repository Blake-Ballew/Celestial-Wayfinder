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

  xTaskCreatePinnedToCore(OLED_Manager::processButtonPressEvent, "inputTask", 8192, NULL, 1, &inputTaskHandle, 0);
  xTaskCreatePinnedToCore(Network_Manager::listenForMessages, "radioTask", 8192, NULL, 1, &radioReadTaskHandle, 1);

#if DEBUG == 1
  Settings_Manager::writeSettingsToSerial();
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

/*
extern "C"
{
  void app_main(void);
}

void app_main()
{
#if DEBIG == 1
  Serial.begin(115200);
#endif
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);

  encoder.attachFullQuad(ENC_A, ENC_B);
  encoder.setCount(0);
  OLED_Manager::init();
  xTaskCreate(OLED_Manager::processButtonPressEvent, "inputTask", 4096, NULL, 1, &inputTaskHandle);

  attachInterrupt(BUTTON_2, button2ISR, FALLING);
  attachInterrupt(BUTTON_3, button3ISR, FALLING);
  attachInterrupt(BUTTON_4, button4ISR, FALLING);
}
*/
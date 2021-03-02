#include <Arduino.h>
#include <Sim800nb.hpp>
#include <SoftwareSerial.h>

#define PIN_RELAY_1 8
#define RELAY_ON false
#define RELAY_OFF true

#define PHONE_WHITE_LIST_1 "+79991112233"
#define COMMAND_RELAY_ON_1 "ON_1"
#define COMMAND_RELAY_OFF_1 "OFF_1"
#define COMMAND_GET_STATUS "STATUS"

Sim800nb sim;
SoftwareSerial soft(3, 2); // (Rx, Tx)


String get_unit_status()
{
  String result = "Unit status:";
  result.concat(F("\nRelay1="));
  result.concat((digitalRead(pin) == RELAY_ON) ? "ON" : "OFF");
  return result;
}

void got_sms(String phone, String text)
{
  // check if phone is in white list
  if (phone.equals(F(PHONE_WHITE_LIST_1)))
  {
    text.toUpperCase();

    if (text.indexOf(F(COMMAND_RELAY_ON_1)) > -1)
    {
      digitalWrite(PIN_RELAY_1, RELAY_ON);
      Serial.print(F("Relay1=ON \n"));
    }
    else if (text.indexOf(F(COMMAND_RELAY_OFF_1)) > -1)
    {
      digitalWrite(PIN_RELAY_1, RELAY_OFF);
      Serial.print(F("Relay1=OFF \n"));
    }

    if (text.indexOf(F(COMMAND_GET_STATUS)) > -1)
    {
      String result = get_unit_status();
      sim.push_command(CommandType::PHONE, PHONE_WHITE_LIST_1);
      sim.push_command(CommandType::SEND, result.c_str());
    }
  }
}

void setup()
{
  Serial.begin(9600);
  TaskManager::begin();
  sim.begin(&soft);
  soft.begin(9600);

  pinMode(PIN_RELAY_1, OUTPUT);
  digitalWrite(PIN_RELAY_1, RELAY_OFF);

  sim.subscribe_sms(got_sms);
  sim.push_command_wait();
  sim.push_command(CommandType::PHONE, PHONE_WHITE_LIST_1);
  sim.push_command(CommandType::SEND, "Unit just started");
}

void loop()
{
  TaskManager::do_tasks();
}
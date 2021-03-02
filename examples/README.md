# SMS relay
GSM relay is based on SIM800L module. 

The example shows how to manage 1x relay module by SMS.
You need:
- Arduino Uno
- SIM800L module
- Relay module
- DC-DC convector for supply SIM800L by 3.7V
- 2 resistors 1kOm - for connection Rx pin of SIM800L to Tx from Aквгштщ

Sim800L is connected to SoftSerial on Arduino. white-phone-list and sms command signatures could be adjusted in constants. 
In example SMS relay is accepting command from defined phone number only. There are three commands implemented:
- **on_1**  - switch on relay
- **off_1** - switch off relay
- **status** - send comeback sms message with relay status

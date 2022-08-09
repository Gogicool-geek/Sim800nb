# Sim800nb + SMS Relay
Library provides SMS handling ability in non-blocking way.

SIM800L module is controlled by pushing command to internal queue, for example:

    Sim800nb sim;
    ..
    sim.push_command(CommandType::PHONE, "+79991112233");
    sim.push_command(CommandType::SEND, "Hello World!);

There are a number commands types available:
* *WAIT* -  stop to read command queue for period of time. Sometimes it is needed to obtain result from SIM800L. Especaily if you send raw command to module and getting result takes time, therefore push "READ" command right after "WAIT". The ducation can be ajusted in constant TIMEOUT_LONG

* *WRITE* - write raw command directly to SIM module (command should be in argument). By default after every WRITE command come short delay. the duration of short delay could be specify in constant TIMEOUT_SHORT

- *SEND* -  send sms (text is in arg. phone should be sent in advace. see PHONE command)

- *READ* -  read sms with particular number (sms number is in argument.), but better to subscribe to event when a new sms come. Please see function subscribe_sms()

- *PHONE* - set default phone (phone is in arg.)

- *CHECK* - check if stored sms available. If they is found, the first one will be proceed and send to subscriber

- *CLEAN* - delete all rod incoming sms and all outgoing sms

- *DELETE* - delete particular sms (number is in argument)


In order to know when a new sms come, you need to subscribe, e.g.:

    sim.subscribe_sms(callback_func);

where callback function has following singnature:

    // @param first phone number
    // @param second sms text
    typedef void (*SmsCallBackFunc_t)(String, String);

That it. Simple.. Please do not forget init library in the beginning by calling:

    sim.begin(&port);

where *port* - is Serial or SoftSerial object. 

PLease have a look how to use this library in the example.
## Example: SMS relay
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
You can combine few commands on one sms sent to module. In example below on sms will switch on relay and send report back with relay's current status:

    ON_1 STATUS


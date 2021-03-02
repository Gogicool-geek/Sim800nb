#include "Sim800nb.hpp"

void Sim800nb::begin(Stream *port)
{
    _port = port;
    _port->flush();
    _buffer.clear();
    _response.reserve(RESPONSE_BUFFER_LENGTH);
    _response = "";
    _callback_sms = NULL;

    _task_heartbeat = TaskManager::add_task("h-beat",
                                            TASK_PRIORITY_MIN,
                                            PERIOD_TASK_HEARTBEAT,
                                            task_heardbeat,
                                            (void *)this,
                                            true,
                                            true);

    _task_delay = TaskManager::add_task("delay",
                                        TASK_PRIORITY_MIN,
                                        1000, // setup when call command
                                        task_make_delay,
                                        (void *)this,
                                        false,
                                        false);

    _task_maintenance = TaskManager::add_task("service",
                                              TASK_PRIORITY_MIN,
                                              PERIOD_TASK_MAINTENANCE, // setup when call command
                                              task_maintenance,
                                              (void *)this,
                                              ENABLE_MAINTENACE_TASK,
                                              true);
    init_sim_module();
}

void Sim800nb::init_sim_module()
{
    push_command("\r\nAT"); // Автонастройка скорости
    push_command_wait();
    push_command("AT+CMGF=1");     // Включить TextMode для SMS
    push_command("AT+DDET=1,0,0"); // Включить DTMF (тональный набор)
    push_command("AT+GSMBUSY=1");  // Запрет входящих звонков
#ifdef DELETE_ALL_SMS_ON_BOOT
    push_command("AT+CMGDA=\"DEL ALL\""); // удаление всех сообщений
#endif
    push_command_wait();
}

void Sim800nb::task_heardbeat(void *param)
{
    Sim800nb *sim = (Sim800nb *)param;
    if (TaskManager::is_timer_enable(sim->_task_delay) == false)
    {
        // sim is not waiting response
        sim->check_command_queue();
    }
    sim->check_sim_response();
}

void Sim800nb::check_sim_response()
{
    if (_port->available())
    {
        _response = _port->readString();
        //Serial.print(F("** response: **\n"));
        Serial.println(_response);
        //Serial.print(F("***************"));

        if (_response.indexOf(F("+CMTI:")) > -1)
        {
            Serial.print(F("*** got new SMS #"));
            int sms_num = _response.substring(_response.indexOf(F("+CMTI:")) + 12).toInt();
            Serial.println(sms_num);
            push_command(CommandType::READ, String(sms_num).c_str());
        }

        if (_response.indexOf(F("+CMGL: ")) > -1)
        {
            Serial.println(F("*** got sms list request"));
            int sms_id = get_sms_number(_response); // we need parse message and get SMS ID
            if (sms_id > 0)
            {
                push_command(CommandType::READ, String(sms_id).c_str());
            }
            TaskManager::disable_timer(_task_delay);
        }
        else if (_response.indexOf(F("+CMGR: ")) > -1)
        {
            Serial.println(F("*** got sms detaila >> "));
            SMSmessage_t msq;
            msq = parse_SMS(_response);
            Serial.print(F(" status:"));
            Serial.print(msq.isValid);
            Serial.print(F(" phone:"));
            Serial.print(msq.phone);
            Serial.print(F(" message:"));
            Serial.println(msq.text);
            if (_callback_sms != NULL)
                _callback_sms(msq.phone, msq.text);
            push_command(CommandType::CLEAN);
        }
        else if (_response.indexOf(F("+CMGS:")) > -1)
        {
            // got SMS sent confirmation
            // we need to request list of received SMS
            // and extend delay
            if (_response.indexOf(F("OK")) > -1)
            {
                Serial.println(F("*** SMS sent OK"));
            }
            else
            {
                Serial.println(F("*** SMS sent ERROR"));
            }
        }
        else
        {
            //Serial.println(F("unattended "));
            // other messages
            // exit withot delay extention
            return;
        }
        // if we recogize command then extend delay
        TaskManager::reset_timer(_task_delay);
    }
}

void Sim800nb::check_command_queue()
{
    CommandItem command_item;
    if (_buffer.pop(command_item) == false)
        return;

    switch (command_item.type)
    {
    case CommandType::WRITE:
        Serial.print(F("*** send command:"));
        Serial.println(command_item.text);
        sim_println(command_item.text);
        break;

    case CommandType::WAIT:
        //Serial.println(F("Enable delay command queue"));
        delay_long(); // start delay timer. no new comand this time
        break;

    case CommandType::SEND:
        Serial.print(F("*** ask to send SMS to:"));
        Serial.print(_phone);
        Serial.print(F(" message:"));
        Serial.println(command_item.text);
        // there is sequence of command -> put in queue
        push_command("\n\rAT+CMGF=1");                                                   // set text mode
        push_command(("AT+CMGS=\"" + _phone + "\"").c_str());                            // send phone
        push_command_wait();                                                             // wait
        push_command((String(command_item.text) + "\r\n" + (String)((char)26)).c_str()); // send message
        push_command_wait();                                                             // wait responce
        break;

    case CommandType::READ:
        Serial.print(F("*** ask to show incoming SMS #"));
        Serial.println(command_item.text);
        sim_println(F("\n\rAT+CMGF=1"));                                // set sms text mode
        sim_println("\r\nAT+CMGR=" + String(command_item.text) + ",0"); // ask for SMS details with change status to REC_READ
        delay_long();                                                   // wait responce
        break;

    case CommandType::PHONE:
        _phone = command_item.text;
        break;

    case CommandType::CHECK:
        Serial.println(F("*** ask to show all inbox SMS"));
        sim_println(F("AT+CMGL=\"ALL\",1")); // ask for SMS list
        delay_long();                        // wait responce
        break;

    case CommandType::CLEAN:
        Serial.println(F("*** ask to delete all rod and sent SMS"));
        sim_println(F("AT+CMGDA=\"DEL READ\""));
        sim_println(F("AT+CMGDA=\"DEL SENT\""));
        delay_long();
        break;

    case CommandType::DELETE:
        Serial.print(F("ask to delete SMS #"));
        sim_println(PSTR("\r\nAT+CMGD=") + String(command_item.text) + PSTR(",0")); // del certain sms and all
        delay_long();
        break;

    default:
        break;
    }
    free((void *)(command_item.text));
}

void Sim800nb::task_make_delay(void *param)
{
    //Sim800nb *sim = (Sim800nb *)param;
}

void Sim800nb::push_command(CommandType type, const char *text)
{
    // pack string
    // example: https://esp32.com/viewtopic.php?t=3086
    int size = strlen(text) + 1;
    void *reference = malloc(size);
    memcpy(reference, text, size);

    CommandItem item;
    item.type = type;
    item.text = (char *)reference;
    _buffer.push(&item);
}

int Sim800nb::get_sms_number(String response)
{
    int begin_index = response.indexOf("+CMGL: ");
    if (begin_index > -1)
    { // Если есть хоть одно, получаем его индекс
        ///Serial.println(begin_index);
        //Serial.println(_response.substring(begin_index + 7, _response.indexOf(",\"REC")));
        int sms_num = (response.substring(begin_index + 7, response.indexOf(",\"REC"))).toInt(); // номер sms
        int phone_index = response.indexOf("\",\"") + 3;
        String phone = response.substring(phone_index, phone_index + 12);

        Serial.print("phone: ");
        Serial.println(phone);
        Serial.print("\t SMS #");
        Serial.println(String(sms_num));
        return sms_num;
    }

    return false;
}

SMSmessage_t Sim800nb::parse_SMS(String msg)
{ // Парсим SMS
    String msgheader = "";
    String msgbody = "";
    SMSmessage_t message;

    if (msg.indexOf(F("+CMGR:")) > -1)
    {
        msg = msg.substring(msg.indexOf("+CMGR: "));
        msgheader = msg.substring(0, msg.indexOf("\r"));
        int firstIndex = msgheader.indexOf("\",\"") + 3;
        int secondIndex = msgheader.indexOf("\",\"", firstIndex);
        message.phone = msgheader.substring(firstIndex, secondIndex); // Выдергиваем телефон

        msgbody = msg.substring(msgheader.length() + 2);
        message.text = msgbody.substring(0, msgbody.lastIndexOf("OK")); // Выдергиваем текст SMS
        message.text.trim();
        message.isValid = true;
    }
    return message;
}

void Sim800nb::task_maintenance(void *param)
{
    Sim800nb *sim = (Sim800nb *)param;
    sim->push_command(CommandType::CLEAN);
    sim->push_command(CommandType::CHECK);
}
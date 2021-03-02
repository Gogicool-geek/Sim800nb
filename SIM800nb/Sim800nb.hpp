// helpfull links: https://codius.ru/articles/GSM_модуль_SIM800L_часть_2

#ifndef SIM_800_N_B_H
#define SIM_800_N_B_H

#include <Arduino.h>
#include <RingBuf.h>
#include <TaskManager.h>

#define DELETE_ALL_SMS_ON_BOOT // comment if you do not want to delete all sms stored in module
#define ENABLE_MAINTENACE_TASK true
// timing parameters
#define RESPONSE_BUFFER_LENGTH 64      // size of responce buffer
#define PERIOD_TASK_HEARTBEAT 100      // period of calling heartbeat: checking command queue and response from SIM module
#define PERIOD_TASK_MAINTENANCE 600000 // period of calling maintenance task
#define TIMEOUT_LONG 2000              //  timeout after WAIT command by default in msec.
#define TIMEOUT_SHORT 200              // timeout between command

#ifndef COMMAND_BUFFER_SIZE
#define COMMAND_BUFFER_SIZE 10 // size of outgoing command buffer
#endif

// push command by function push_command() to internal command queue for proceed furhter
// command types are follow:
enum CommandType
{
    WAIT,   // stop read command queue for period TIMEOUT_LONG
    WRITE,  // write command directly to SIM module (command is in argument.) after command -short stop read TIMEOUT_SHORT
    SEND,   // send sms (text id in arg. phone should be sent in advace. see PHONE command)
    READ,   // read sms with (sms number is in argument.)
    PHONE,  // set default phone (phone is in arg.)
    CHECK,  // check if stored sms available. If they is found, the first one will be proceed
    CLEAN,  // delete all rod incoming sms and all outgoing sms
    DELETE, // delete particular sms (number is in argument)
};
// callback func singnature
// @param first phone number
// @param second sms text
typedef void (*SmsCallBackFunc_t)(String, String);
//
// Non-Blocking library for handling SIM800L module
//
class Sim800nb
{
public:
    // init setup procedure
    // @param port UART connaected to sim module
    void begin(Stream *port);
    // subscribe for incoming sms
    void subscribe_sms(SmsCallBackFunc_t callback) { _callback_sms = callback; }
    // send command to queue
    void push_command(CommandType type, const char *text = "");
    inline void push_command(const char *text) { push_command(CommandType::WRITE, text); };
    inline void push_command_wait() { push_command(CommandType::WAIT, String(param).c_str()); };

private:
    struct SMSmessage_t
    {
        bool isValid;
        String phone;
        String text;
    };
    struct CommandItem
    {
        CommandType type; // command type
        const char *text; // argument of command
    };
    RingBuf<CommandItem, COMMAND_BUFFER_SIZE> _buffer; // internal ring buffer for storing incoming commands
    SmsCallBackFunc_t _callback_sms;
    Stream *_port;
    Task_t _task_heartbeat;
    Task_t _task_delay;
    Task_t _task_maintenance;
    String _phone;    // current phone number where next sms will be sent
    String _response; // responce buffer;
    // The heartbeat of library:
    // periodic task to check command & responce from SIM module
    static void task_heardbeat(void *param);
    // if task in enable, a new command does not read from queue
    // it needs to obtain responce from SIM module when it is needed
    static void task_make_delay(void *param);
    // fulfill sim module maintenance like check unread messages and delete rod ones
    static void task_maintenance(void *param);
    // check command in queue and execute them
    void check_command_queue();
    // check responce from SIM module and parse it
    void check_sim_response();
    // init module
    void init_sim_module();
    // parse phone number and text from sim response
    // @param responce from sim module
    // @returns struct with phone, text and message status
    SMSmessage_t parse_SMS(String responce);
    // parse the first number of sms in sms box list
    // @param responce from sim module
    int get_sms_number(String response);
    // print to SIM module
    template <class T>
    inline void sim_println(T raw)
    {
        _port->println(raw);
        TaskManager::enable_timer(_task_delay);
    }

    inline void delay_short()
    {
        TaskManager::set_time(_task_delay, TIMEOUT_SHORT);
        TaskManager::enable_timer(_task_delay);
    }
    inline void delay_long()
    {
        TaskManager::set_time(_task_delay, TIMEOUT_LONG);
        TaskManager::enable_timer(_task_delay);
    }
};

#endif
// P2P485
//
// David Watson aka dkw
// October 14 2017
// apengineering@yahoo.com
//
// There is a copyright attached to this, mostly because somebody told me I should,
// but withe the exception of th SSerialC hack (source should have original authors)
// and the Dallas/Maxim crc routine, it's all been pretty much clean-room coded.
//
// This is intended to be free to anyone who wants to use it for non-commercial 
// purposes so enjoy.
//
// This is set up now to send with or without CRC based on if send_crc is set.
// On the receiving side, the byte immediately following ETX will either be null
// or CRC. If null, no CRC check will be made and the packet simply received.
// Why? CRC takes time and in some circumstances the operators may feel it unnecessary.
//
// Next is to ACK or NAK the packet (if ack_flag set) and go into timed retries.
// Sorry, no retries at the SAC level, SCA is a slave. SAC now acknowledges, or not,
// personal receipt of valid data. SAC will NOT respond to a general call. Timeouts
// and retries will be mother's resposiblility
//

#include <SSerialC.h>
#include <Streaming.h>
#include <functions.h>

#define     STX         2
#define     ETX         3
#define     ACK         6
#define     NAK         21
#define     ESC         27

//=======1=========2=========3=========4=========5=========6
// Macros

#define     endl        "\r\n"
#define     crlf        << "\r\n"
#define     space       << " " <<
#define     spacend     << " "
#define     comma       << "," <<
#define     tab         << "\t" <<
#define     tabend      << "\t"
#define     Set(x)      x=1
#define     Clear(x)    x=0

//=======1=========2=========3=========4=========5=========6
// inline functions

static inline void      txEnable() {bitSet(PORTB,1);}   // This hardwires DE to pin D9
static inline void      txDisable() {bitClear(PORTB,1);}

//=======1=========2=========3=========4=========5=========6
// Function prototypes

void buildMessage(char);
void clearInputBuffer();
void clearNtwk();
void clearSerial();
void ntwkEvent();
void processCharacter(byte);
void processMessage(char*);
void sendPacket(char*);
void serialEvent();

typedef struct
{
    byte bit1:1;
    byte bit2:1;
    byte bit3:1;
    byte bit4:1;
    byte bit5:1;
    byte bit6:1;
    byte bit7:1;
    byte bit8:1;
} bit_ref;

bit_ref flags1;
bit_ref flags2;

#define have_STX        flags1.bit1
#define have_ADDR       flags1.bit2
#define have_SRC        flags1.bit3
#define have_ETX        flags1.bit4
#define valid_data      flags1.bit5
#define message_waiting flags1.bit6
#define ntwk_event      flags1.bit7
#define serial_event    flags1.bit8

#define send_crc        flags2.bit1
//#define flags2.bit2
//#define flags2.bit3
//#define flags2.bit4
//#define flags2.bit5
//#define flags2.bit6
//#define flags2.bit7
//#define flags2.bit8

SSerialC ntwk(16, 17);

const byte device_id = 11;
const byte buffer_length = 47;

byte source_addr;
byte dest_addr;
byte max_text = (buffer_length - 7);

char input_buffer[buffer_length];

//=======1=========2=========3=========4=========5=========6
void clearInputBuffer()
{
    for(byte i = 0; i < buffer_length; i++) input_buffer[i] = 0;
}
//=======1=========2=========3=========4=========5=========6
void clearSerial()
{
    while(Serial.available()) Serial.read();
}
//=======1=========2=========3=========4=========5=========6
void clearNtwk()
{
    while(ntwk.available()) ntwk.read();
}
//=======1=========2=========3=========4=========5=========6
void setup()
{
    pinMode(13, OUTPUT);
    pinMode(9, OUTPUT);
    bitClear(PORTB,5);
    Set(send_crc);
    Serial.begin(57600);
    while(!Serial);
    ntwk.begin(38400);
    Serial << "P2P485\n";
    Serial << "Device ID " << device_id crlf;
}
//=======1=========2=========3=========4=========5=========6
void loop()
{
    if (Serial.available()) serialEvent();
    if (ntwk.available()) ntwkEvent();
    if (message_waiting) processMessage(input_buffer);
}
//=======1=========2=========3=========4=========5=========6
void serialEvent()
{
    if (!serial_event) Set(serial_event);
    char ch = Serial.read();
    buildMessage(ch);
}
//=======1=========2=========3=========4=========5=========6
void buildMessage(char ch)
{
    static byte index = 0;
    switch (ch)
    {
        case 13:
        case 10:
            if (index == 0) return;
            else
            {
                clearSerial();
                input_buffer[index] = 0;
                index = 0;
                Set(message_waiting);
            }
            break;
        default:
            input_buffer[index++] = ch;
            if(index == max_text)
            {
                Serial << "2big.\n";
                clearInputBuffer();
                index = 0;
            }
            break;
    }
}
//=======1=========2=========3=========4=========5=========6
void ntwkEvent()
{
    if (!ntwk_event) Set(ntwk_event);
    char ch = ntwk.read();
    if((ch == STX) || have_STX) processCharacter(ch);
    else return;
    if(message_waiting) clearNtwk();
}
//=======1=========2=========3=========4=========5=========6
void processCharacter(byte ch)
{
    //Serial << ch;
    static byte _index = 0;
    char acknak[5];
    switch (ch)
    {
        case STX:
            Set(have_STX);
            _index = 0;
            Clear(have_ADDR);
            Clear(have_SRC);
            Clear(have_ETX);
            Clear(valid_data);
            clearInputBuffer();
            break;
    
        case ETX:
            Set(have_ETX);
            break;
        
        default:
            if (!have_STX) break;
            if (!have_ADDR)
            {
                if ((ch == device_id) || (ch == 0))
                {
                    Set(have_ADDR);
                    dest_addr = ch;
                    break;
                }
                else 
                {
                    Clear(have_STX);
                    break;
                }
            }
            else if (!have_SRC)
            {
                source_addr = ch;
                Set(have_SRC);
                break;
            }
            else 
            {
                if(!have_ETX)
                {
                    if(ch == ACK)
                    {
                        Serial << "K\n";
                        Clear(have_STX);
                        clearInputBuffer();
                        return;
                    }
                    input_buffer[_index++] = ch;
                    break;
                }
                else
                {
                    if(ch == 0) Set(valid_data);
                    else if(ch == crc8(input_buffer)) Set(valid_data);
                    input_buffer[_index] = 0;
                    if(valid_data)
                    {
                        Set(message_waiting);
                        acknak[3] = ACK;
                    }
                    else 
                    {
                        Serial << "Bad CRC\n";
                        acknak[3] = NAK;
                    }
                    Clear(have_STX);
                    if(dest_addr != 0)
                    {
                        acknak[0] = STX;
                        acknak[1] = source_addr;
                        acknak[2] = device_id;
                        ntwkTx(acknak, 4);
                    }
                }
            }
            break;  
    }
}
//=======1=========2=========3=========4=========5=========6
void processMessage(char *_cptr)
{
    if(isdigit(_cptr[0]) || (_cptr[0] == '/')) sendPacket(_cptr);
    else 
    {
        if(ntwk_event) Serial << source_addr  << ":" << dest_addr;
        Serial << "> " << _cptr crlf;
    }
    if(serial_event) Clear(serial_event);
    if(ntwk_event) Clear(ntwk_event);
    Clear(message_waiting);
    clearInputBuffer();
    clearSerial();   
}
//=======1=========2=========3=========4=========5=========6
void sendPacket(char *_cptr)
{
    byte _crc;
    byte _dest;
    if(_cptr[0] == '/')
    {
        _dest = source_addr;
        byte i;
        for(i = 0; i < strlen(_cptr); i++) (_cptr[i] = _cptr[i + 1]);
        _cptr[i] = 0;
    }
    else _dest = getAddress(_cptr);
    Serial << _dest << "> " << _cptr crlf;
    if(send_crc) _crc = crc8(_cptr);
    byte _ndex = shiftRightArray(_cptr, 3);

    _cptr[0] = STX;
    _cptr[1] = _dest;
    _cptr[2] = device_id;

    _cptr[_ndex++] = ETX;
    if(send_crc) _cptr[_ndex++] = _crc;
    _cptr[_ndex ++] = 0;

    ntwkTx(_cptr, _ndex);
    clearInputBuffer(); 
}
//=======1=========2=========3=========4=========5=========6
void ntwkTx(char *_cptr, byte _len)
{
    // Note to self: transmit one byte at a time if you want to interrupt you can.
    // Also, as char arrays are zero/NULL terminated, transmitting a 0 becomes problematic.
    //for(byte i = 0; i < _len; i++) Serial << _HEX(_cptr[i]) crlf;
    txEnable();
    for(byte i = 0; i < _len; i++) ntwk << _cptr[i];
    txDisable();
}
//=======1=========2=========3=========4=========5=========6

#include <Arduino.h>

#ifndef functions
#define functions
uint8_t getLength(char*);
uint8_t shiftRightArray(char*, uint8_t);
uint8_t recoverCRC(char*);
uint8_t getAddress(char*);
uint8_t crc8(char*);
double getDouble(char*, byte*);
long getLong(char*, byte*);

//=======1=========2=========3=========4=========5=========6
// returns the number of char in a NULL terminated char array
// eliminates the need of the string library just for the
// strlen function. Skip the header.
uint8_t getLength(char *_cptr)
{
    uint8_t _ndex = 3;
    while(_cptr[++_ndex]);
    return _ndex;
}
//=======1=========2=========3=========4=========5=========6
// shifts the start of the array from 0 to _pos
// returns the next index of the array.
// useful to position the contents of an array to be packetized
uint8_t shiftRightArray(char *_cptr, uint8_t _pos)
{
    uint8_t _len = strlen(_cptr);
    uint8_t _ndex = (_len + _pos);
    _cptr[_ndex] = 0;
    while(_len--) (_cptr[(_len + _pos)] = _cptr[_len]);
    return _ndex++;
}
//=======1=========2=========3=========4=========5=========6
// Assuming the CRC is tacked on to the end of the message,
// this pulls that byte as the return value and replaces it
// with a NULL terminator
uint8_t recoverCRC(char *_cptr)
{
    uint8_t _len = getLength(_cptr);
    uint8_t _crc = _cptr[(_len - 1)];
    _cptr[(_len - 1)] = 0;
    return _crc;
}
//=======1=========2=========3=========4=========5=========6
// This function strips a multi-byte address (i.e. the byte-per-char
// that is a result of terminal entry), converts it to its single byte
// value and then zero-indexes the array and adds a NULL terminator.
uint8_t getAddress(char *_cptr)
{
    uint8_t ndex = 0;
    
    uint8_t dest = getLong(_cptr, &ndex);
    uint8_t len = strlen(_cptr);
    uint8_t msg_length = (len - ndex);
    
    for(uint8_t j = 0; j < msg_length; j++) (_cptr[j] = _cptr[j + ndex]);
    _cptr[msg_length] = 0;
    return dest;
}
//=======1=========2=========3=========4=========5=========6
// With Dallas/Maxim inverse polynomial, returns 8-bit CRC
uint8_t crc8(char *_cptr)
{
    byte _len = strlen(_cptr);
    byte _crc = 0;
    while (_len--) 
    {
        byte _byte = *_cptr++;
        for (byte i = 8; i; i--)
        {
            byte _mix = (_crc ^ _byte) & 0x01;
            _crc >>= 1;
            if (_mix) _crc ^= 0x8C;
            _byte >>= 1;
        }
    }
    return _crc;
}
//=======1=========2=========3=========4=========5=========6
double getDouble(char * cptr, byte * bptr)
{
    byte ch;
    byte divisor        = 1;
    long number         = 0;
    int decimal_part    = 0;

    number = getLong(cptr, bptr);

    if (cptr[*bptr] == 46) (*bptr)++;
    else return number;
    while (1)
    {
        ch = cptr[*bptr];
        if (ch > 47 && ch < 58)
        {
            if (divisor < 100) // restrict doubles to 2 decimal points
            {
                decimal_part = decimal_part * 10 + (ch - 48);
                divisor *= 10;
            }
            (*bptr)++;
        }
        else break;
    }
    if (number > 100) return number;
    if (number == 0 && decimal_part == 0) return 1;
    return ((double)((number * divisor) + decimal_part) / divisor);
}
//=======1=========2=========3=========4=========5=========6
long getLong(char * cptr, byte * bptr)
{
    char ch;
    long number = 0;
    while (1)
    {
        ch = cptr[*bptr];
        if (ch > 47 && ch < 58)
        {
            number = (number * 10) + (ch - 48);
            (*bptr)++;
        }
        else return (number);
    }
}
//=======1=========2=========3=========4=========5=========6
#endif
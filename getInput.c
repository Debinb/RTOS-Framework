//Functions to extract and parse user input from Terminal

#include <stdint.h>
#include <stdbool.h>

#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

//STRUCT:
#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;


void getsUart0(USER_DATA* data)
{
    uint8_t count = 0;
    while(true)
    {
        char c = getcUart0();                     //getting serial data
        if((c == 8 || c == 127) && (count > 0))   //Checking if the character is a backspace (ASCII code 8 or 127) and count > 0
        {
            count--;                              //decrement the last character
        }

        else if(c == 13)                          //Adding a null terminator at the end of string after carriage return (ASCII code 13
        {
            data -> buffer[count] = '\0';
            return;
        }

        else if (c >= 32)                         //Incrementing count when space (ASCII code 32) is pressed
        {
            data->buffer[count] = c;
            count++;

            if (count == MAX_CHARS)               //MAX chars [80] typed results in returning the whole string in the next line.
            {
                putcUart0('\n');
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}

//Parsing the strings input by the user.
void parseFields(USER_DATA* data)
{
    data->fieldCount = 0;
    uint8_t i = 0;
    int prevCh = 0;
    uint8_t indexcount= 0;

    while(data->buffer[i]) // != '\0' && data->fieldCount < MAX_FIELDS)   When data buffer is not empty, the while loop is true
    {
        if (((data->buffer[i] >= 'a') && (data->buffer[i] <= 'z')) || ((data->buffer[i] >= 'A') && (data->buffer[i] <= 'Z')) || (data->buffer[i] == '_')) //checking if the char given is alphanumeric
        {
            if (prevCh == 0) //Transition from delimiter to alpha char
            {
                data->fieldType[indexcount] = 'a';      //Assigns 'a' if the string is alphanumeric
                data->fieldPosition[indexcount] = i;    //The char position of the string is assigned to 'i'.
                data->fieldCount++;
                prevCh = 1;
                indexcount++;
            }
        }

        else if (((data->buffer[i] >= '0') && (data->buffer[i] <= '9')) || (data->buffer[i] == '-') || (data->buffer[i] == '.')) //printing 'n' if numeric
        {
           if(prevCh == 0) //Transition from delimiter to numeric char
           {
               data->fieldType[indexcount] = 'n';
               data->fieldPosition[indexcount] = i;
               data->fieldCount++;
               prevCh = 1;
               indexcount++;
            }
        }
        else                              //If its not a alphanumeric or numeric
        {
            data->buffer[i] = '\0';       //the end is given a NULL
            prevCh = 0;
        }
        i++;
    }
}

//Gets the string typed
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if(fieldNumber <= MAX_FIELDS)
    {
        return &(data->buffer[data->fieldPosition[fieldNumber]]);
    }
    else
    {
        return 0;
    }
}

int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    int32_t i = 0;
    int32_t fieldint = 0;
    char *strtoi;

    if((fieldNumber <= data -> fieldCount) && (data->fieldType[fieldNumber] == 'n'))
    {
        strtoi = &(data->buffer[data->fieldPosition[fieldNumber]]);
        while(strtoi[i])
        {
            fieldint = fieldint * 10 + (strtoi[i] - '0');
            i++;
        }
        return fieldint;
    }
    return 0;
}

//Function to compare the strings (Custom strcmp)
int cmpStr(const char* string1, const char* string2)
{
    while(*string1 && *string2 && (*string1 == *string2))
    {
        ++string1;
        ++string2;
    }
    return (*string1 - *string2);
}


//Function to compare user input
bool isCommand (USER_DATA * data, const char strCommand[], uint8_t minArguments)
{
    uint8_t argCount = (data->fieldCount)-1;  // number of arguments (excluding the command field)

    int i = 0;
    while (strCommand[i] != '\0')             // compare strings by each character (case-insensitive)
    {
        char CharInBuffer = data->buffer[i];    // store the character at index from the input buffer
        char CharInCommand = strCommand[i];     // store the character at index from the command string

        if (CharInBuffer >= 'A' && CharInBuffer <= 'Z')     // Converting to lowercase if chars are uppercase
        {
            CharInBuffer += 32;
        }
        if (CharInCommand >= 'A' && CharInCommand <= 'Z')
        {
            CharInCommand += 32;
        }

        if (CharInBuffer != CharInCommand)                 // compares the converted strings.
        {
            return 0;
        }
        i++;
    }

    if (data->buffer[i] == ' ' || data->buffer[i] == '\0')  // Check if the string is followed by a space or null-terminator in the buffer
    {
        if (argCount >= minArguments)
        {
            return 1;  // Command found and has enough arguments
        }
        else
        {
            return 0;  // Command found but not enough arguments
        }
    }
    return 0;
}

//Function to get count of fields parsed.
int getFieldCount(USER_DATA* data)
{
    return data->fieldCount;        //gives number of fields parsed.
}

//Function to reverse string.
void reverseStr(char* str, uint32_t len)
{
    uint32_t start = 0;
    uint32_t end = len - 1;
    while(start < end)              //When start equals or exceeds end, string has been reversed.
    {
        char temp = str[start];     //Swap characters at start and end
        str[start] = str[end];
        str[end] = temp;
        start++;                    //iterate until center of the string is reached.
        end--;
    }
}

//Function to convert integer to string
void IntToStr(uint32_t num, char* str)
{
    uint32_t i = 0;

    if(num == 0)            //For handling an input of zero
    {
        str[i++] = '0';    //Stores a zero as a string.
        str[i] = '\0';     //Null terminates the string
        return;
    }

    while(num != 0)
    {
        uint32_t remainder = num % 10;      //gets the last digit of the number
        str[i++] = remainder + '0';         //For converting the integer to ascii value.
        num = num/10;                       //removes the last digit from the number.
    }

    str[i] = '\0';                          //mark the end of string using null terminator
    reverseStr(str,i);                      //reverse the string since the processed values are stored in a reverse order.
}

//Function to check if input is integer
bool isInteger(char* str)
{
    uint8_t i = 0;
    for (i = 0; str[i] != '\0'; i++)
    {
        if (str[i] < '0' || str[i] > '9')       //is not an integer
        {
            return 0;
        }
    }
    return 1;                                   //is an integer
}


char* IntToHex(uint32_t num, char* str)
{
    int i;
    char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    str[8] = '\0';                              // Null-terminate at the end for 8 hex digits

    for (i = 28; i >= 0; i -= 4)
    {
        uint8_t bit;
        bit = (num >> i) & 0xF;             // Extract 4 bits
        str[7 - (i / 4)] = hex_chars[bit];  // Place hex character in the correct position
    }
    return str;
}

//Function to copy the string
void StringCopy( char* source,  char* destination)
{
    while(*source)
    {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
}


#pragma once

#include <stdint.h>



//typedef struct{
//    uint8_t   reportID;
//    uint8_t   buttonMask;
//    int8_t    dx;
//    int8_t    dy;
////    int8_t    dWheel;
//}sMouseReport_t;

typedef struct {
    uint8_t buttonMask;    // Bits 0-2: buttons, rest padding
    int8_t dx;           // Relative X (-127 to +127)
    int8_t dy;           // Relative Y (-127 to +127)
}sMouseReport_t;

typedef struct{
    uint8_t   reportID;
    uint8_t   modifier;
    uint8_t   reserved;
    uint8_t   keycode[6]; // change it to 5 if needed
}sKeyboardReport_t;


<<<<<<< HEAD
//uint8_t report_map[] = {
//
//0b00000101, // 0000 page, 01 Global, 1 byte
//0b00000001, // Generic Desktop
//
//0b00001001, // 0000 page, 10 Local, 1 byte
//0b00000010, // Generic Desktop > Mouse
//
//0b10100001 /* 1010-Collection, 00-main, 01-size */,
//0b00000001, // Application <- this is the one that host looks
//
//0b10000101, // 1000 Report ID,  01 Global, 1 Byte
//0b00000001, // Report id value 1
//
//    0b00000101, // 0000-page, 01-global, 1-byte
//    0b00000001, // Generic Desktop
//    0b00001001, // 0000-page, 10-local, 1-byte
//    0b00000001, // Pointer
//
//    0b10100001, // 1010-Collection, 00-main, 1-byte
//    0b00000000, // Physical
//
//        0b00000101, // 0000-page, 01-global, 1-byte
//        0b00001001, // button page
//
//        0b00010101, // 0001-logical minimum, 01-Global, 01-byte
//        0b00000000, // not pressed
//
//        0b00100101, // 0010-logical maximum, 01-global, 01-byte
//        0b00000001, // pressed
//
//        0b01110101, // 0111-report size, 01-global, 01-byte
//        0b00000001, // report size = 1 bit
//
//        0b10010101, // 1001-Report count, 01-global, 1-byte
//        0b00000011, // 3 values of 1 bit (for 3 buttons)
//
//        0b10000001, // 1000-input, 00-main, 01-byte
//        0b00000010, // bit field, no null pos, preffered state, liner, no wrap, abs, var, data
//
//
//
//        // PADDING
//        0b01110101, // 0111-report size, 01-global, 01-byte
//        0b00000001, // 1 bit
//
//        0b10010101, // 1001-report count, 01-global, 01-byte
//        0b00000101, // 5 reports of 1 bit
//
//        0b10000001, // 1000-input, 00-main, 01-byte
//        0b00000001, // bit field, no null pos, preffered state, liner, no wrap, abs, array,const
//
//
//        // X Y Wheel
//        0b00000101, // 0000 page, 01 Global, 1 byte
//        0b00000001, // Generic Desktop
//
//        0b00001001, // 0000 page, 10 Local, 1 byte
//        0b00110000, // Generic Desktop > X (30h)
//
//        0b00001001, // 0000 page, 10 Local, 1 byte
//        0b00110001, // Generic Desktop > Y (38h)
//
//        0b00001001, // 0000 page, 10 Local, 1 byte
//        0b00111000, // Generic Desktop > Wheel (38h)
//
//        0b00010101, // 0001-logical minimum, 01-Global, 01-byte
//        0b10000001, // -128 / 81h min
//
//        0b00100101, // 0010-logical maximum, 01-global, 01-byte
//        0b01111111, //  127 / 7Fh max
//
//        0b01110101, // 0111-report size, 01-global, 01-byte
//        0b00001000, // 8 bits
//
//        0b10010101, // 1001-report count, 01-global, 01-byte
//        0b00000011, // 3 reports of 8 bits
//
//        0b10000001, // 1000-input, 00-main, 01-byte
//        0b00000110, // bit field, no null pos, preffered state, liner, no wrap, rel, var, data
//
//    0b11000000, // end of physical pointer collection
//
//0b11000000, // end of application collection
//
//
//
//// Keyboard
//0b00000101, // 0000 page, 01 Global, 1 byte
//0b00000001, // Generic Desktop
//
//0b00001001, // 0000 page, 10 Local, 1 byte
//0b00000110, // Generic Desktop > Keyboard
//
//0b10100001 /* 1010-Collection, 00-main, 01-size */,
//0b00000001, // Application <- this is the one that host looks
//
//    0b10000101, // 1000 Report ID,  01 Global, 1 Byte
//    0b00000010, // Report id value 2
//
//    0b10100001, // 1010-Collection, 00-main, 1-byte
//    0b00000000, // Physical
//
//        0x05, 0x07, // Usage Page (Key Codes);
//        0x19, 0xE0, // Usage Minimum (224),
//
//    0b11000000, // end of application collection
//
//0b11000000, // end of application collection
//
//};
//
//uint8_t report_map[] = {
//    0x05, 0x01,
//    0x09, 0x02,
//    0xA1, 0x01,
//    0x85, 0x01,
//        0x05,0x01,
//        0x09, 0x01,
//
//        0xA1, 0x00,
//            0x05, 0x09,
//            0x15, 0x00,
//            0x25, 0x01,
//            0x75, 0x01,
//            0x95, 0x03,
//            0x81, 0x02,
//
//            0x75, 0x01,
//            0x95, 0x05,
//            0x81, 0x01,
//
//            0x05, 0x01,
//            0x09, 0x30,
//            0x09, 0x31,
//            0x09, 0x38,
//            0x15, 0x81,
//            0x25, 0x7F,
//            0x75, 0x08,
//            0x95, 0x03,
//            0x81, 0x06,
//        0xC0,
//    0xC0,
//
//
//}
=======
uint8_t report_map[120] = {
>>>>>>> e1e3d30 (hidreport)

//0x
//05010902A1010901A100050919012903150025019503750181029501750581010501093009311581257F750895028106C0C0

0b00001001, // 0000 page, 10 Local, 1 byte
0b00000010, // Generic Desktop > Mouse

0b10100001 /* 1010-Collection, 00-main, 01-size */,   
0b00000001, // Application <- this is the one that host looks

0b10000101, // 1000 Report ID,  01 Global, 1 Byte
0b00000001, // Report id value 1

    0b00000101, // 0000-page, 01-global, 1-byte
    0b00000001, // Generic Desktop
    0b00001001, // 0000-page, 10-local, 1-byte
    0b00000001, // Pointer

    0b10100001, // 1010-Collection, 00-main, 1-byte
    0b00000000, // Physical

        0b00000101, // 0000-page, 01-global, 1-byte
        0b00001001, // button page 

        0b00010101, // 0001-logical minimum, 01-Global, 01-byte
        0b00000000, // not pressed

        0b00100101, // 0010-logical maximum, 01-global, 01-byte
        0b00000001, // pressed

        0b01110101, // 0111-report size, 01-global, 01-byte
        0b00000001, // report size = 1 bit

        0b10010101, // 1001-Report count, 01-global, 1-byte
        0b00000011, // 3 values of 1 bit (for 3 buttons)

        0b10000001, // 1000-input, 00-main, 01-byte
        0b00000010, // bit field, no null pos, preffered state, liner, no wrap, abs, var, data



        // PADDING
        0b01110101, // 0111-report size, 01-global, 01-byte
        0b00000001, // 1 bit 

        0b10010101, // 1001-report count, 01-global, 01-byte
        0b00000101, // 5 reports of 1 bit
        
        0b10000001, // 1000-input, 00-main, 01-byte 
        0b00000001, // bit field, no null pos, preffered state, liner, no wrap, abs, array,const


        // X Y Wheel
        0b00000101, // 0000 page, 01 Global, 1 byte
        0b00000001, // Generic Desktop

        0b00001001, // 0000 page, 10 Local, 1 byte
        0b00110000, // Generic Desktop > X (30h)

        0b00001001, // 0000 page, 10 Local, 1 byte
        0b00110001, // Generic Desktop > Y (38h)

        0b00001001, // 0000 page, 10 Local, 1 byte
        0b00111000, // Generic Desktop > Wheel (38h)

        0b00010101, // 0001-logical minimum, 01-Global, 01-byte
        0b10000001, // -128 / 81h min

        0b00100101, // 0010-logical maximum, 01-global, 01-byte
        0b01111111, //  127 / 7Fh max

        0b01110101, // 0111-report size, 01-global, 01-byte
        0b00001000, // 8 bits 

        0b10010101, // 1001-report count, 01-global, 01-byte
        0b00000011, // 3 reports of 8 bits

        0b10000001, // 1000-input, 00-main, 01-byte 
        0b00000110, // bit field, no null pos, preffered state, liner, no wrap, rel, var, data

    0b11000000, // end of physical pointer collection

0b11000000, // end of application collection



// Keyboard
0b00000101, // 0000 page, 01 Global, 1 byte
0b00000001, // Generic Desktop

0b00001001, // 0000 page, 10 Local, 1 byte
0b00000110, // Generic Desktop > Keyboard

0b10100001 /* 1010-Collection, 00-main, 01-size */,     
0b00000001, // Application <- this is the one that host looks

    0b10000101, // 1000 Report ID,  01 Global, 1 Byte
    0b00000010, // Report id value 2

    0b10100001, // 1010-Collection, 00-main, 1-byte
    0b00000000, // Physical

        // ---------------- SWITCH / BUTTON DEFINITIONS ----------------

        // 0x05, 0x07
        0b00000101, // Usage Page (Global Item): tag=Usage Page, type=Global, size=1 byte
        0b00000111, // 0x07 = Keyboard/Keypad Page (each usage = key code)

        // 0x19,0xE0
        0b00010001, // Usage Minimum (Local Item)
        0b1110000, // 0xE0

        // 0x29, 0xE7
        0b00100001, // Usage Maximum (Local Item)
        0b11100111, // 0xE7

        // 0x15, 0x00
        0b01010101, // Logical Minimum (Global Item): tag=Logical Min, type=Global
        0b00000000, // 0x00 = OFF state (logic low)

        // 0x25, 0x01
        0b01100101, // Logical Maximum (Global Item): tag=Logical Max, type=Global
        0b00000001, // 0x01 = ON state (logic high)

        // 0x75, 0x01
        0b01110101, // Report Size (Global Item)
        0b00000001, // 1 bit per switch

        // 0x95, 0x08
        0b10010101, // Report Count (Global Item)
        0b00001000, 

        // 0x81, 0x02
        0b10000001, // Input (Main Item)
        0b00000010, // (Data, Variable, Absolute) → actual switch data to host

        // ---------------- PADDING (for byte alignment) ----------------

        0x95, //Report Count
        0x01, 

        0x75, //Report Size
        0x08, 

        0x81, //Input(Constant)
        0x01, 

        // ----------------- LED Report ----------------------------------
        0x95,  //Report Count(5)
        0x05, 

        0x75,  // Report Size(1)
        0x01, 

        0x05, // Usage Page for LEDs
        0x08,

        0x19, // Usage Minimum(1)
        0x01, 

        0x29, // Usage Maximum(5)
        0x05, 

        0x91, // Output(data, variable, absolute)
        0x02, 

        // -------------- LED Report padding -------------------------
        0x95, //Report Count(1)
        0x01, 

        0x75, //Report Size (3)
        0x03, 

        0x91, // Output (Constant)
        0x01, 

        // ------------- Key Arrays ---------------------------
        0x95, //Report Count (6)
        0x06, 

        0x75, // Report Size (8)
        0x08, 

        0x15, // Logical Minimum (0)
        0x00, 

        0x25, // Logical Maximum(101)
        0x65, 

        0x05, // Usage Page (Key Codes),
        0x07, 

        0x19, // Usage Minimum (0)
        0x00, 

        0x29, // Usage Maximum (101),
        0x65, 

        0x81, // Input (Data, Array)
        0x00, 

    0b11000000, // end of physical collection

0b11000000, // end of application collection

};


    

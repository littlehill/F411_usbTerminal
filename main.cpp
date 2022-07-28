/*
 *******************************************************************************
 * Project:		F411_usbTerminal
 * Component:
 * Element:
 * File: main.cpp
 *
 * __Description:__
 * usbserial terminal for small uC like STM32F411/F103 (black/blue-pill board etc.)
 * main reason for creating this:
 * - to have something that can be easily modified for custom fixtures in automated testing
 * - digital IO or analog input readout+controll over serial allows for easy prototyping from shell/terminal on host
 *
 * __Credit:__
 * terminal implemetation: https://os.mbed.com/users/murilopontes/code/tinyshell/
 * printf 'hack' https://forums.mbed.com/t/redirecting-printf-to-usb-serial-in-mbed-os-6/14279
 * usbserial doc: https://os.mbed.com/handbook/USBSerial
 *
 * __History:__
 * Version      Date        Author      Description
 * --------------------------------------------------------------------------- *
 * 1.0			28/Jul/2022	Mirek			Initial
 *******************************************************************************/

#include "mbed.h"
#include "USBSerial.h"
#include "tinysh.h"
#include "commands.h"
#include "ringbuffer.h"
#include <cstdint>

#define MY_VERSION_MAJOR 1
#define MY_VERSION_MINOR 0

#define MAIN_LOOP_DELAY_MS 10
#define HBLED_TIME_MS 800
#define USB_CONNECTED_WAIT_MS 500

#define USB_ECHO_ENABLED 1
#define USB_RX_RINGBUFFER_SIZE  128

#define HW_SERIAL_BAUDRATE  115200
#define HW_ECHO_ENABLED 1
#define HW_RX_RINGBUFFER_SIZE  128


// Standardized LED and button names
#define LED1_PIN     PC_13   // blackpill on-board led
#define BUTTON1_PIN  PA_0  // blackpill on-board button
#define HW_SERIAL_TX_PIN PA_9
#define HW_SERIAL_RX_PIN PA_10

/*--- Global definitions ----*/
USBSerial *usbline; //Virtual serial port over USB
RingBuffer *usbRXRingBuffer; // place to store data from USB iterrupt

// extra HW debug console
UnbufferedSerial hwserial(HW_SERIAL_TX_PIN, HW_SERIAL_RX_PIN, HW_SERIAL_BAUDRATE);
RingBuffer *hwRXRingBuffer; // place to store data from USB iterrupt

DigitalOut led1(LED1_PIN);
DigitalIn button(BUTTON1_PIN);
/*---------------------------*/


void rxhandler_hwserial() {
    uint8_t fi, rc;
    rc = hwserial.read(&fi, 1);
    if (rc == 1) {
        hwserial.write(&fi, 1);
    }
}

 class USBStream : public Stream {
    int _putc(int ovc) {
        return usbline->_putc(ovc);
    }
    int _getc() {
        return 0x00; //usbline->_getc();
    }
};

//Virtual serial port RX IRQ handler
/*Echo is done when processing the character, not when it is received*/
void serialCb() {
  uint8_t tc = (uint8_t)usbline->_getc();

  if (!usbRXRingBuffer->write(&tc, 1)) { //write returns 0 if buffer full
      //could not write, reset buffer
      usbRXRingBuffer->reset();
      USBStream tmpErrStream;
      tmpErrStream.printf("\nERROR: USB RX buffer FULL\n");
  }
}

//mandatory tiny shell output function
void tinysh_char_out(unsigned char tc)
{
  usbline->_putc((int)tc);
}


//custom function
void foo_fnt(int argc, char **argv)
{
    USBStream localStream;
    localStream.printf("foo command called\r\n");
    for(int i=0; i<argc; i++) {
        localStream.printf("argv[%d]=\"%s\"\r\n",i,argv[i]);
    }
}
//custom command
tinysh_cmd_t myfoocmd= {0,"foo","foo command","[args]",foo_fnt,0,0,0};
tinysh_cmd_t ts_aboutcmd= {0,"help","about command","[args]",foo_fnt,0,0,0};

int main()
{
   // hwserial.attach(&rxhandler_hwserial, SerialBase::RxIrq);
    unsigned int loopCount = 0;
    unsigned int toggleTime = 0;
    USBStream print2usb; //instance of USBStream, juts for easy printf outputs


    usbRXRingBuffer = new RingBuffer(USB_RX_RINGBUFFER_SIZE);
    usbRXRingBuffer->reset();

    usbline = new USBSerial(false, 0x1f00, 0x2012, 0x0001);
    usbline->init();
    usbline->connect();
    usbline->attach(&serialCb);

    while(!usbline->connected()) {  // wait until the terminal is open
        HAL_Delay(USB_CONNECTED_WAIT_MS);
    }

    print2usb.printf("INIT usb-serial I/O control - V%d.%d\r\n", MY_VERSION_MAJOR, MY_VERSION_MINOR);
    print2usb.printf("Author: [Mirek LittleHill]\r\n");
    print2usb.printf("This is the bare metal Mbed OS %d.%d.%d.\r\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    print2usb.printf("tiny shell system build %s %s\r\n",__DATE__,__TIME__); //print build date
    print2usb.printf("#######\r\n\r\n"); //separate user input

    //set prompt
    tinysh_set_prompt("$>");

    //add custom commands here
    tinysh_add_command(&myfoocmd);
    tinysh_add_command(&ts_aboutcmd);


    toggleTime = HAL_GetTick();

    while (true)
    {
        uint32_t timenow = HAL_GetTick();
        if ((timenow - toggleTime) > HBLED_TIME_MS ) {
            led1 = !led1;
            toggleTime = timenow;
        }

    //if available parse a next character
        while (usbRXRingBuffer->canRead(1)) {
            uint8_t tc = 0;
            if (usbRXRingBuffer->tryRead(&tc, 1)) {
                tinysh_char_in((unsigned char)tc);
            }
            else {
                // TODO: failed to read a character when expected - handle error and reset
            }
        }
        HAL_Delay(MAIN_LOOP_DELAY_MS);
        loopCount++;
    }
}

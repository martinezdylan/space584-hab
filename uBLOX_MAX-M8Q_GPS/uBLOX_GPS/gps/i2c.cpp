#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include "i2c.h"


// *****************************************************************************
// open bus, sets structure and initialises GPIO
// The scl and sda line are set to be always 0 (low) output, when a high is
// required they are set to be an input.
// *****************************************************************************
I2C::I2C(uint8_t adr,      // 7 bit address
	uint8_t data,      // GPIO pin for data 
	uint8_t clock,     // GPIO pin for clock
	uint32_t delay,    // clock delay us
	uint32_t timeout)  // clock stretch & timeout
{
    address = adr;
    sda = 8; //data;
    scl = 9; //clock;
    clock_delay = delay;
    _timeout = timeout;
    Failed = 0;
    if (wiringPiSetup() == -1)
    {
        printf("failed to setup wiringPi.");
	exit (1);
    }
    printf("raspberry pi ver.%d\n",piBoardRev());
    printf("SDA/SCL = %d/%d\n", wpiPinToGpio(sda), wpiPinToGpio(scl));
}

bool I2C::open()
{
    // also they should be set low, input - output determines level
    pinMode(sda, INPUT);
    pinMode(scl, INPUT);

    digitalWrite(sda, LOW);
    digitalWrite(scl, LOW);

    pullUpDnControl(sda, PUD_UP);
    pullUpDnControl(scl, PUD_UP);
    printf("i2c open\n");
    return false;
}

// *****************************************************************************
// *****************************************************************************
void I2C::reset()
{
    pinMode(sda, INPUT);
    digitalWrite(scl, LOW);

    for (int i=0; i<16; i++)
    {
	pinMode(scl, OUTPUT);
	usleep(clock_delay);
	pinMode(scl, INPUT);
	usleep(clock_delay);
    }
}

// *****************************************************************************
// Returns 1 if bus is free, i.e. both sda and scl high
// *****************************************************************************
bool I2C::isBusFree() {
    return digitalRead(sda) && digitalRead(scl);
}

// *****************************************************************************
// writes buffer
// *****************************************************************************
void I2C::puts(uint8_t *s, uint32_t len) {
    Start();
    SendByte(address * 2);
    while(len) {
        uint8_t nack = SendByte(*(s++));
        len--;
    }
    Stop(); 
}

// *****************************************************************************
// read one byte
// *****************************************************************************
uint8_t I2C::getc() {
    uint8_t rv;
    Start();
    SendByte(address * 2 + 1); // read address
    rv = ReadByte(true);
    Stop(); // stop
    return rv;
}

// *****************************************************************************
// *****************************************************************************
void I2C::delayMilliseconds (unsigned int millis) {
  struct timespec sleeper, dummy ;
  sleeper.tv_sec  = (time_t)(millis / 1000) ;
  sleeper.tv_nsec = (long)(millis % 1000) * 1000000 ;
  nanosleep (&sleeper, &dummy) ;
}

// *****************************************************************************
// bit delay, determins bus speed. nanosleep does not give the required delay
// its too much, by about a factor of 100
// This simple delay using the current Aug 2012 board gives aproximately:
// 500 = 50kHz. Obviously a better method of delay is needed.
// *****************************************************************************
void I2C::BitDelay(uint32_t del) {
    usleep(del);
}

// *****************************************************************************
// clock with stretch - bit level
// puts clock line high and checks that it does go high. When bit level
// stretching is used the clock needs checking at each transition
// *****************************************************************************
void I2C::ClockHigh() {
    uint32_t to = _timeout;
    pinMode(scl, INPUT);
    // check that it is high
    while (!digitalRead(scl))
    {
	usleep(1000);
        if(!to--)
	{
            fprintf(stderr, "i2c_info: Clock line held by slave\n");
	    Failed = 1;
            return;
        }
    }
}

// *****************************************************************************
void I2C::ClockLow() {
	pinMode(scl, OUTPUT);
}

// *****************************************************************************
void I2C::DataHigh() {
	pinMode(sda, INPUT);
}

// *****************************************************************************
void I2C::DataLow() {
	pinMode(sda, OUTPUT);
}


// *****************************************************************************
// Start condition
// This is when sda is pulled low when clock is high. This also puls the clock
// low ready to send or receive data so both sda and scl end up low after this.
// *****************************************************************************
void I2C::Start() {
    uint32_t to = _timeout;
    // bus must be free for start condition
    while(to-- && !isBusFree())
    {
	usleep(1000);
    }
    if (!isBusFree())
    {
        fprintf(stderr, "i2c_info: Cannot set start condition\n");
	Failed = 1;
        return;
    }
    // start condition is when data linegoes low when clock is high
    DataLow();
    BitDelay(clock_delay/2);
    ClockLow();
    BitDelay(clock_delay);
}


// *****************************************************************************
// stop condition
// when the clock is high, sda goes from low to high
// *****************************************************************************
void I2C::Stop() {
    DataLow();
    BitDelay(clock_delay);
    ClockHigh(); // clock will be low from read/write, put high
    BitDelay(clock_delay);
    DataHigh();
}

// *****************************************************************************
// sends a byte to the bus, this is an 8 bit unit so could be address or data
// msb first
// returns 1 for NACK and 0 for ACK (0 is good)
// *****************************************************************************
bool I2C::SendByte(uint8_t value) 
{
    uint32_t rv;
    uint8_t j, mask=0x80;
    // clock is already low from start condition
    for(j=0;j<8;j++)
    {
        BitDelay(clock_delay);
        if (value & mask)
	{
	    DataHigh();
	}
	else
	{
	    DataLow();
	}
        // clock out data
        ClockHigh();  // clock it out
        BitDelay(clock_delay);
        ClockLow();      // back to low so data can change
        mask>>= 1;      // next bit along
    }
    // release bus for slave ack or nack
    DataHigh();
    BitDelay(clock_delay);
    ClockHigh();     // and clock high tels slave to NACK/ACK
    BitDelay(clock_delay); // delay for slave to act
    rv = digitalRead(sda);     // get ACK, NACK from slave

    ClockLow();
    BitDelay(clock_delay);
    return rv;
}

// *****************************************************************************
// receive 1 char from bus
// Input
// send: 1=nack, (last byte) 0 = ack (get another)
// *****************************************************************************
uint8_t I2C::ReadByte(bool ack) {
    uint8_t j, data=0;
    for (j=0;j<8;j++)
    {
        data<<= 1;      // shift in
        BitDelay(clock_delay);
        ClockHigh();      // set clock high to get data
        BitDelay(clock_delay); // delay for slave

	if (digitalRead(sda)) 
            data++;   // get data

	ClockLow();
    }

    // clock has been left low at this point
    // send ack or nack
    BitDelay(clock_delay);
 
    if (ack)
    {
	DataHigh();
    }
    else
    {
	DataLow();
    }

    BitDelay(clock_delay);
    ClockHigh();    // clock it in
    BitDelay(clock_delay);
    ClockLow();
    DataHigh();

    return data;
}

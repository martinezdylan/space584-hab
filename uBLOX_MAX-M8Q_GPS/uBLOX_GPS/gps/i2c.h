#include <stdint.h>

class I2C {
    uint8_t address; // 7 bit address
    uint8_t sda; // pin used for sda coresponds to gpio
    uint8_t scl; // clock
    uint32_t clock_delay; // proporional to bus speed
    uint32_t _timeout; //

    void BitDelay(uint32_t del);

    void ClockHigh();
    void ClockLow();
    void DataHigh();
    void DataLow();
    void Start();
    void Stop();
    bool SendByte(uint8_t value);
    uint8_t ReadByte(bool ack);

  public:
    int Failed;
    I2C(uint8_t adr,       // 7 bit address
	uint8_t data,      // GPIO pin for data 
	uint8_t clock,     // GPIO pin for clock
	uint32_t delay,    // clock delay us
	uint32_t timeout); // clock stretch & timeout
    void delayMilliseconds (unsigned int millis);
    bool open();
    void reset();
    bool isBusFree();
    void puts(uint8_t *s, uint32_t len);
    uint8_t getc();
};

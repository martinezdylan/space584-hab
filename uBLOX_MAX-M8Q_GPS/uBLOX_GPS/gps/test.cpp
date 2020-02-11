#include "i2c.h"

int main(void)
{
    I2C *i2c;
    i2c = new I2C( 0x42, 8, 9, 10, 100);
    i2c->open();
    while(1)
    {
         i2c->ClockHigh();
         i2c->delayMilliseconds(100);
         i2c->DataHigh();
         i2c->delayMilliseconds(100);
         i2c->ClockLow();
         i2c->delayMilliseconds(100);
         i2c->DataLow();
         i2c->delayMilliseconds(100);
    }
}	

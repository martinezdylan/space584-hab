#include "i2c.h"

struct TGPS
{
	// long Time;						// Time as read from GPS, as an integer but 12:13:14 is 121314
	long SecondsInDay;					// Time in seconds since midnight
	int Hours, Minutes, Seconds;
	float Longitude, Latitude;
	int32_t Altitude, MaximumAltitude;
	float AscentRate;
	unsigned int Satellites;
	int Speed;
	int Direction;
	float DS18B20Temperature[2];
	float BatteryVoltage;
	float BMP180Temperature;
	float Pressure;
	float BoardCurrent;
	int DS18B20Count;
	float PredictedLongitude, PredictedLatitude;
	int FlightMode;
	int PowerMode;
	int Lock;
};

typedef void (*fix_callback)(TGPS* data);

class Ublox {
    I2C *i2c;
     struct TGPS *GPS;
    int SDA, SCL;
    void SendUBX(unsigned char *MSG, int len);
    void SendUBXwithAck(unsigned char *MSG, int len);
    char Hex(char Character);
    int GPSChecksumOK(unsigned char *Buffer, int Count);
    float FixPosition(float Position);
    void ProcessLine(struct TGPS *GPS, unsigned char *Buffer, int Count);
    void WriteLog(unsigned char *Buffer);
    void WriteJSON();
  public:
    bool EnableGPSLogging;
    fix_callback fix_handler;
    Ublox();
    void run();
    void SetFlightMode();
};



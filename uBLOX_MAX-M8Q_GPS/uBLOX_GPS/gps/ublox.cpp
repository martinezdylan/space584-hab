#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "ublox.h"
#include "trace.h"

void Ublox::WriteLog(unsigned char *Buffer)
{
	FILE *fp;
trace_in("write log");
	if ((fp = fopen("gps.log", "at")) != NULL)
	{
		fputs((char *)Buffer, fp);
		fclose(fp);
	}
trace_out();
}

void Ublox::WriteJSON()
{
        FILE *fp;
trace_in("write json");
        if ((fp = fopen("gps.json", "w")) != NULL)
        {
                fprintf(fp,"}");
                fprintf(fp,"  Longitude : %.4f,\n",GPS->Longitude);
                fprintf(fp,"  Latitude : %.4f,\n", GPS->Latitude);
                fprintf(fp,"  Altitude : %.1f,\n", GPS->Altitude);
                fprintf(fp,"}");
                fclose(fp);
        }
trace_out();
}

void Ublox::SendUBX(unsigned char *MSG, int len)
{
trace_in("send ubx");
        i2c->puts(MSG, len);
trace_out();
}

void Ublox::SendUBXwithAck(unsigned char *MSG, int len)
{
trace_in("send ubx with ack");
	i2c->puts(MSG, len);
        if (MSG[2] == 0x06)
        {
          i2c->delayMilliseconds(100);
          printf("\r\nACK for %02x %02x ? ", MSG[2], MSG[3]);
          char ch;
          while (ch != 0xb5)
            ch = i2c->getc();
          printf("%02X ", ch);
          for (int i=0; i<9; i++)
          {
            char Character = i2c->getc();
            printf("%02X ", Character );
          }
          printf("\r\n");
        }
trace_out();
}

char Ublox::Hex(char Character)
{
trace_in("hex");
	char HexTable[] = "0123456789ABCDEF";
	return HexTable[Character & 0x0f];
trace_out();
}

int Ublox::GPSChecksumOK(unsigned char *Buffer, int Count)
{
  unsigned char XOR, i, c;
trace_in("gpschecksumok");
trace_out();
return 1;
printf("count = %d\n", Count);
  XOR = 0;
  for (i = 1; i < (Count-4); i++)
  {
    c = Buffer[i];
    XOR ^= c;
  }
printf("[A]");
  bool rv =  (Buffer[Count-4] == '*') ;
printf("[B]");
       rv &= (Buffer[Count-3] == Hex(XOR >> 4));
printf("[C]");
       rv &= (Buffer[Count-2] == Hex(XOR & 15));
printf("[D]");

  trace_out();
  return rv;
}

float Ublox::FixPosition(float Position)
{
	float Minutes, Seconds;
trace_in("fixpos");
	Position = Position / 100;
	Minutes = trunc(Position);
	Seconds = fmod(Position, 1);
trace_out();
	return Minutes + Seconds * 5 / 3;
}

void Ublox::SetFlightMode()
{
trace_in("setflightmode");
    printf ("Setting flight mode\n");
    // Send navigation configuration command
    unsigned char setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};
    SendUBXwithAck(setNav, sizeof(setNav));
trace_out();
}

void Ublox::ProcessLine(struct TGPS *GPS, unsigned char *Buffer, int Count)
{
trace_in("proccessline");
    static int SystemTimeHasBeenSet=0;

    float utc_time, latitude, longitude, hdop, altitude;
    int lock, satellites;
    char active, ns, ew, units, timestring[16], speedstring[16], *course, *date, restofline[80], *ptr;
    if (GPSChecksumOK(Buffer, Count))
    {
	satellites = 0;

	if (strncmp((char *)Buffer+3, "GGA", 3) == 0)
	{
	    if (sscanf((char *)Buffer+7, "%f,%f,%c,%f,%c,%d,%d,%f,%f,%c", &utc_time, &latitude, &ns, &longitude, &ew, &lock, &satellites, &hdop, &altitude, &units) >= 1)
	    {
		// $GPGGA,124943.00,5157.01557,N,00232.66381,W,1,09,1.01,149.3,M,48.6,M,,*42
		if (satellites >= 4)
		{
		    unsigned long utc_seconds;
                    utc_seconds = utc_time;
                    GPS->Hours = utc_seconds / 10000;
                    GPS->Minutes = (utc_seconds / 100) % 100;
                    GPS->Seconds = utc_seconds % 100;
                    GPS->SecondsInDay = GPS->Hours * 3600 + GPS->Minutes * 60 + GPS->Seconds;
                    GPS->Latitude = FixPosition(latitude);
	            if (ns == 'S')
                        GPS->Latitude = -GPS->Latitude;
		    GPS->Longitude = FixPosition(longitude);
		    if (ew == 'W')
                        GPS->Longitude = -GPS->Longitude;

		    if (GPS->Altitude <= 0)
                    {
			GPS->AscentRate = 0;
                    }
		    else
		    {
			GPS->AscentRate = GPS->AscentRate * 0.7 + ((int32_t)altitude - GPS->Altitude) * 0.3;
		    }
	            // printf("Altitude=%ld, AscentRate = %.1lf\n", GPS->Altitude, GPS->AscentRate);
                    GPS->Altitude = altitude;
		}
	    GPS->Satellites = satellites;
	}
	if (EnableGPSLogging)
	{
		WriteLog(Buffer);
	}
        WriteJSON();
    }
    else if (strncmp((char *)Buffer+3, "RMC", 3) == 0)
    {
	speedstring[0] = '\0';
	if (sscanf((char *)Buffer+7, "%[^,],%c,%f,%c,%f,%c,%[^,],%s", timestring, &active, &latitude, &ns, &longitude, &ew, speedstring, restofline) >= 7)
	{
	    // $GPRMC,124943.00,A,5157.01557,N,00232.66381,W,0.039,,200314,,,A*6C
            ptr = restofline;
	    course = strsep(&ptr, ",");
            date = strsep(&ptr, ",");
            GPS->Speed = (int)atof(speedstring);
            GPS->Direction = (int)atof(course);
            if ((atof(timestring) > 0) && !SystemTimeHasBeenSet)
            {
		struct tm tm;
		char timedatestring[32];
		time_t t;
		// Now create a tm structure from our date and time
		memset(&tm, 0, sizeof(struct tm));
		sprintf(timedatestring, "%c%c-%c%c-20%c%c %c%c:%c%c:%c%c",
			date[0], date[1], date[2], date[3], date[4], date[5],
         		timestring[0], timestring[1], timestring[2], 
                        timestring[3], timestring[4], timestring[5]);
		strptime(timedatestring, "%d-%m-%Y %H:%M:%S", &tm);

		t = mktime(&tm);
		if (stime(&t) == -1)
		{
		    printf("Failed to set system time\n");
		}
		else
		{
		    SystemTimeHasBeenSet = 1;
		}
	    }
	}

	if (EnableGPSLogging)
	{
             WriteLog(Buffer);
	}
        WriteJSON();
    }
    else if (strncmp((char *)Buffer+3, "GSV", 3) == 0)
    {
            // Disable GSV
            printf("Disabling GSV\r\n");
            unsigned char setGSV[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39 };
            SendUBX(setGSV, sizeof(setGSV));
    }
    else if (strncmp((char *)Buffer+3, "GLL", 3) == 0)
    {
            // Disable GLL
            printf("Disabling GLL\r\n");
            unsigned char setGLL[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B };
            SendUBX(setGLL, sizeof(setGLL));
    }
    else if (strncmp((char *)Buffer+3, "GSA", 3) == 0)
    {
            // Disable GSA
            printf("Disabling GSA\r\n");
            unsigned char setGSA[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32 };
            SendUBX(setGSA, sizeof(setGSA));
    }
    else if (strncmp((char *)Buffer+3, "VTG", 3) == 0)
    {
            // Disable VTG
            printf("Disabling VTG\r\n");
//            unsigned char setVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47};
              unsigned char setVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x06, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x02, 0x2E};
            SendUBX(setVTG, sizeof(setVTG));
    }
    else if (strncmp((char *)Buffer+3, "TXT", 3) == 0)
    {
            // disable TXT
/*
            printf("Disabling TXT\r\n");
            unsigned char setTXT[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x41, 0xEB};
//            unsigned char setTXT[] = {0xB5, 0x62, 0x06, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xAE};
            SendUBX(setTXT, sizeof(setTXT));
*/
    }
    else
    {
            printf("Unknown NMEA sentence: %s\n", Buffer);
    }
  }
  else
  {
     printf("Bad checksum\r\n");
  }
printf("\r\n");
trace_out();
}

void Ublox::run(void)
{
    unsigned char Line[100];
    int id, Length;
trace_in("run");
    Length = 0;
    i2c = new I2C( 0x42, SDA, SCL, 10, 100);
    //printf ("SDA/SCL = %d/%d\n", SDA, SCL);

    while (1)
    {
        int i;
	unsigned char Character;

	if (i2c->open())
	{
		printf("Failed to open I2C\n");
		//exit(1);
	}

	SetFlightMode();

        while (!i2c->Failed)
        {
            Character = i2c->getc();
            if (Character == 0xFF) // no more data
            {
		i2c->delayMilliseconds(100);
	    }
            else if (Character == '$') // NMEA start byte
            {
		Line[0] = Character;
		Length = 1;
                printf("\n$");
            }
            else if (Length > 90) // msg too long
            {
		Length = 0;
            }
            else if ((Length > 0) && (Character != '\r'))
            {
               	Line[Length++] = Character;
               	if ((Character == '\n')) // || (Character == '\r'))
               	{
               		Line[Length] = '\0';
                        printf("-->");
               		ProcessLine(GPS, Line, Length);
			i2c->delayMilliseconds (100);
               		Length = 0;
               	}
		else
		    if ((Character>20) && (Character<0x80))
                      printf("%c",Character);
                    else
                      printf("<%02x>", Character);
            }

	}
	i2c->reset();
    }
trace_out();
}

Ublox::Ublox()
{
trace_in("ublox ctor");
trace_out();
}

//--------------------------------------------------------------------
// APRS Frame Encoder
// based on PiInTheSky
// and argp library examples
//--------------------------------------------------------------------


#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <argp.h>
#include "json.h"

const char *argp_program_version = "piaprs 1.0";
const char *argp_program_bug_address = "<ido.roseman@gmail.com>";

/* Program documentation. */
static char doc[] =
  "piaprs -- create an APRS audio file you can send over radio";

/* A description of the arguments we accept. */
static char args_doc[] = "CallSign Lon Lat";

/* Keys for options without short-options. */
#define OPT_SSID  1            /* –ssid */

/* flags */
static unsigned argp_flags = 0; //ARGP_IN_ORDER;

/* The options we understand. */
static struct argp_option options[] = {
  /* name      key  arg    flag doc */
/*
  {"verbose",  'v', 0,      0,  "Produce verbose output" },
  {"quiet",    'q', 0,      0,  "Don't produce any output" },
  {"silent",   's', 0,      OPTION_ALIAS },
*/
  {"altitude", 'a', "ALT",  0,  "Altitude (in meters)" },
  {"ssid",     OPT_SSID, "NUMBER",     0,  "ssid number. see..." },
  {"file",     'f', "FILE", 0,  "read gps data from file"},
  {"output",   'o', "FILE", 0,  "Output to FILE instead of standard output" },
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  float Longitude, Latitude;
  unsigned int Altitude;
  char *callsign;
  int ssid;
  char *args[2];                /* arg1 & arg2 */
  int silent, verbose;
  char *output_file;
  char *input_file;
} Arguments;

int params_needed = 3;

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'q': case 's':
      arguments->silent = 1;
      break;
    case 'v':
      arguments->verbose = 1;
      break;
    case 'o':
      arguments->output_file = arg;
      break;
    case 'f':
      arguments->input_file = arg;
      params_needed = 1;
      break;
    case 'a':
      arguments->Altitude = arg ? atoi(arg) : 0;
    case OPT_SSID:
      arguments->ssid = arg ? atoi (arg) : 11;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 3)
        /* Too many arguments. */
        argp_usage (state);

      switch (state->arg_num)
      {
         case 0: arguments-> callsign = arg; break;
         case 1: arguments->Longitude = atof(arg); break;
         case 2: arguments->Latitude  = atof(arg); break;
      }
      break;

    case ARGP_KEY_END:
      if (state->arg_num < params_needed)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

void json_handler(char *token, char *value, void *param)
{
  struct arguments *args = param;
  if (!strcmp(token,"Lon"))
     args->Longitude = atof(value); 
  else if (!strcmp(token, "Lat"))
     args->Latitude  = atof(value);
  else if (!strcmp(token, "Alt"))
     args->Altitude = atoi(value);
//  else
//     printf("unknown %s = %s\n",token, value);
}

// Sine wave table for tone generation
uint8_t _sine_table[] = {
#include "sine_table.h"
};

typedef uint16_t	UI;
typedef uint32_t	UL;
typedef uint16_t	US;
typedef uint8_t		UC;
typedef int16_t		SI;
typedef int32_t		SL;
typedef int16_t		SS;
typedef int8_t		SC;
 
#define attr(a) __attribute__((a))
 
#define packed attr(packed)

/* WAV header, 44-byte total */
typedef struct{
 UL riff	packed;
 UL len		packed;
 UL wave	packed;
 UL fmt		packed;
 UL flen	packed;
 US one		packed;
 US chan	packed;
 UL hz		packed;
 UL bpsec	packed;
 US bpsmp	packed;
 US bitpsmp	packed;
 UL dat		packed;
 UL dlen	packed;
}WAVHDR;
 
typedef struct 
{
	float Time;
	float Longitude, Latitude;
	unsigned int Altitude;
	unsigned int Satellites;
	int Speed;
	int Direction;
	float InternalTemperature;
	float BatteryVoltage;
	float ExternalTemperature;
	float Pressure;
	float BoardCurrent;
} TGPS;

// APRS / AFSK variables
  
 
/* "converts" 4-char string to long int */
#define dw(a) (*(UL*)(a))


static uint8_t *_ax25_callsign(uint8_t *s, char *callsign, char ssid)
{
  char i;
  for(i = 0; i < 6; i++)
  {
    if(*callsign) *(s++) = *(callsign++) << 1;
    else *(s++) = ' ' << 1;
  }
  *(s++) = ('0' + ssid) << 1;
  return(s);
}

char *ax25_base91enc(char *s, uint8_t n, uint32_t v)
{
  /* Creates a Base-91 representation of the value in v in the string */
  /* pointed to by s, n-characters long. String length should be n+1. */

  for(s += n, *s = '\0'; n; n--)
  {
    *(--s) = v % 91 + 33;
    v /= 91;
  }

  return(s);
}

uint8_t *ax25_frame(int *length, char *scallsign, char sssid, char *dcallsign, char dssid, char *path1, char ttl1, char *path2, char ttl2, char *data, ...)
{
	static uint8_t frame[200];
  uint8_t *s, j;
  uint16_t CRC;
  va_list va;

  va_start(va, data);

  /* Write in the callsigns and paths */
  s = _ax25_callsign(frame, dcallsign, dssid);
  s = _ax25_callsign(s, scallsign, sssid);
  if(path1) s = _ax25_callsign(s, path1, ttl1);
  if(path2) s = _ax25_callsign(s, path2, ttl2);

  /* Mark the end of the callsigns */
  s[-1] |= 1;

  *(s++) = 0x03; /* Control, 0x03 = APRS-UI frame */
  *(s++) = 0xF0; /* Protocol ID: 0xF0 = no layer 3 data */

  // printf ("Length A is %d\n", s - frame);
  
  // printf ("Adding %d bytes\n", 
  vsnprintf((char *) s, 200 - (s - frame) - 2, data, va);
  va_end(va);

  // printf ("Length B is %d\n", strlen(frame));
  
	// Calculate and append the checksum

	for (CRC=0xffff, s=frame; *s; s++)
    {
		CRC ^= ((unsigned int)*s);
		
        for (j=0; j<8; j++)
        {
			if (CRC & 1)
			{
				CRC = (CRC >> 1) ^ 0x8408;
			}
			else
			{
				CRC >>= 1;
			}
        }
    }
	/* 
	for (CRC=0xffff, s=frame; *s; s++)
    {
  	  CRC = ((CRC) >> 8) ^ ccitt_table[((CRC) ^ *s) & 0xff];
	}
	*/
	
	*(s++) = ~(CRC & 0xFF);
	*(s++) = ~((CRC >> 8) & 0xFF);
  
	// printf ("Checksum = %02Xh %02Xh\n", *(s-2), *(s-1));

	*length = s - frame;
  
	return frame;
}

/* Makes 44-byte header for 8-bit WAV in memory
usage: wavhdr(pointer,sampleRate,dataLength) */
 
void wavhdr(void*m,UL hz,UL dlen){
 WAVHDR*p=m;
 p->riff=dw("RIFF");
 p->len=dlen+44;
 p->wave=dw("WAVE");
 p->fmt=dw("fmt ");
 p->flen=0x10;
 p->one=1;
 p->chan=1;
 p->hz=hz;
 p->bpsec=hz;
 p->bpsmp=1;
 p->bitpsmp=16;	// 8
 p->dat=dw("data");
 p->dlen=dlen;
}
void make_and_write_freq(FILE *f, UL cycles_per_bit, UL baud, UL lfreq, UL hfreq, int8_t High)
{
	// write 1 bit, which will be several values from the sine wave table
	static uint16_t phase  = 0;
	uint16_t step;
	int i;

	if (High)
	{
		step = (512 * hfreq << 7) / (cycles_per_bit * baud);
		// printf("-");
	}
	else
	{
		step = (512 * lfreq << 7) / (cycles_per_bit * baud);
		// printf("_");
	}
	
	for (i=0; i<cycles_per_bit; i++)
	{
		// fwrite(&(_sine_table[(phase >> 7) & 0x1FF]), 1, 1, f);
		int16_t v = _sine_table[(phase >> 7) & 0x1FF] * 0x80 - 0x4000;
		v *= 1.3;
		// int16_t v = _sine_table[(phase >> 7) & 0x1FF] * 0x100 - 0x8000;
		fwrite(&v, 2, 1, f);
		phase += step;
	}
}

void make_and_write_bit(FILE *f, UL cycles_per_bit, UL baud, UL lfreq, UL hfreq, unsigned char Bit, int BitStuffing)
{
	static int8_t bc = 0;
	static int8_t High = 0;
			
	/*
	if (Bit)
	{
		// Stay with same frequency, but only for a max of 5 in a row
		bc++;
	}
	else
	{
		// 0 means swap frequency
		High = !High;
		bc = 0;
	}
	
	make_and_write_freq(f, cycles_per_bit, baud, lfreq, hfreq, High);
	if (BitStuffing)
	{
		if (bc >= 4)
		{	
			High = !High;
			make_and_write_freq(f, cycles_per_bit, baud, lfreq, hfreq, High);
			bc = 0;
		}
	}
	else
	{
		bc = 0;
	}
	*/
	if(BitStuffing)
	{
		if(bc >= 5)
		{
			High = !High;
			make_and_write_freq(f, cycles_per_bit, baud, lfreq, hfreq, High);
			bc = 0;
		}
	}
	else
	{
		bc = 0;
	}
	
	if (Bit)
	{
		// Stay with same frequency, but only for a max of 5 in a row
		bc++;
	}
	else
	{
		// 0 means swap frequency
		High = !High;
		bc = 0;
	}
	
	make_and_write_freq(f, cycles_per_bit, baud, lfreq, hfreq, High);	
}

 
void make_and_write_byte(FILE *f, UL cycles_per_bit, UL baud, UL lfreq, UL hfreq, unsigned char Character, int BitStuffing)
{
	int i;
	
	// printf("%02X ", Character);
		
	for (i=0; i<8; i++)
	{
		make_and_write_bit(f, cycles_per_bit, baud, lfreq, hfreq, Character & 1, BitStuffing);
		Character >>= 1;
	}
}

 
/* makes wav file */
void makeafsk(UL freq, UL baud, UL lfreq, UL hfreq, unsigned char *Message, int message_length)
{
	UL preamble_length, postamble_length, flags_before, flags_after, cycles_per_bit, cycles_per_byte, total_cycles;
	UC* m;
	FILE *f;
	int i;
	
	f = fopen("aprs.wav","wb");
	if (f == NULL)
	{
	    printf("fopen failed, errno = %d\n", errno);
	    return;
	}
	
	cycles_per_bit = freq / baud;
	printf ("cycles_per_bit=%d\n", cycles_per_bit);
	cycles_per_byte = cycles_per_bit * 8;
	printf ("cycles_per_byte=%d\n", cycles_per_byte);

	preamble_length = 128;
	postamble_length = 64;
	flags_before = 32;
	flags_after = 32;

	// Calculate size of file
	total_cycles = (cycles_per_byte * (flags_before + message_length + flags_after)) + ((preamble_length + postamble_length) * cycles_per_bit);

	// Make header
	m = malloc(sizeof(WAVHDR));
	wavhdr(m, freq, total_cycles * 2 + 10);		// * 2 + 10 is new

	// Write wav header
	fwrite(m, 1, 44, f);

	// Write preamble
	/*
	for (i=0; i< preamble_length; i++)
	{
		make_and_write_freq(f, cycles_per_bit, baud, lfreq, hfreq, 0);
	}
	*/
	
	for (i=0; i<flags_before; i++)
	{
		make_and_write_byte(f, cycles_per_bit, baud, lfreq, hfreq, 0x7E, 0);
	}
	
	// Create and write actual data
	for (i=0; i<message_length; i++)
	{
		make_and_write_byte(f, cycles_per_bit, baud, lfreq, hfreq, Message[i], 1);
	}

	for (i=0; i<flags_after; i++)
	{
		make_and_write_byte(f, cycles_per_bit, baud, lfreq, hfreq, 0x7E, 0);
	}

	// Write postamble
	for (i=0; i< postamble_length; i++)
	{
		make_and_write_freq(f, cycles_per_bit, baud, lfreq, hfreq, 0);
	}
	
	fclose(f);
//	free(m);
}

	
void SendAPRS(const struct arguments *ARGS)
{
	static Count=0;
	char stlm[9];
	char slat[5];
	char slng[5];
	char comment[3]={' ', ' ', '\0'};
	double aprs_lat, aprs_lon;
	int32_t aprs_alt;
	static uint16_t seq = 0;
	uint8_t *frame;
	int i, length;

	/*
	lat = 51.95023;
	lon = -2.54445;
	alt = 190;
	*/
	
	Count++;
	seq++;
	
	// Convert the min.dec coordinates to APRS compressed format
	aprs_lat = 900000000 - ARGS->Latitude * 10000000;
	aprs_lat = aprs_lat / 26 - aprs_lat / 2710 + aprs_lat / 15384615;
	aprs_lon = 900000000 + ARGS->Longitude * 10000000 / 2;
	aprs_lon = aprs_lon / 26 - aprs_lon / 2710 + aprs_lon / 15384615;
	aprs_alt = ARGS->Altitude ; // * 32808 / 10000; // this is from feet to meters 


	// Construct the compressed telemetry format
	ax25_base91enc(stlm + 0, 2, seq);
	
    frame = ax25_frame(&length,
    	ARGS->callsign,
	ARGS->ssid,
    "APRS", 0,
    "WIDE1", 1, "WIDE2",1,
    "!/%s%sO   /A=%06ld|%s|%s/%s,%d'C,http://idoroseman.com",
    ax25_base91enc(slat, 4, aprs_lat),
    ax25_base91enc(slng, 4, aprs_lon),
    aprs_alt, stlm, comment, ARGS->callsign, Count);	// , errorstatus,temperature1);

	printf("Message length=%d\n", length);

	makeafsk(48000, 1200, 1200, 2200, frame, length);
}

int main(int argc, char *argv[])
{
	TGPS *gps;
	struct arguments arguments;

	/* Default values. */
        arguments.Longitude = 32.0;
        arguments.Latitude = 34.0;
        arguments.Altitude = 100; //meter 
        arguments.callsign = "4X6UB";
        arguments.ssid = 11;

	arguments.silent = 0;
	arguments.verbose = 0;
	arguments.output_file = "-";
        arguments.input_file = "";
	argp_parse (&argp, argc, argv, argp_flags, 0, &arguments);
        if (strcmp(arguments.input_file, ""))
        {
           printf("reading gps file \"%s\"\n", arguments.input_file);
           json_parse(arguments.input_file, json_handler, &arguments);
	   printf("Lat %.4f\n", arguments.Latitude);
	   printf("Lon %.4f\n", arguments.Longitude);
	   printf("Alt %.4f\n", arguments.Altitude);
           printf("done");
        }
	printf("Generating APRS message ...\n");
        SendAPRS(&arguments);
}

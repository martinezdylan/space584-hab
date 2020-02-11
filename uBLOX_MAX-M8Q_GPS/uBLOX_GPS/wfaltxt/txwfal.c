/*
*  This program was partly copied from pcm.c
*  It generates sound that write characters in a waterfal display
*  from 350 to 2800 Hz
*  Author M.Bos - PA0MBO
*  Date Aug 30th 2012
*  filename wfaltxt.c
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#define  PI  (4.0*atan(1.0))
#define FFT_SIZE 4096
#define PIX_WIDTH 2
#define FREQ_AMPLITUDE 3E3 
#define FREQ_OFFSET 350.0
#define Fs 48000
#define LP_LENGTH 97
#define EXTRA_LINE_SPACE (10+2)


static char *snddevices[4] = {"hw:0,0", "hw:1,0", "hw:2,0", "hw:3:0"} ;
static char *device;


/* static char *device = "plughw:0,0";	 playback device */
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;	/* sample format */
static unsigned int rate = 48000;	/* stream rate was 44100 */
static unsigned int channels = 2;	/* count of channels */
static snd_pcm_sframes_t buffer_size = (4*FFT_SIZE);	/* ring buffer length in us */
static snd_pcm_sframes_t period_size = FFT_SIZE ;	/* period time in us */
static double freq = 440;	/* sinusoidal wave frequency in Hz */
static int verbose = 0;		/* verbose flag */
static int resample = 1;	/* enable alsa-lib resampling */
static int period_event = 0;	/* produce poll event after each period */

static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;

/* static int txtblock[50][100] ; */

static void generate_line(int *fontline,
	       	fftw_complex *in, fftw_complex *out,
	       fftw_plan p, 	float *audiobuf)
{
	int i, j ;
	complex ExpStep, CurExp;
        double NormFreqOffset;
	int cnt_freqs=0;
	double fractpart, intpart, dphase;


        NormFreqOffset = -2.0*PI*(FREQ_OFFSET)/Fs;
/* 	printf("NormFreqOffset %g \n", NormFreqOffset); */
	CurExp = (1.0 + 0.0*I);
	ExpStep = (cos(NormFreqOffset) + I*sin(NormFreqOffset));

	for (i=0; i < FFT_SIZE; i++)
	{
		in[i]=( 0.0 + 0.0*I) ;  /* no data yet */
	}
	cnt_freqs=0;
	for (i=0; i < 100; i++)
	{
		if (fontline[i] > 0.0 )
		{
			for (j=0; j < PIX_WIDTH; j++)
			{
				cnt_freqs++;
			}
		}
	}
	for (i=0; i < 100; i++)
	{
		if (fontline[i] > 0.0 )
		{
			for (j=0; j < PIX_WIDTH; j++)
			{
				if ( ((i*4+j)%2)==0)
				{
			        	in[i*PIX_WIDTH+j]= (FREQ_AMPLITUDE/(sqrt(cnt_freqs)) + 0.0*I) ;  
				}
				else 
				{
			        	in[i*PIX_WIDTH+j]= (-FREQ_AMPLITUDE/(sqrt(cnt_freqs)) + 0.0*I) ;  
				}

			}
		}
	}
	fftw_execute(p);
	/* now shift to higher center frequency */

	for (i=0; i < FFT_SIZE; i++)
	{
		out[i] *= conj(CurExp) ;
		CurExp *= ExpStep;
	/* 	printf("real CurExp %g imag CurExp %g\n", creal(CurExp), cimag(CurExp)); */
		audiobuf[i] = (float) creal(out[i]);
	}
	return;
}

static int
set_hwparams (snd_pcm_t * handle,
	      snd_pcm_hw_params_t * params, snd_pcm_access_t access)
{
  unsigned int rrate;
  snd_pcm_uframes_t size;
  int err, dir;

  /* choose all parameters */
  err = snd_pcm_hw_params_any (handle, params);
  if (err < 0)
    {
      printf
	("Broken configuration for playback: no configurations available: %s\n",
	 snd_strerror (err));
      return err;
    }
  /* set hardware resampling */
  err = snd_pcm_hw_params_set_rate_resample (handle, params, resample);
  if (err < 0)
    {
      printf ("Resampling setup failed for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  /* set the interleaved read/write format */
  err = snd_pcm_hw_params_set_access (handle, params, access);
  if (err < 0)
    {
      printf ("Access type not available for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  /* set the sample format */
  err = snd_pcm_hw_params_set_format (handle, params, format);
  if (err < 0)
    {
      printf ("Sample format not available for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  /* set the count of channels */
  err = snd_pcm_hw_params_set_channels (handle, params, channels);
  if (err < 0)
    {
      printf ("Channels count (%i) not available for playbacks: %s\n",
	      channels, snd_strerror (err));
      return err;
    }
  /* set the stream rate */
  rrate = rate;
  err = snd_pcm_hw_params_set_rate_near (handle, params, &rrate, 0);
  if (err < 0)
    {
      printf ("Rate %iHz not available for playback: %s\n", rate,
	      snd_strerror (err));
      return err;
    }
  if (rrate != rate)
    {
      printf ("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
      return -EINVAL;
    }
  /* set the buffer time */
  err =
    snd_pcm_hw_params_set_buffer_size (handle, params,  buffer_size);
  if (err < 0)
    {
      printf ("Unable to set buffer time %i for playback: %s\n", buffer_size,
	      snd_strerror (err));
      return err;
    }
  err = snd_pcm_hw_params_get_buffer_size (params, &size);
  if (err < 0)
    {
      printf ("Unable to get buffer size for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  buffer_size = size;
  printf("in txt2sound buffer_size %d\n", buffer_size);

  /* set the period size */
  err =
    snd_pcm_hw_params_set_period_size (handle, params, period_size, 0);
  if (err < 0)
    {
      printf ("Unable to set period time %i for playback: %s\n", period_size,
	      snd_strerror (err));
      return err;
    }
  err = snd_pcm_hw_params_get_period_size (params, &size, &dir);
  if (err < 0)
    {
      printf ("Unable to get period size for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  period_size = size;
  printf("txt2sound period_size %d \n", period_size);

  /* write the parameters to device */
  err = snd_pcm_hw_params (handle, params);
  if (err < 0)
    {
      printf ("Unable to set hw params for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  return 0;
}

static int
set_swparams (snd_pcm_t * handle, snd_pcm_sw_params_t * swparams)
{
  int err;

  /* get the current swparams */
  err = snd_pcm_sw_params_current (handle, swparams);
  if (err < 0)
    {
      printf ("Unable to determine current swparams for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  /* start the transfer when the buffer is almost full: */
  /* (buffer_size / avail_min) * avail_min */
  err =
    snd_pcm_sw_params_set_start_threshold (handle, swparams,
					   (buffer_size / period_size) *
					   period_size);
  if (err < 0)
    {
      printf ("Unable to set start threshold mode for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  /* allow the transfer when at least period_size samples can be processed */
  /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
  err =
    snd_pcm_sw_params_set_avail_min (handle, swparams,
				     period_event ? buffer_size :
				     period_size);
  if (err < 0)
    {
      printf ("Unable to set avail min for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  /* enable period events when requested */
  if (period_event)
    {
      err = snd_pcm_sw_params_set_period_event (handle, swparams, 1);
      if (err < 0)
	{
	  printf ("Unable to set period event: %s\n", snd_strerror (err));
	  return err;
	}
    }
  /* write the parameters to the playback device */
  err = snd_pcm_sw_params (handle, swparams);
  if (err < 0)
    {
      printf ("Unable to set sw params for playback: %s\n",
	      snd_strerror (err));
      return err;
    }
  return 0;
}

/*
*   Underrun and suspend recovery
*/

static int
xrun_recovery (snd_pcm_t * handle, int err)
{
  if (verbose)
    printf ("stream recovery\n");
  if (err == -EPIPE)
    {				/* under-run */
      err = snd_pcm_prepare (handle);
      if (err < 0)
	printf ("Can't recovery from underrun, prepare failed: %s\n",
		snd_strerror (err));
      return 0;
    }
  else if (err == -ESTRPIPE)
    {
      while ((err = snd_pcm_resume (handle)) == -EAGAIN)
	sleep (1);		/* wait until the suspend flag is released */
      if (err < 0)
	{
	  err = snd_pcm_prepare (handle);
	  if (err < 0)
	    printf ("Can't recovery from suspend, prepare failed: %s\n",
		    snd_strerror (err));
	}
      return 0;
    }
  return err;
}
static void filltxtblock( char *txtext, int txtblock[][100])
{
	int i, j, k, l, ltrindex ;
	int line_cnt, char_cnt, txt_cnt;
	char teken;
	int txtptr;
	static int letters[26][10][10] =
	{
		{                                  /*   A */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,1,1,0,0,0,0,0},
			{0,0,1,0,0,1,0,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,1,1,1,1,1,1,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                                /* B  */
			{0,0,0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,0,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,1,1,1,1,1,0,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,1,1,1,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                                /* C */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                              /* D */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0}
		},
		{                               /* E */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                              /* F */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                             /* G */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,1,1,1,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                            /* H */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,1,1,1,1,1,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                             /* I */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                            /* J */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,0,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{1,1,1,1,0,0,0,0,0,0}
		},
		{                          /* K */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,1,0,0,0,0,0},
			{1,0,0,1,0,0,0,0,0,0},
			{1,0,1,0,0,0,0,0,0,0},
			{1,1,0,0,0,0,0,0,0,0},
			{1,1,0,0,0,0,0,0,0,0},
			{1,0,1,0,0,0,0,0,0,0},
			{1,0,0,1,0,0,0,0,0,0},
			{1,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                         /* L */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},	
			{1,0,0,0,0,0,0,0,0,0},	
			{1,0,0,0,0,0,0,0,0,0},	
			{1,0,0,0,0,0,0,0,0,0},	
			{1,0,0,0,0,0,0,0,0,0},	
			{1,0,0,0,0,0,0,0,0,0},	
			{1,0,0,0,0,0,0,0,0,0},	
			{1,1,1,1,1,1,0,0,0,0}
		},
		{                        /* M */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,1,0,0,0,0,1,1,0,0},
			{1,0,1,0,0,1,0,1,0,0},
			{1,0,0,1,1,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                       /* N */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,1,0,0,0,0,0,1,0,0},
			{1,0,1,0,0,0,0,1,0,0},
			{1,0,0,1,0,0,0,1,0,0},
			{1,0,0,0,1,0,0,1,0,0},
			{1,0,0,0,0,1,0,1,0,0},
			{1,0,0,0,0,0,1,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                         /* O */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                       /* P */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                      /* Q */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,1,0,0,0},
			{1,0,0,0,1,0,1,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{0,0,1,1,1,0,1,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                       /* R */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{1,1,1,1,1,0,0,0,0,0},
			{1,0,1,0,0,0,0,0,0,0},
			{1,0,0,1,0,0,0,0,0,0},
			{1,0,0,0,1,0,0,0,0,0},
			{1,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                       /* S */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,1,0,0,0},
			{0,1,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,0,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{1,1,1,1,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                     /* T */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,1,1,1,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                     /* U */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,0,1,1,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                  /* V */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,1,0},
			{1,0,0,0,0,0,0,0,1,0},
			{1,0,0,0,0,0,0,0,1,0},
			{1,0,0,0,0,0,0,0,1,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,0,1,0,0,0,1,0,0,0},
			{0,0,0,1,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                   /* W */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,1,0,0,0,1,0},
			{1,0,0,0,1,0,0,0,1,0},
			{1,0,0,0,1,0,0,0,1,0},
			{1,0,0,0,1,0,0,0,1,0},
			{1,0,0,0,1,0,0,0,1,0},
			{1,0,0,0,1,1,0,0,1,0},
			{0,1,0,1,0,0,1,0,1,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                    /* X */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,0,1,0,0,1,0,0,0,0},
			{0,0,0,1,1,0,0,0,0,0},
			{0,0,0,1,1,0,0,0,0,0},
			{0,0,1,0,0,1,0,0,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                 /* Y */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,0,1,0,0,1,0,0,0,0},
			{0,0,0,1,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{                   /* Z */
			{0,0,0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,1,0,0}
		}
	} ;
	static int cijfers[12][10][10] =
	{ 
		{           /*  0 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,0,0,1,0,0},
			{0,1,0,0,0,1,1,0,0,0},
			{1,0,0,0,0,1,1,0,0,0},
			{1,0,0,0,1,0,1,0,0,0},
			{1,0,0,1,0,0,1,0,0,0},
			{1,0,1,0,0,0,1,0,0,0},
			{0,1,0,0,0,1,0,0,0,0},
			{1,0,1,1,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{         /* 1 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,1,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,1,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{        /* 2 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,1,1,1,1,0,0,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,1,1,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{         /* 3 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,1,1,1,1,0,0,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,0,0,1,1,0,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,0,0,1,1,1,1,0,0,0}
		},
		{        /* 4 */
			{0,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,1,0,0,0,0,0},
			{1,0,0,0,1,0,0,0,0,0},
			{1,1,1,1,1,1,1,1,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{         /* 5 */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,1,1,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{1,1,1,1,1,1,0,0,0,0}
		},
		{       /* 6 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,1,1,1,1,0,0,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,0,0,0,0},
			{0,1,0,0,1,1,0,0,0,0},
			{0,1,0,1,0,0,1,0,0,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,1,0,0,0},
			{0,0,1,1,1,1,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{       /* 7 */
			{0,0,0,0,0,0,0,0,0,0},
			{1,1,1,1,1,1,1,1,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{          /* 8 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,1,1,1,1,1,0,0,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,0,1,1,1,1,1,0,0,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,1,0,0,0,0,0,1,0,0},
			{0,0,1,1,1,1,1,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{         /* 9 */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,1,1,1,1,0,0,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,0,1,0,0,0,0,1,0,0},
			{0,0,1,1,1,1,1,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,1,1,1,1,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{         /* slash /  */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,1,0,0},
			{0,0,0,0,0,0,1,0,0,0},
			{0,0,0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,0,0,0,0},
			{0,0,0,1,0,0,0,0,0,0},
			{0,0,1,0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		},
		{          /* space */
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0,0,0,0}
		}
	};


/* reset block */
        for (i=0; i < 50; i++)
	{
		for (j=0; j < 100; j++)
		{
			txtblock[i][j]=0;
		}
	}
        txt_cnt = strlen(txtext);
	line_cnt = 0;
	char_cnt =0 ;
	txtptr =0;
	while (txt_cnt > 0)
	{
		if ((txtext[txtptr]==10)||(char_cnt==10))   /* LF */
		{
			char_cnt=0;
			line_cnt++;
		}
		if (txtext[txtptr]==32)   /* space */
		{
			char_cnt++;
		}
		if (isupper(txtext[txtptr]))
		{
			for (i=0; i < 10; i++)
			{
				for (j=0; j < 10; j++)
				{
                                 txtblock[i+EXTRA_LINE_SPACE*line_cnt][j+char_cnt*10]=letters[txtext[txtptr]-65][9-i][j];
				}
			}
			char_cnt++ ;
		}
		if (isdigit(txtext[txtptr]))
		{
			for (i=0;i < 10; i++)
			{
				for (j=0; j < 10; j++)
				{
                                 txtblock[i+EXTRA_LINE_SPACE*line_cnt][j+char_cnt*10]=cijfers[txtext[txtptr]-48][9-i][j];
				}
			}
			char_cnt++;
		}
		if (txtext[txtptr]==47)  /* slash */
		{
			for (i=0;i < 10; i++)
			{
				for (j=0; j < 10; j++)
				{
                                 txtblock[i+EXTRA_LINE_SPACE*line_cnt][j+char_cnt*10]=cijfers[10][9-i][j];
				}
			}
			char_cnt++;
		}
	
			txtptr++;
			txt_cnt--;
	}
	    
}

/*
 *   Transfer method - write only
					  */

static int
write_loop (snd_pcm_t * handle, fftw_complex *in, fftw_complex *out,
            fftw_plan p, float *audiobufin,  float *audiobuffilt,
	    float *Zifilt, float *lpfilt,
	    signed short *samples, snd_pcm_channel_area_t * areas,
	     char *txtext, int txtblock[][100], int nr_of_lines)
{
  signed short *ptr;
  int err, cptr, i, j;
  filltxtblock(txtext, txtblock);
/*    for (i=0; i < 10; i++)
    {
	    printf("kols line %d ", i);
		    for (j=0; j< 20 ; j++)
		    {
			    printf(" %d ",  txtblock[i][j]);
		    }
		    printf("\n");
    } */
    for (j=0; j < 15*nr_of_lines ; j++)
    {
      generate_line (txtblock[j], in, out, p, audiobufin );
       filter(audiobufin, audiobuffilt, lpfilt, Zifilt,
              FFT_SIZE, LP_LENGTH); 
   /*   for (i=0; i <100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp(-(100-i)*0.100));
      }  */
      for (i=0; i <FFT_SIZE; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) audiobuffilt[i];
      }
    /*  for (i=FFT_SIZE-100; i <FFT_SIZE; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp((FFT_SIZE-100 -i)*0.1));
      } */

      /* debugging 
      for (i=0; i < FFT_SIZE ; i++)
      {
	      printf("%d %d\n", i, samples[2*i]);
      }
      */
      ptr = samples;
      cptr = period_size;
      while (cptr > 0)
	{
	  err = snd_pcm_writei (handle, ptr, cptr);
	  if (err == -EAGAIN)
	    continue;
	  if (err < 0)
	    {
	      if (xrun_recovery (handle, err) < 0)
		{
		  printf ("Write error: %s\n", snd_strerror (err));
		  exit (EXIT_FAILURE);
		}
	      break;		/* skip one period */
	    }
	  ptr += err * channels;
	  cptr -= err;
	}
      generate_line (txtblock[j], in, out, p, audiobufin );
       filter(audiobufin, audiobuffilt, lpfilt, Zifilt,
              FFT_SIZE, LP_LENGTH); 
      for (i=0; i <100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp(-(100-i)*0.100));
      }
      for (i=100; i <FFT_SIZE-100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) audiobuffilt[i];
      }
      for (i=FFT_SIZE-100; i <FFT_SIZE; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp((FFT_SIZE-100 -i)*0.1));
      }
      /* debugging 
      for (i=0; i < FFT_SIZE ; i++)
      {
	      printf("%d %d\n", i, samples[2*i]);
      }
      */
      ptr = samples;
      cptr = period_size;
      while (cptr > 0)
	{
	  err = snd_pcm_writei (handle, ptr, cptr);
	  if (err == -EAGAIN)
	    continue;
	  if (err < 0)
	    {
	      if (xrun_recovery (handle, err) < 0)
		{
		  printf ("Write error: %s\n", snd_strerror (err));
		  exit (EXIT_FAILURE);
		}
	      break;		/* skip one period */
	    }
	  ptr += err * channels;
	  cptr -= err;
	}
      generate_line (txtblock[j], in, out, p, audiobufin );
       filter(audiobufin, audiobuffilt, lpfilt, Zifilt,
              FFT_SIZE, LP_LENGTH); 
      for (i=0; i <100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp(-(100-i)*0.100));
      }
      for (i=100; i <FFT_SIZE-100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) audiobuffilt[i];
      }
      for (i=FFT_SIZE-100; i <FFT_SIZE; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp((FFT_SIZE-100 -i)*0.1));
      }
      /* debugging 
      for (i=0; i < FFT_SIZE ; i++)
      {
	      printf("%d %d\n", i, samples[2*i]);
      }
      */
      ptr = samples;
      cptr = period_size;
      while (cptr > 0)
	{
	  err = snd_pcm_writei (handle, ptr, cptr);
	  if (err == -EAGAIN)
	    continue;
	  if (err < 0)
	    {
	      if (xrun_recovery (handle, err) < 0)
		{
		  printf ("Write error: %s\n", snd_strerror (err));
		  exit (EXIT_FAILURE);
		}
	      break;		/* skip one period */
	    }
	  ptr += err * channels;
	  cptr -= err;
	}
      generate_line (txtblock[j], in, out, p, audiobufin );
       filter(audiobufin, audiobuffilt, lpfilt, Zifilt,
              FFT_SIZE, LP_LENGTH); 
      for (i=0; i <100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp(-(100-i)*0.100));
      }
      for (i=100; i <FFT_SIZE-100; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) audiobuffilt[i];
      }
      for (i=FFT_SIZE-100; i <FFT_SIZE; i++)
      {
          samples[2*i]= samples[2*i+1] = (signed short ) (audiobuffilt[i]*exp((FFT_SIZE-100 -i)*0.1));
      }
      /* debugging 
      for (i=0; i < FFT_SIZE ; i++)
      {
	      printf("%d %d\n", i, samples[2*i]);
      }
      */
      ptr = samples;
      cptr = period_size;
      while (cptr > 0)
	{
	  err = snd_pcm_writei (handle, ptr, cptr);
	  if (err == -EAGAIN)
	    continue;
	  if (err < 0)
	    {
	      if (xrun_recovery (handle, err) < 0)
		{
		  printf ("Write error: %s\n", snd_strerror (err));
		  exit (EXIT_FAILURE);
		}
	      break;		/* skip one period */
	    }
	  ptr += err * channels;
	  cptr -= err;
	}
    }
}


int
main (int argc, char *argv[])
{
  snd_pcm_t *handle;
  int err;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;
  int method = 0;
  signed short *samples;
  unsigned int chn;
  snd_pcm_channel_area_t *areas;
  int verbose=1;

  snd_pcm_hw_params_alloca (&hwparams);
  snd_pcm_sw_params_alloca (&swparams);

  int pixelline[100] ;
  int i, j, resread ;
  signed short sndsample[FFT_SIZE]; 
  fftw_complex *in, *out ;
  fftw_plan p;
  FILE *fplowpass ;
  float lpfilt[LP_LENGTH] ;
  float Zifilt[LP_LENGTH];
  float audiobufin[FFT_SIZE];
  float audiobuffilt[FFT_SIZE];
  signed short txaudiobuf[2*FFT_SIZE];
  char txtext[40];
  char linebuf[12];
  FILE *fpwfal;
  static int txtblock[50][100];
  int indxdevout ;
  int nr_of_lines ;
  if (argc != 2)
  {
    printf("needed cmdline arg nr_devout\n");
    exit(1);
  }
  indxdevout = atoi(argv[1]);
  printf("indxdevout is %d \n", indxdevout);
  device = snddevices[indxdevout]; 
  in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex)*FFT_SIZE);
  if (in == NULL)
  {
	   printf("cannot fftw_malloc in \n");
	   exit(1);
  }
  out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex)*FFT_SIZE);
  if (out == NULL)
  {
	   printf("canno fftw_malloc out\n");
	   exit(1);
  }
  p = fftw_plan_dft_1d(FFT_SIZE, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
   
  /* now read lowpass filter data */
  fplowpass = fopen("mylp3kc_97.dat", "r");
  if (fplowpass == NULL)
  {
	   printf("Cannot open mylp3kc_97.dat\n");
	   exit(1);
  }
  for (i=0; i < LP_LENGTH; i++)
  {
	   resread= fscanf(fplowpass, "%g", &lpfilt[i]);
  }
  if ( resread != 1)
  {
     printf("premature end of file mylp3kc_97.dat\n");
     exit(1);
  }
  for (i=0; i < LP_LENGTH; i++)
  {
    Zifilt[i] = 0.0 ;
  }


  err = snd_output_stdio_attach (&output, stdout, 0);
  if (err < 0)
    {
      printf ("Output failed: %s\n", snd_strerror (err));
      return 0;
    }

  printf ("Playback device is %s\n", device);
  printf ("Stream parameters are %iHz, %s, %i channels\n", rate,
	  snd_pcm_format_name (format), channels);

  if ((err = snd_pcm_open (&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
      printf ("Playback open error: %s\n", snd_strerror (err));
      return 0;
    }

  if ((err =
       set_hwparams (handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
      printf ("Setting of hwparams failed: %s\n", snd_strerror (err));
      exit (EXIT_FAILURE);
    }
  if ((err = set_swparams (handle, swparams)) < 0)
    {
      printf ("Setting of swparams failed: %s\n", snd_strerror (err));
      exit (EXIT_FAILURE);
    }

  if (verbose > 0)
    snd_pcm_dump (handle, output);

  samples =
    malloc ((period_size * channels *
	     snd_pcm_format_physical_width (format)) / 8);
  if (samples == NULL)
    {
      printf ("Not enough memory\n");
      exit (EXIT_FAILURE);
    }

  areas = calloc (channels, sizeof (snd_pcm_channel_area_t));
  if (areas == NULL)
    {
      printf ("Not enough memory\n");
      exit (EXIT_FAILURE);
    }
  for (chn = 0; chn < channels; chn++)
    {
      areas[chn].addr = samples;
      areas[chn].first = chn * snd_pcm_format_physical_width (format);
      areas[chn].step = channels * snd_pcm_format_physical_width (format);
    }
   /* now get text to be sent from file  max 33 chars in 3 line is ended with lf */
   if ((fpwfal=fopen("wfal.txt","r"))==NULL)
   {
     printf("Cannot open wfal.txt\n");
     exit(1);
   }
   nr_of_lines =0;
   fgets(linebuf, 12, fpwfal);
   strcpy(txtext, linebuf);
   i = strlen(txtext);
   if (strlen(linebuf) > 1 ) nr_of_lines++ ;
   fgets(linebuf,12,fpwfal);
   strcpy(txtext+i, linebuf);
   if (strlen(linebuf) > 1 ) nr_of_lines++ ;
   i = strlen(txtext);
   fgets(linebuf, 12, fpwfal);
   strcpy(txtext+i, linebuf);
   if (strlen(linebuf) > 1 ) nr_of_lines++ ;
   printf("text to be sent is %s nr_of_lines is %d\n", txtext, nr_of_lines); 
   
   err =write_loop(handle, in, out, p, audiobufin, audiobuffilt,
		  Zifilt, lpfilt,  samples, areas,  txtext, txtblock, nr_of_lines);
  /* err = transfer_methods[method].transfer_loop (handle, samples, areas); */
  if (err < 0)
    printf ("Transfer failed: %s\n", snd_strerror (err));

  free (areas);
  free (samples);
  snd_pcm_close (handle);
  printf("txtwfal finished\n");
  return 0;
}



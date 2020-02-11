
/*
*    File filter.c
*
* implements filter routine a la MATLAB
*
* PA0MBO - M.Bos
* Date November 7th 2006
*
* result is stored in y[], filter is h[], Zi[] = state vector
* n is number of elements in input (sigin) and m is length of
* the filter vector h[]
*
*/
#include <stdio.h>
void filter(float *sigin, float *y, float *h, float *Zi, int n, int m)
{
  int i, j, k, l, p, pp;

  l = m - 1;
  for (i = 0; i < n; i++)

    {
      if (i < l)
	y[i] = Zi[i];

      else
	y[i] = 0.0;
      for (j = 0; ((j <= i) && (j < m)); j++)

	{
	  y[i] += h[j] * sigin[i - j];
	}
    }
  i = n - 1;
  for (k = 0; k < l; k++)

    {
      pp = 0;
      Zi[k] = 0.0;
      for (p = k; p < l; p++)

	{
	  Zi[k] += h[p + 1] * sigin[i - pp++];
	}
    }
}

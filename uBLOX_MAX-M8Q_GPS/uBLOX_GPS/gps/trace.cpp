#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char buffer[255];
int pos[25];
int tab = 0;
int pointer=0;

void trace_in(const char *name)
{
  int i;
  printf("\n");
  for ( i=0; i<tab; i++)
    printf("| ");
  printf("%s.", name);
  strcpy(&buffer[pointer], name);
  pos[tab++] = pointer;
  pointer += strlen(name);
}

int trace_out()
{
   tab--;
   pointer = pos[tab];
  int i;
  printf("\n");
  for ( i=0; i<tab; i++)
    printf("| ");
   printf("done.");
}


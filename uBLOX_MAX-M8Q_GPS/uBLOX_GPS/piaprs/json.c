#include <stdlib.h>
#include <stdio.h>
#include "json.h"

FILE *inf;
char peek = ' ';
int eof = 0;

char getnextch()
{
  char rv = peek;
  while (!feof(inf))
  {
     peek = fgetc(inf);
     if ((peek != ' ') && (peek!='\r') && (peek!='\n') && (peek!='\t'))
     {
//       printf("%c",peek);
       return 1;
     }
  }
  
}

int lookfor(char expected)
{
//   printf("\nlooking for \"%c\":",expected);
   while (!eof)
   {
      if (peek == expected)
      {
//        printf("found");
        getnextch();
        return 1;
      }
      getnextch();
   }
   return 0;
}

void gettoken(char *token)
{
//   printf("\nreading token :");
   if (peek=='\"')
   {
//     printf("\"");
     while (!feof(inf))
     {
        peek = fgetc(inf);
//        printf("%c",peek);
        if (peek == '\"')
        {
           getnextch();
           break;
        }
        *token++ = peek;
     }
     *token++ = '\0';
//     printf("\"");
   }
   else
   {
      while (!eof)
      {
         if ((peek==':') || (peek==',') || (peek=='}'))
        {
           break;
        }         
        *token++ = peek;
        getnextch();
      }
     *token++ = '\0';
   }
}

void json_parse(char *filename, json_callback handler, void *param)
{
  char key[20];
  char val[80];
  
  inf = fopen(filename, "r");
  if (inf == NULL)
  {
     printf("error opening file %s", filename);
  }
//  printf("start parsing");
  lookfor('{');
  while ((!eof) && (peek!='}'))
  {
    gettoken(key);
    lookfor(':');
    gettoken(val);
    json_handler(key, val, param); 

    if (peek==',')
      lookfor(',');
  }
  fclose(inf);
}

//-----------------------------------------------------

void example_json_handler(char *token, char *value, void *param)
{
  printf("\n\n%s = %s\n\n",token, value);
}

void example()
{
  char *filename = "../utils/gps.json";
  json_parse(filename, example_json_handler, NULL);
}
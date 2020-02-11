#include<iostream>
#include<exception>
#include "ublox.h"
#include "trace.h"

using namespace std;
int main(void)
{
trace_in("main");
    Ublox *gps = new Ublox();
    while(1)
    {
      try
      {
        cout << "starting gps";
        gps->run();
      }
      catch (exception& e)
      {
         cout << "std exception: " << e.what() << '\n';
      }
      catch (...) {
         cout << "Exception occurred";
      }
   }
trace_out();
    return 0;
}	

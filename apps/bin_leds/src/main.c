#include <System.h>
#include <stdio.h>

int main(){
  /* basic initializations */
  InitializeSystem();
  InitializeLEDs();
  EnableDebugOutput(DEBUG_ITM);

  int time = 0;
  while(1){
    /* Luckily, the LED setting function works just like a binary clock, i.e.
     * SetLEDs(n) turns the i-th LED on if and only if the i-th binary digit of
     * n is 1.
     */
    SetLEDs(time);
    time = (time + 1) % 16;
    /* wait approx. 1s */
    Delay(100);
  }

}


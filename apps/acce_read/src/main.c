#include <System.h>
#include <Accelerometer.h>
#include <stdio.h>

int main()
{
	/* initializations */
	InitializeSystem();
	EnableDebugOutput(DEBUG_ITM);
	InitializeLEDs();
	InitializeAccelerometer();
	CalibrateAccelerometer();

	int8_t data[3] = {};
	while(1)
	{
		/* keep an LED on while reading the accelerometer... */
		SetLEDs(1);

                /* read and print accelerometer data */
		ReadCalibratedAccelerometerData(data);

		iprintf("Roll:\t%i\n", data[0]);
		iprintf("Pitch:\t%i\n", data[1]);
		iprintf("Yaw:\t%i\n\n", data[2]);

		/* ... and turn it of when we're done */
		SetLEDs(0);

		/* the data flow is HUGE if there is no delay */
		Delay(100);
	}
}

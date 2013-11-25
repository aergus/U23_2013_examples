#include <System.h>
#include <stdio.h>

/* A sine curve is hard-coded. */
#include "sines.h"

#define AudioFreq AudioFreq_22k

#define COORD_X 0
#define COORD_Y 1
#define BUTTON  2

/* quick double-and-add hack to calculate powers of two */
uint16_t daa_pow(uint16_t b, uint16_t e){
  if(e == 0)
    return 1;
	else if( e % 2 == 0)
		return daa_pow(b*b, e/2);
	else
		return b*daa_pow(b*b, (e-1)/2);
}

void ReadData(uint16_t data[3]){
	ADC_SoftwareStartConv(ADC1);
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	data[COORD_X] = ADC_GetConversionValue(ADC1);
	ADC_SoftwareStartConv(ADC2);
	while(!ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC));
	data[COORD_Y] = ADC_GetConversionValue(ADC2);
	data[BUTTON] = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1);
}

static void AudioCallback(void *context, int16_t buffer[256]){
	uint16_t values[3];
	ReadData(values);

	/* process the data */
	double amp_scale = 10000.0 + 2000*(values[COORD_Y]/512);
	uint16_t freq_scale = daa_pow(2, values[COORD_X]/512);
	if(values[BUTTON] == Bit_RESET)
		SetLEDs(1 | 2 | 8);
	else
		SetLEDs(4);
	iprintf("amplitude scale: %i, frequence scale: %i\n", (uint16_t) amp_scale, freq_scale);
	
	for(uint32_t i = 0; i < 128; i++) {
	  uint16_t val =  (int16_t) amp_scale*sines[(freq_scale*i) % 1024];
		buffer[2*i+0] = buffer[2*i+1] = val;
	}
}

int main(void){
	/* initialize stuff */
	InitializeSystem();
	InitializeAudio(AudioFreq);
	InitializeLEDs();
	EnableDebugOutput(DEBUG_ITM);

	/* enable the clocks required */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); /* pin for the button */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); /* pins for ADC */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); /* ADC1 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE); /* ADC2 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); /* timer */

	/* initialize the pins in the respective modes */
	GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
		.GPIO_Speed = GPIO_Speed_50MHz,
		.GPIO_Mode = GPIO_Mode_IN,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_PuPd = GPIO_PuPd_UP, /* The button needs pullup. */
		.GPIO_Pin = GPIO_Pin_1
	});
	GPIO_Init(GPIOC, &(GPIO_InitTypeDef){
		.GPIO_Speed = GPIO_Speed_50MHz,
		.GPIO_Mode = GPIO_Mode_AN,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_PuPd = GPIO_PuPd_NOPULL,
		.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1
	});

	/* initialize ADC */
	ADC_Init(ADC1, &(ADC_InitTypeDef){
		.ADC_Resolution = ADC_Resolution_12b,
		.ADC_ScanConvMode = DISABLE,
		.ADC_ContinuousConvMode = ENABLE, 
		.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None,
		.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1,
		.ADC_DataAlign = ADC_DataAlign_Right,
		.ADC_NbrOfConversion = 1
	});
	ADC_Init(ADC2, &(ADC_InitTypeDef){
		.ADC_Resolution = ADC_Resolution_12b, 
		.ADC_ScanConvMode = DISABLE,
		.ADC_ContinuousConvMode = ENABLE, 
		.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None,
		.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1,
		.ADC_DataAlign = ADC_DataAlign_Right,
		.ADC_NbrOfConversion = 1
	});

	/* set the channels and start the ADCs */
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_3Cycles);
	ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, ADC_SampleTime_3Cycles);
	ADC_Cmd(ADC1, ENABLE);
	ADC_Cmd(ADC2, ENABLE);

	SetAudioVolume(0xa0);
	PlayAudioWithCallback(AudioCallback, NULL);

	while(1){
		/* It's strange that nothing happens if we don't delay. */
	  Delay(100);
	}

	return 0;
}

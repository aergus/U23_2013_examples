#include <System.h>
#include <stm32f4xx.h>

int main()
{
	/* basic initializations */
	InitializeSystem();

	/* clock for GPIOA (user button), GPIOD (LEDS) and timer 4 */
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/* tick with frequency 100kHz and period 500ms */
        uint32_t PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 100000) - 1;
        uint32_t period = 50000;

        /* configure the timer */
        TIM_TimeBaseInit(TIM4, &(TIM_TimeBaseInitTypeDef){
                .TIM_Period = period - 1,
                .TIM_Prescaler = PrescalerValue,
                .TIM_ClockDivision = 0,
                .TIM_CounterMode = TIM_CounterMode_Up,
        });

	/* start the timer */
	TIM_Cmd(TIM4, ENABLE);

        /* set output mode for PD12 to PD15 (i.e. LEDs) */
        GPIO_Init(GPIOD, &(GPIO_InitTypeDef){
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode = GPIO_Mode_OUT,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd = GPIO_PuPd_UP,
                .GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15
        });

	/* set input mode for PA0 (i.e. the user button) */
	GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
                .GPIO_Speed = GPIO_Speed_50MHz,
                .GPIO_Mode = GPIO_Mode_IN,
                .GPIO_OType = GPIO_OType_PP,
                .GPIO_PuPd = GPIO_PuPd_NOPULL,  //no internal pullup or pulldown, is present on PCB
                .GPIO_Pin = GPIO_Pin_0
        });

	/* use EXTI0 (since we want PA0 to trigger interrupts) */
        EXTI_Init(&(EXTI_InitTypeDef){
                .EXTI_Line = EXTI_Line0,
                .EXTI_Mode = EXTI_Mode_Interrupt,
                .EXTI_Trigger = EXTI_Trigger_Rising,
                .EXTI_LineCmd = ENABLE
        });

	/* connect PA0 actively to EXTI */
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

	/* last but not least, enable the interrupt controller! */
        NVIC_Init(&(NVIC_InitTypeDef){
                .NVIC_IRQChannel = EXTI0_IRQn,
                .NVIC_IRQChannelPreemptionPriority = 0x00,
                .NVIC_IRQChannelSubPriority = 0x00,
                .NVIC_IRQChannelCmd = ENABLE
        });

	/* turn the first LED in the beginning */
	GPIO_SetBits(GPIOD, GPIO_Pin_13);

	/* do NOTHING, i.e. just wait for the interrupts */
	while(1);
}

void setNextLED()
{
	/* Let's see which LED is on... */
	int16_t gpio_data = GPIO_ReadOutputData(GPIOD);
	/* These are our LEDs: */
	uint16_t leds[4] = {GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15};
	/* find the LED which comes afte the LED which is currently on */
	int i, next;
	for(i=0; i<4; i++){
		if(gpio_data & leds[i]){
			next = (i+1) % 4;
		}
	}
	/* turn (only) the next LED on */
	gpio_data = leds[next];
	GPIO_Write(GPIOD, gpio_data);
}

/* Here's our interrupt handler: */
void EXTI0_IRQHandler()
{
        if(EXTI_GetITStatus(EXTI_Line0) != RESET)
        {
		/* We want to keep turning the next
		 * LED on while the button is pressed.
		 */
		while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == Bit_SET)
		{
			/* reset to counter in order to... */
			TIM_SetCounter(TIM4, 0);
			uint32_t count;
			/* ... wait a little bit... */
			while((count=TIM_GetCounter(TIM4)) < 40000){
				/* stop bothering when the button
				 * isn't pressed anymore */
				if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != Bit_SET)
				{
					/* change the LED anyway if the button
					 * was kept pressed long enough
					 */
					if(count > 10000)
					{
						setNextLED();
					}
					EXTI_ClearITPendingBit(EXTI_Line0);
					return;
				}
			}
			/* ... and then turn the next LED on */
			setNextLED();
		}
		EXTI_ClearITPendingBit(EXTI_Line0);
		return;
        }
}


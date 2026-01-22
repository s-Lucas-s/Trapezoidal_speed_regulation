#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"

#define time 1

void EN(uint8_t i)
{
	if(i)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_8);
	}else{
		GPIO_ResetBits(GPIOA,GPIO_Pin_8);
	}
}


void DIR(uint8_t i)
{
	if(i)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_9);
	}else{
		GPIO_ResetBits(GPIOA,GPIO_Pin_9);
	}
}


void PUL(uint8_t i)
{
	if(i)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_10);
	}else{
		GPIO_ResetBits(GPIOA,GPIO_Pin_10);
	}
}

void A_PUL(void)
{
	PUL(0);
	Delay_ms(time);
	PUL(1);
	Delay_ms(time);
}

int main(void)
{
	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	//EN
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;				/* 复用推挽输出 */
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//DIR
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//PUL
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	DIR(1);
	Delay_ms(1);
	PUL(1);
	
	
	while (1)
	{
		A_PUL();
		
//		Delay_ms(1000);
	}
}

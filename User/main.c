#include "stm32f10x.h"                  // Device header
#include "sys.h"

#define time 1

// void EN(uint8_t i)
// {
// 	if(i)
// 	{
// 		GPIO_SetBits(GPIOA,GPIO_Pin_8);
// 	}else{
// 		GPIO_ResetBits(GPIOA,GPIO_Pin_8);
// 	}
// }


// void DIR(uint8_t i)
// {
// 	if(i)
// 	{
// 		GPIO_SetBits(GPIOA,GPIO_Pin_9);
// 	}else{
// 		GPIO_ResetBits(GPIOA,GPIO_Pin_9);
// 	}
// }


void PUL(uint8_t i)
{
	if(i)
	{
		GPIO_SetBits(GPIOB,GPIO_Pin_8);
	}else{
		GPIO_ResetBits(GPIOB,GPIO_Pin_8);
	}
}

// void CEN(uint8_t i)
// {
//     if (i)
//     {
//         GPIO_SetBits(GPIOA, GPIO_Pin_11);
//     }
//     else
//     {
//         GPIO_ResetBits(GPIOA, GPIO_Pin_11);
//     }
// }

void A_PUL(void)
{
	PUL(0);
	Delay_ms(time);
	PUL(1);
	Delay_ms(time);
}


__IO uint32_t g_set_speed = 100;       /* 最大速度 单位为0.1rad/sec */
__IO uint32_t g_step_accel = 2;        /* 加速度 单位为0.1rad/sec^2 */
__IO uint32_t g_step_decel = 2;        /* 减速度 单位为0.1rad/sec^2 */
__IO uint16_t g_step_angle = 1;         /* 设置的步数*/
extern __IO uint32_t g_add_pulse_count; /* 脉冲个数累计*/
int main(void)
{
    // NVIC_Set();
    // control_init();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // PUL
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; /* 推挽输出 */
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // DIR
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    while (1)
    {
        // create_t_ctrl_param(SPR * g_step_angle, g_step_accel, g_step_decel, g_set_speed);
        // g_add_pulse_count = 0;
        // Delay_ms(10000);
        for (int64_t i = 0; i < 320000;i++)
		{
            A_PUL();
        }
        Delay_s(10);
    }
}

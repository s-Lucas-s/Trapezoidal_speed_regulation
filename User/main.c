#include "stm32f10x.h" // Device header
#include "sys.h"

__IO uint32_t g_set_speed = 80;       /* 最大速度 单位为0.1rad/sec */
__IO uint32_t g_step_accel = 5;       /* 加速度 单位为0.1rad/sec^2 */
__IO uint32_t g_step_decel = 5;       /* 减速度 单位为0.1rad/sec^2 */
__IO uint16_t g_step_angle = 2;        /* 设置的步数*/
extern __IO uint32_t g_add_pulse_count; /* 脉冲个数累计*/
int main(void)
{
    NVIC_Set();
    control_init();
    stepper_start();
    Delay_s(3); // EN引脚控制后适当延迟，防止丢步

    while (1)
    {
        create_t_ctrl_param(SPR * g_step_angle, g_step_accel, g_step_decel, g_set_speed);
        g_add_pulse_count = 0;
        Delay_s(6);
    }
}

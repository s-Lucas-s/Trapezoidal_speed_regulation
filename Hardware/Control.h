#ifndef __CONTROL_H
#define __CONTROL_H

#include "sys.h"

//      Timx     Tim1
//      OC       OC4
//      PUL_GPIO GPIOA
//      PUL_Pin  GPIO_Pin_11
#define DIR_GPIO GPIOA
#define DIR_Pin  GPIO_Pin_12
#define EN_GPIO  GPIOA
#define EN_Pin   GPIO_Pin_8

/******************************************************************************************/

#define TIM_FREQ       72000000U       /* 定时器主频 */
#define MAX_STEP_ANGLE 0.225           /* 最小步距(360°/SPR) */
#define PAI            3.1415926       /* 圆周率*/
#define FSPR           200             /* 步进电机单圈步数 360°/108° */
#define MICRO_STEP     1600            /* 步进电机驱动器细分数 */
#define T1_FREQ        (TIM_FREQ / 36) /* 频率ft值 */
#define SPR            1600            /* 旋转一圈需要的脉冲数          注：雷赛智能的驱动器的细分数为每转脉冲数，即，步数 / 圈 */

/* 数学常数 */

#define ALPHA       ((float)(2 * PAI / SPR)) /* 步距角α = 2*pi/spr */
#define A_T_x10     ((float)(10 * ALPHA * T1_FREQ))
#define T1_FREQ_148 ((float)((T1_FREQ * 0.69) / 10)) /* 0.69为误差修正值 */
#define A_SQ        ((float)(2 * 100000 * ALPHA))
#define A_x200      ((float)(200 * ALPHA)) /* 2*10*10*a/10 */

typedef struct
{
    __IO uint8_t run_state;    /* 电机旋转状态 */
    __IO uint8_t dir;          /* 电机旋转方向 */
    __IO int32_t step_delay;   /* 下个脉冲周期（时间间隔），启动时为加速度 */
    __IO uint32_t decel_start; /* 开始减速位置 */
    __IO int32_t decel_val;    /* 减速阶段步数 */
    __IO int32_t min_delay;    /* 速度最快，计数值最小的值(最大速度，即匀速段速度) */
    __IO int32_t accel_count;  /* 加减速阶段计数值 */
} speedRampData;

enum STA {
    STOP = 0, /* 加减速曲线状态：停止*/
    ACCEL,    /* 加减速曲线状态：加速阶段*/
    DECEL,    /* 加减速曲线状态：减速阶段*/
    RUN       /* 加减速曲线状态：匀速阶段*/
};

/*
 * EN上升沿 -> t1 -> DIR下降沿 -> t2 -> PUL下降沿
 * t1：
 *    a.出厂默认参数时 t1≥350ms；
 *    b.默认参数下，单独开启抱闸功能时 t1≥600ms；
 *     c.默认参数下，单独开启上电自运行功能时 t1≥550ms；
 *    d.默认参数下，开启抱闸、上电自运行功能时 t1≥800ms；
 * t2：
 *     DIR 至少提前 PUL 下降沿 5μs 确定其状态高或低。
 * PUL：
 *    脉冲宽度至少不小于 2.5μs
 */

enum DIR {
    CCW = 0, /* 逆时针 */
    CW       /* 顺时针 */
};

enum EN {
    EN_ON  = 0, /* 失能脱机引脚 */
    EN_OFF = 1  /* 使能脱机引脚 使能后电机停止旋转 */
};

#define Set_DIR(x)                                                               \
    do {                                                                         \
        x ? GPIO_SetBits(DIR_GPIO, DIR_Pin) : GPIO_ResetBits(DIR_GPIO, DIR_Pin); \
    } while (0)

#define Set_EN(x)                                                                 \
    do {                                                                          \
        x ? GPIO_SetBits(EN_GPIO, EN_Pin) : GPIO_ResetBits(EN_GPIO, EN_Pin);      \
    } while (0) /* TIM_CtrlPWMOutputs关闭波形输出 */

void control_init(void);  /* 控制初始化 */
void stepper_start(void); /* 开启步进电机 */
void stepper_stop(void);  /* 关闭步进电机 */

void create_t_ctrl_param(int32_t step, uint32_t accel, uint32_t decel, uint32_t speed); /* 梯形加减速控制函数 */

#endif

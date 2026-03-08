#include "Control.h"

/**
 * @brief       开启步进电机
 * @param       motor_num: 步进电机接口序号
 * @retval      无
 */
void stepper_start(void)
{
    // TOGGLE模式：开启通道4输出 + 使能捕获比较中断
    TIM_CCxCmd(TIM1, TIM_Channel_4, TIM_CCx_Enable);
    TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE); // 开启CC4中断
    // 使能电机
    Set_EN(EN_ON);
    // 开启PWM输出
    TIM_CtrlPWMOutputs(TIM1, ENABLE); 
}

/**
 * @brief       关闭步进电机
 * @param       motor_num: 步进电机接口序号
 * @retval      无
 */
void stepper_stop(void)
{
    // 关闭TOGGLE模式的捕获比较中断+输出
    TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);          // 关闭通道4中断
    TIM_CCxCmd(TIM1, TIM_Channel_4, TIM_CCx_Disable); // 关闭通道4输出
    // 清除中断标志位（避免残留中断触发）
    TIM_ClearITPendingBit(TIM1, TIM_IT_CC4);
    // 失能电机
    Set_EN(EN_OFF);
    // 关闭PWM输出
    TIM_CtrlPWMOutputs(TIM1, DISABLE); 
}

/**
 * @brief       定时器初始化
 * @param       无
 * @retval      无
 */
void Timer_init(void)
{                                                         /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);  // 开启TIM1的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // 开启GPIOA的时钟

    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; // GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 将PA11引脚初始化为复用推挽输出
                                           // 受外设控制的引脚，均需要配置为复用模式

    /*配置时钟源*/
    TIM_InternalClockConfig(TIM1); // 选择TIM1为内部时钟，若不调用此函数，TIM默认也为内部时钟

    /*时基单元初始化*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;              // 定义结构体变量
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 计数器模式，选择向上计数
    TIM_TimeBaseInitStructure.TIM_Period = 65536 - 1;               // 计数周期，即ARR的值
    TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;               // 预分频器，即PSC的值
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            // 重复计数器，高级定时器才会用到
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);             // 将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元

    /*输出比较初始化*/
    TIM_OCInitTypeDef TIM_OCInitStructure;  // 定义结构体变量
    TIM_OCStructInit(&TIM_OCInitStructure); // 结构体初始化

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;           // 输出比较模式
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;     // 输出极性，选择为高，若选择极性为低，则输出高低电平取反
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 输出使能
    TIM_OCInitStructure.TIM_Pulse = 36;                           // 初始的CCR值
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);                      // 将结构体变量交给TIM_OC4Init，配置TIM2的输出比较通道1
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable);            // 关闭预转载

    TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE); // 清除中断标志
    /*TIM使能*/
    TIM_Cmd(TIM1, ENABLE); // 使能TIM1，定时器开始运行
}

/**
 * @brief       电机控制初始化
 * @param       无
 * @retval      无
 */
void control_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // EN
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = EN_Pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; /* 推挽输出 */
    GPIO_Init(EN_GPIO, &GPIO_InitStructure);

    // DIR
    GPIO_InitStructure.GPIO_Pin = DIR_Pin;
    GPIO_Init(DIR_GPIO, &GPIO_InitStructure);

    Set_DIR(CW);
    Set_EN(EN_OFF);

    Timer_init();
}

/********************************************梯形加减速***********************************************/
speedRampData g_srd = {STOP, CW, 0, 0, 0, 0, 0}; /* 加减速变量 */
__IO int32_t g_step_position = 0;                /* 当前位置 */
__IO uint8_t g_motion_sta = 0;                   /* 是否在运动？0：停止，1：运动 */
__IO uint32_t g_add_pulse_count = 0;             /* 脉冲个数累计 */
/*
 * @brief       生成梯形运动控制参数
 * @param       step：移动的步数 (正数为顺时针，负数为逆时针).
 * @param       accel  加速度,实际值为accel*0.1*rad/sec^2  10倍并且2个脉冲算一个完整的周期
 * @param       decel  减速度,实际值为decel*0.1*rad/sec^2
 * @param       speed  最大速度,实际值为speed*0.1*rad/sec
 * @retval      无
 */
void create_t_ctrl_param(int32_t step, uint32_t accel, uint32_t decel, uint32_t speed)
{
    __IO uint16_t tim_count; /* 达到最大速度时的步数*/
    __IO uint32_t max_s_lim; /* 必须要开始减速的步数（如果加速没有达到最大速度）*/
    __IO uint32_t accel_lim;
    if (g_motion_sta != STOP) /* 只允许步进电机在停止的时候才继续*/
        return;
    if (step < 0) /* 步数为负数 */
    {
        g_srd.dir = CCW; /* 逆时针方向旋转 */
        Set_DIR(CCW);
        step = -step; /* 获取步数绝对值 */
    }
    else
    {
        g_srd.dir = CW; /* 顺时针方向旋转 */
        Set_DIR(CW);
    }

    if (step == 1) /* 步数为1 */
    {
        g_srd.accel_count = -1;  /* 只移动一步 */
        g_srd.run_state = DECEL; /* 减速状态. */
        g_srd.step_delay = 1000; /* 默认速度 */
    }
    else if (step != 0) /* 如果目标运动步数不为0*/
    {
        /*设置最大速度极限, 计算得到min_delay用于定时器的计数器的值 min_delay = (alpha / t)/ w*/
        g_srd.min_delay = (int32_t)(A_T_x10 / speed); // 匀速运行时的计数值

        /* 通过计算第一个(c0) 的步进延时来设定加速度，其中accel单位为0.1rad/sec^2
         step_delay = 1/tt * sqrt(2*alpha/accel)
         step_delay = ( tfreq*0.69/10 )*10 * sqrt( (2*alpha*100000) / (accel*10) )/100 */

        g_srd.step_delay = (int32_t)((T1_FREQ_148 * sqrt(A_SQ / accel)) / 10); /* c0 */

        max_s_lim = (uint32_t)(speed * speed / (A_x200 * accel / 10)); /* 计算多少步之后达到最大速度的限制 max_s_lim = speed^2 / (2*alpha*accel) */

        if (max_s_lim == 0) /* 如果达到最大速度小于0.5步，我们将四舍五入为0,但实际我们必须移动至少一步才能达到想要的速度 */
        {
            max_s_lim = 1;
        }
        accel_lim = (uint32_t)(step * decel / (accel + decel)); /* 这里不限制最大速度 计算多少步之后我们必须开始减速 n1 = (n1+n2)decel / (accel + decel) */

        if (accel_lim == 0) /* 不足一步 按一步处理*/
        {
            accel_lim = 1;
        }
        if (accel_lim <= max_s_lim) /* 加速阶段到不了最大速度就得减速。。。使用限制条件我们可以计算出减速阶段步数 */
        {
            g_srd.decel_val = accel_lim - step; /* 减速段的步数 */
        }
        else
        {
            g_srd.decel_val = -(max_s_lim * accel / decel); /* 减速段的步数 */
        }
        if (g_srd.decel_val == 0) /* 不足一步 按一步处理 */
        {
            g_srd.decel_val = -1;
        }
        g_srd.decel_start = step + g_srd.decel_val; /* 计算开始减速时的步数 */

        if (g_srd.step_delay <= g_srd.min_delay) /* 如果一开始c0的速度比匀速段速度还大，就不需要进行加速运动，直接进入匀速 */
        {
            g_srd.step_delay = g_srd.min_delay;
            g_srd.run_state = RUN;
        }
        else
        {
            g_srd.run_state = ACCEL;
        }
        g_srd.accel_count = 0; /* 复位加减速计数值 */
    }
    g_motion_sta = 1;                 /* 电机为运动状态 */
    TIM_CtrlPWMOutputs(TIM1, ENABLE); // 开启PWM输出
    tim_count = TIM_GetCounter(TIM1);
    TIM_SetCompare4(TIM1, tim_count + g_srd.step_delay / 2); // 设置CH3通道比较值
    TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);                  // 使能捕获比较3中断                                     /* 使能定时器通道 */
}

void TIM1_CC_IRQHandler(void)
{
    __IO uint32_t tim_count = 0;
    uint16_t new_step_delay = 0;               /* 保存新（下）一个延时周期 */
    __IO static uint16_t last_accel_delay = 0; /* 加速过程中最后一次延时（脉冲周期） */
    __IO static uint32_t step_count = 0;       /* 总移动步数计数器*/
    __IO static int32_t rest = 0;              /* 记录new_step_delay中的余数，提高下一步计算的精度 */
    __IO static uint8_t i = 0;                 /* 定时器使用翻转模式，需要进入两次中断才输出一个完整脉冲 */

    if (TIM_GetITStatus(TIM1, TIM_IT_CC4) != RESET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC4);

        tim_count = TIM_GetCounter(TIM1);
        TIM_SetCompare4(TIM1, tim_count + g_srd.step_delay / 2);

        i++;        /* 定时器中断次数计数值 */
        if (i == 2) /* 2次，说明已经输出一个完整脉冲 */
        {
            i = 0;                   /* 清零定时器中断次数计数值 */
            switch (g_srd.run_state) /* 加减速曲线阶段 */
            {
            case STOP:
                step_count = 0; /* 清零步数计数器 */
                rest = 0;       /* 清零余值 */
                /* 关闭通道*/
                TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE); // 关闭通道中断
                TIM_CtrlPWMOutputs(TIM1, DISABLE);       // 关闭PWM输出
                g_motion_sta = 0;                        /* 电机为停止状态  */
                break;

            case ACCEL:
                g_add_pulse_count++; /* 只用于记录相对位置转动了多少度 */
                step_count++;        /* 步数加1*/
                /* 更新绝对位置 */
                g_step_position += (g_srd.dir == CW) ? 1 : -1;
                g_srd.accel_count++;                                                                                 /* 加速计数值加1*/
                new_step_delay = g_srd.step_delay - (((2 * g_srd.step_delay) + rest) / (4 * g_srd.accel_count + 1)); /* 计算新(下)一步脉冲周期(时间间隔) */
                rest = ((2 * g_srd.step_delay) + rest) % (4 * g_srd.accel_count + 1);                                /* 计算余数，下次计算补上余数，减少误差 */
                if (step_count >= g_srd.decel_start)                                                                 /* 检查是否到了需要减速的步数 */
                {
                    g_srd.accel_count = g_srd.decel_val; /* 加速计数值为减速阶段计数值的初始值 */
                    g_srd.run_state = DECEL;             /* 下个脉冲进入减速阶段 */
                }
                else if (new_step_delay <= g_srd.min_delay) /* 检查是否到达期望的最大速度 计数值越小速度越快，当你的速度和最大速度相等或更快就进入匀速*/
                {
                    last_accel_delay = new_step_delay; /* 保存加速过程中最后一次延时（脉冲周期）*/
                    new_step_delay = g_srd.min_delay;  /* 使用min_delay（对应最大速度speed）*/
                    rest = 0;                          /* 清零余值 */
                    g_srd.run_state = RUN;             /* 设置为匀速运行状态 */
                }
                break;

            case RUN:
                g_add_pulse_count++;
                step_count++; /* 步数加1 */
                /* 更新绝对位置 */
                g_step_position += (g_srd.dir == CW) ? 1 : -1;

                new_step_delay = g_srd.min_delay;    /* 使用min_delay（对应最大速度speed）*/
                if (step_count >= g_srd.decel_start) /* 需要开始减速 */
                {
                    g_srd.accel_count = g_srd.decel_val; /* 减速步数做为加速计数值 */
                    new_step_delay = last_accel_delay;   /* 加阶段最后的延时做为减速阶段的起始延时(脉冲周期) */
                    g_srd.run_state = DECEL;             /* 状态改变为减速 */
                }
                break;

            case DECEL:
                step_count++; /* 步数加1 */
                g_add_pulse_count++;
                /* 更新绝对位置 */
                g_step_position += (g_srd.dir == CW) ? 1 : -1;
                g_srd.accel_count++;
                new_step_delay = g_srd.step_delay - (((2 * g_srd.step_delay) + rest) / (4 * g_srd.accel_count + 1)); /* 计算新(下)一步脉冲周期(时间间隔) */
                rest = ((2 * g_srd.step_delay) + rest) % (4 * g_srd.accel_count + 1);                                /* 计算余数，下次计算补上余数，减少误差 */

                /* 检查是否为最后一步 */
                if (g_srd.accel_count >= 0) /* 判断减速步数是否从负值加到0是的话 减速完成 */
                {
                    g_srd.run_state = STOP;
                }
                break;
            }
            g_srd.step_delay = new_step_delay; /* 为下个(新的)延时(脉冲周期)赋值 */
        }
    }
}

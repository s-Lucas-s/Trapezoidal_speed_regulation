#ifndef __SYS_H
#define __SYS_H

typedef enum {
    STM_ture  = 1,
    STM_false = 0
} STM_err_t;

#include "stm32f10x.h"
#include "Control.h"
#include "Delay.h"
#include "OLED.h"
#include "math.h"

void NVIC_Set(void);

#endif

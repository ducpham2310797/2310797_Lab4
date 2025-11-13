/*
 * Tasks.c
 *
 *  Created on: Oct 30, 2025
 *      Author: my pc
 */
#include "Tasks.h"


void Task_LED1(void) {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
}

void Task_LED2(void) {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
}

void Task_LED3(void) {
    HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}

void Task_LED4(void) {
    HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
}

void Task_LED5(void) {
    HAL_GPIO_TogglePin(LED5_GPIO_Port, LED5_Pin);
}



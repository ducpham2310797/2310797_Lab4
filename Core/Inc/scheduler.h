#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stdint.h>

/* Error codes */
#define ERROR_SCH_TOO_MANY_TASKS            1
#define ERROR_SCH_CANNOT_DELETE_TASK        2
#define ERROR_SCH_TASK_NOT_FOUND            3
#define NO_TASK_ID                          0

/* Core scheduler functions */
void SCH_Init(void);
void SCH_Update(void);
void SCH_Dispatch_Tasks(void);
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD);
uint8_t SCH_Delete_Task(uint32_t taskID);

/* Utility functions */
uint32_t SCH_Get_Current_Time(void);
uint8_t SCH_Get_Error_Code(void);

#endif // __SCHEDULER_H

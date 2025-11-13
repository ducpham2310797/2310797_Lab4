#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

// Maximum number of tasks
#define SCH_MAX_TASKS 40
#define NO_TASK_ID 0

// Error codes
#define ERROR_SCH_TOO_MANY_TASKS            1
#define ERROR_SCH_CANNOT_DELETE_TASK        2
#define ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK  3
#define ERROR_SCH_WAITING_FOR_START_COMMAND 4
#define ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START 5
#define ERROR_SCH_LOST_SLAVE                6
#define ERROR_SCH_CAN_BUS_ERROR             7

// Return codes
#define RETURN_ERROR   0
#define RETURN_NORMAL  1

// Task structure
typedef struct {
    void (*pTask)(void);      // Pointer to the task function
    uint32_t Delay;           // Delay (ticks) until function will run
    uint32_t Period;          // Interval (ticks) between subsequent runs
    uint8_t RunMe;            // Flag indicating task is due to execute
    uint32_t TaskID;          // Unique task identifier
} sTask;

// Function prototypes
void SCH_Init(void);
void SCH_Update(void);
void SCH_Dispatch_Tasks(void);
uint32_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD);
uint8_t SCH_Delete_Task(uint32_t taskID);
void SCH_Report_Status(void);
void SCH_Go_To_Sleep(void);

// Global error code
extern uint8_t Error_code_G;

#endif /* SCHEDULER_H_ */

#include "scheduler.h"
#include <stddef.h>

// Task array
sTask SCH_tasks_G[SCH_MAX_TASKS];

// Error code variable
uint8_t Error_code_G = 0;

// Last error code for reporting
static uint8_t Last_error_code_G = 0;
static uint32_t Error_tick_count_G = 0;

// Task ID counter
static uint32_t Task_ID_Counter = 1;

/**
 * @brief Initialize the scheduler
 */
void SCH_Init(void) {
    uint8_t i;

    // Clear all tasks
    for (i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_Delete_Task(i);
    }

    // Reset error code
    Error_code_G = 0;
    Last_error_code_G = 0;
    Error_tick_count_G = 0;

    // Reset task ID counter
    Task_ID_Counter = 1;
}

/**
 * @brief Update function - called from timer ISR
 */
void SCH_Update(void) {
    uint8_t Index;

    // Check all tasks
    for (Index = 0; Index < SCH_MAX_TASKS; Index++) {
        // Check if there is a task at this location
        if (SCH_tasks_G[Index].pTask != NULL) {
            if (SCH_tasks_G[Index].Delay == 0) {
                // Task is due to run
                SCH_tasks_G[Index].RunMe += 1;

                if (SCH_tasks_G[Index].Period != 0) {
                    // Schedule periodic tasks to run again
                    SCH_tasks_G[Index].Delay = SCH_tasks_G[Index].Period;
                }
            } else {
                // Not yet ready to run - decrement delay
                SCH_tasks_G[Index].Delay -= 1;
            }
        }
    }
}

/**
 * @brief Dispatch tasks - runs tasks that are due
 */
void SCH_Dispatch_Tasks(void) {
    uint8_t Index;

    // Dispatches (runs) the next task (if one is ready)
    for (Index = 0; Index < SCH_MAX_TASKS; Index++) {
        if (SCH_tasks_G[Index].RunMe > 0 && SCH_tasks_G[Index].pTask != NULL) {
            // Run the task
            (*SCH_tasks_G[Index].pTask)();

            // Reset/reduce RunMe flag
            SCH_tasks_G[Index].RunMe -= 1;

            // If this is a one-shot task, remove it
            if (SCH_tasks_G[Index].Period == 0) {
                SCH_Delete_Task(SCH_tasks_G[Index].TaskID);
            }
        }
    }

    // Report system status
    SCH_Report_Status();

    // Enter idle mode (optional)
    SCH_Go_To_Sleep();
}

/**
 * @brief Add a task to the scheduler
 * @param pFunction Pointer to the task function
 * @param DELAY Initial delay before first execution (in ticks)
 * @param PERIOD Interval between subsequent executions (in ticks)
 * @return Task ID or SCH_MAX_TASKS if error
 */
uint32_t SCH_Add_Task(void (*pFunction)(), uint32_t DELAY, uint32_t PERIOD) {
    uint8_t Index = 0;

    // Find a gap in the array
    while ((SCH_tasks_G[Index].pTask != NULL) && (Index < SCH_MAX_TASKS)) {
        Index++;
    }

    // Have we reached the end of the list?
    if (Index == SCH_MAX_TASKS) {
        // Task list is full
        Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
        return NO_TASK_ID;
    }

    // Add task to the array
    SCH_tasks_G[Index].pTask = pFunction;
    SCH_tasks_G[Index].Delay = DELAY;
    SCH_tasks_G[Index].Period = PERIOD;
    SCH_tasks_G[Index].RunMe = 0;
    SCH_tasks_G[Index].TaskID = Task_ID_Counter++;

    // Return task ID
    return SCH_tasks_G[Index].TaskID;
}

/**
 * @brief Delete a task from the scheduler
 * @param taskID The ID of the task to delete
 * @return RETURN_NORMAL if successful, RETURN_ERROR otherwise
 */
uint8_t SCH_Delete_Task(uint32_t taskID) {
    uint8_t Index;
    uint8_t Return_code = RETURN_ERROR;

    // Find the task with matching ID
    for (Index = 0; Index < SCH_MAX_TASKS; Index++) {
        if (SCH_tasks_G[Index].TaskID == taskID) {
            // Clear the task
            SCH_tasks_G[Index].pTask = NULL;
            SCH_tasks_G[Index].Delay = 0;
            SCH_tasks_G[Index].Period = 0;
            SCH_tasks_G[Index].RunMe = 0;
            SCH_tasks_G[Index].TaskID = 0;

            Return_code = RETURN_NORMAL;
            break;
        }
    }

    if (Return_code == RETURN_ERROR) {
        Error_code_G = ERROR_SCH_CANNOT_DELETE_TASK;
    }

    return Return_code;
}

/**
 * @brief Report scheduler status (error codes)
 */
void SCH_Report_Status(void) {
    // Check for a new error code
    if (Error_code_G != Last_error_code_G) {
        Last_error_code_G = Error_code_G;

        if (Error_code_G != 0) {
            Error_tick_count_G = 60000; // Display error for 60000 ticks
        } else {
            Error_tick_count_G = 0;
        }
    } else {
        if (Error_tick_count_G != 0) {
            if (--Error_tick_count_G == 0) {
                Error_code_G = 0; // Reset error code
            }
        }
    }
}

/**
 * @brief Put system to sleep (optional power saving)
 */
void SCH_Go_To_Sleep(void) {
    // Optional: Implement sleep mode
    // __WFI(); // Wait for interrupt
}

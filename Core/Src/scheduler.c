#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>

/*----------------------------------------------------------------------------
 * Task Node Structure - Sorted Linked List
 *---------------------------------------------------------------------------*/
typedef struct TaskNode {
    void (*pTask)(void);           // Function pointer to task
    uint32_t Delay;                 // Delta delay to next execution
    uint32_t Period;                // Repeat interval (0 = one-shot)
    uint32_t TaskID;                // Unique identifier
    struct TaskNode* next;          // Next task in sorted list
} TaskNode;

/*----------------------------------------------------------------------------
 * Global Variables
 *---------------------------------------------------------------------------*/
static TaskNode* g_TaskListHead = NULL;  // Head of sorted task list
static uint32_t g_CurrentTick = 0;        // System tick counter (10ms each)
static uint32_t g_NextTaskID = 1;         // Auto-increment task ID
static uint8_t g_ErrorCode = 0;           // Error code register

/*----------------------------------------------------------------------------
 * SCH_Init() - Initialize the scheduler
 * - Clears all tasks
 * - Resets tick counter
 * - Prepares for operation
 *---------------------------------------------------------------------------*/
void SCH_Init(void) {
    // Clear all existing tasks
    while (g_TaskListHead != NULL) {
        TaskNode* temp = g_TaskListHead;
        g_TaskListHead = g_TaskListHead->next;
        free(temp);
    }

    g_TaskListHead = NULL;
    g_CurrentTick = 0;
    g_NextTaskID = 1;
    g_ErrorCode = 0;
}

/*----------------------------------------------------------------------------
 * SCH_Update() - CRITICAL: Must be called from Timer ISR every 10ms
 *
 * Complexity: O(1) - Only updates head of sorted list!
 *
 * This is the KEY optimization that satisfies the requirement:
 * "O(n) searches in the SCH_Update function" is considered unsatisfactory.
 *
 * Called from: HAL_TIM_PeriodElapsedCallback()
 *---------------------------------------------------------------------------*/
void SCH_Update(void) {
    g_CurrentTick++;

    // Only decrement the head task's delay (O(1) operation!)
    // Because list is sorted, only the first task needs checking
    if (g_TaskListHead != NULL && g_TaskListHead->Delay > 0) {
        g_TaskListHead->Delay--;
    }
}

/*----------------------------------------------------------------------------
 * SCH_Add_Task() - Add task to sorted list by insertion
 *
 * Parameters:
 *   pFunction - Pointer to task function (void function(void))
 *   DELAY     - Initial delay in ticks (1 tick = 10ms)
 *   PERIOD    - Repeat period in ticks (0 = one-shot task)
 *
 * Returns: Task ID (> 0) on success, 0 on failure
 *
 * Example:
 *   SCH_Add_Task(Task_LED1, 0, 50);    // Run every 500ms, start immediately
 *   SCH_Add_Task(Task_LED2, 100, 100); // Run every 1s, start after 1s
 *---------------------------------------------------------------------------*/
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD) {
    if (pFunction == NULL) {
        g_ErrorCode = ERROR_SCH_TOO_MANY_TASKS;
        return NO_TASK_ID;
    }

    // Allocate new task node
    TaskNode* newTask = (TaskNode*)malloc(sizeof(TaskNode));
    if (newTask == NULL) {
        g_ErrorCode = ERROR_SCH_TOO_MANY_TASKS;
        return NO_TASK_ID;
    }

    // Initialize task data
    newTask->pTask = pFunction;
    newTask->Period = PERIOD;
    newTask->TaskID = g_NextTaskID++;
    newTask->next = NULL;

    /*
     * Insert in sorted order using DELTA TIME technique
     *
     * Example: Tasks at 10ms, 25ms, 30ms stored as:
     * Head -> [10] -> [15] -> [5] -> NULL
     *         ^^^     ^^^     ^^^
     *       10-0    25-10   30-25
     */

    if (g_TaskListHead == NULL || DELAY < g_TaskListHead->Delay) {
        // Insert at head
        newTask->Delay = DELAY;
        if (g_TaskListHead != NULL) {
            g_TaskListHead->Delay -= DELAY;
        }
        newTask->next = g_TaskListHead;
        g_TaskListHead = newTask;
    } else {
        // Find insertion point
        TaskNode* current = g_TaskListHead;
        uint32_t accumulatedTime = g_TaskListHead->Delay;

        while (current->next != NULL &&
               accumulatedTime + current->next->Delay <= DELAY) {
            accumulatedTime += current->next->Delay;
            current = current->next;
        }

        // Insert after current
        newTask->Delay = DELAY - accumulatedTime;
        newTask->next = current->next;

        if (current->next != NULL) {
            current->next->Delay -= newTask->Delay;
        }

        current->next = newTask;
    }

    return newTask->TaskID;
}

/*----------------------------------------------------------------------------
 * SCH_Dispatch_Tasks() - Execute all tasks that are ready
 *
 * Must be called from main loop:
 *   while(1) {
 *       SCH_Dispatch_Tasks();
 *   }
 *
 * Complexity: O(k) where k = number of ready tasks
 *---------------------------------------------------------------------------*/
void SCH_Dispatch_Tasks(void) {
    // Process all tasks with Delay == 0
    while (g_TaskListHead != NULL && g_TaskListHead->Delay == 0) {
        TaskNode* taskToRun = g_TaskListHead;

        // Remove from head
        g_TaskListHead = g_TaskListHead->next;

        // Execute the task
        if (taskToRun->pTask != NULL) {
            (*taskToRun->pTask)();
        }

        // Handle periodic tasks
        if (taskToRun->Period > 0) {
            // Reschedule periodic task
            void (*savedFunc)(void) = taskToRun->pTask;
            uint32_t savedPeriod = taskToRun->Period;
            uint32_t savedID = taskToRun->TaskID;

            free(taskToRun);

            // Re-add with same ID and period
            TaskNode* rescheduled = (TaskNode*)malloc(sizeof(TaskNode));
            if (rescheduled != NULL) {
                rescheduled->pTask = savedFunc;
                rescheduled->Period = savedPeriod;
                rescheduled->TaskID = savedID;
                rescheduled->next = NULL;

                // Re-insert in sorted order
                if (g_TaskListHead == NULL || savedPeriod < g_TaskListHead->Delay) {
                    rescheduled->Delay = savedPeriod;
                    if (g_TaskListHead != NULL) {
                        g_TaskListHead->Delay -= savedPeriod;
                    }
                    rescheduled->next = g_TaskListHead;
                    g_TaskListHead = rescheduled;
                } else {
                    TaskNode* current = g_TaskListHead;
                    uint32_t accum = g_TaskListHead->Delay;

                    while (current->next != NULL &&
                           accum + current->next->Delay <= savedPeriod) {
                        accum += current->next->Delay;
                        current = current->next;
                    }

                    rescheduled->Delay = savedPeriod - accum;
                    rescheduled->next = current->next;

                    if (current->next != NULL) {
                        current->next->Delay -= rescheduled->Delay;
                    }

                    current->next = rescheduled;
                }
            }
        } else {
            // One-shot task, just free it
            free(taskToRun);
        }
    }
}

/*----------------------------------------------------------------------------
 * SCH_Delete_Task() - Remove task by ID
 *
 * Parameters:
 *   taskID - ID returned by SCH_Add_Task()
 *
 * Returns: 1 on success, 0 on failure
 *---------------------------------------------------------------------------*/
uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (g_TaskListHead == NULL) {
        g_ErrorCode = ERROR_SCH_CANNOT_DELETE_TASK;
        return 0;
    }

    TaskNode* current = g_TaskListHead;
    TaskNode* previous = NULL;

    while (current != NULL) {
        if (current->TaskID == taskID) {
            // Found the task to delete
            if (previous == NULL) {
                // Deleting head
                g_TaskListHead = current->next;
                if (g_TaskListHead != NULL) {
                    g_TaskListHead->Delay += current->Delay;
                }
            } else {
                // Deleting middle or end
                previous->next = current->next;
                if (current->next != NULL) {
                    current->next->Delay += current->Delay;
                }
            }

            free(current);
            return 1;
        }

        previous = current;
        current = current->next;
    }

    g_ErrorCode = ERROR_SCH_TASK_NOT_FOUND;
    return 0;
}

/*----------------------------------------------------------------------------
 * SCH_Get_Current_Time() - Get current time in milliseconds
 *
 * Returns: Time in ms (tick * 10ms)
 *---------------------------------------------------------------------------*/
uint32_t SCH_Get_Current_Time(void) {
    return g_CurrentTick * 10;  // Convert ticks to milliseconds
}

/*----------------------------------------------------------------------------
 * SCH_Get_Error_Code() - Get and clear error code
 *---------------------------------------------------------------------------*/
uint8_t SCH_Get_Error_Code(void) {
    uint8_t error = g_ErrorCode;
    g_ErrorCode = 0;
    return error;
}

#include "OS_Core.h"

/**
 * @brief Structure used to control the tasks execution.
 * Is private to OS_Core.c so is can't be manipulate from other files.
 */
typedef struct{
    u32 osLastError;                        // Last error
    u32 osSystemStatus;                     // System status (Reset, Running, IRQ)
    u32 osScheduleExec;                     // Execution flag
    OsTaskCtrl* osCurrTaskCallback;         // Current task executing
    OsTaskCtrl* osNextTaskCallback;         // Next task to be executed
    OsTaskCtrl* osTaskList[OS_MAX_TASKS];   // List of tasks 
}OsKernelCtrl;

OsKernelCtrl OsKernel;

retType OsTaskCreate(const char* taskName, void* taskFunction, OsTaskCtrl* taskCtrlStruct)
{
    static u8 taskCount = 0;

    /* Check that taskFunction and taskCtrlStruct is not NULL */
    if (NULL == taskFunction || NULL == taskCtrlStruct || taskCount == OS_MAX_TASKS)
    {
        return API_OS_ERROR;
    }

    /* If this is the first call, set osTaskList to NULL on each place */
    if (taskCount == 0){
        for (u8 i=0; i<OS_MAX_TASKS; i++)
        {
            OsKernel.osTaskList[i] = NULL;
        }
    }
    /* If the taskList is full return Error */
    else if (taskCount == (OS_MAX_TASKS - 1)) 
    {
        return API_OS_ERROR;
    }
    
    /*
                   STACK FRAME
        ---------------------------------
        |             xPSR              | <= 1<<24 
        ---------------------------------
        |              PC               | <= Entry point (taskFunction address)
        ---------------------------------
        |              LR               | <= EXEC_RETURN_VALUE
        ---------------------------------
        |              R12              |
        ---------------------------------
        |              R3               |
        ---------------------------------
        |              R2               |
        ---------------------------------
        |              R1               |
        ---------------------------------
        |              R0               | <= MSP
        ---------------------------------
        |            LR IRQ             |
        ---------------------------------
    */

    /*  
        Arrange the STACK Frame for the first time:
        1) Set a 1 on bit 24 of xPSR to make sure we are executing THUMB instructions.
        2) PC must have the entry point (taskFunction in this case).
        3) Set the link register to EXEC_RETURN_VALUE to trigger.
        4) Set the stack pointer
    */

    taskCtrlStruct->taskMemory[OS_MAX_STACK_SIZE/4 - XPSR_REG_POSITION]     = XPSR_VALUE;
    taskCtrlStruct->taskMemory[OS_MAX_STACK_SIZE/4 - PC_REG_POSTION]        = (uint32_t)taskFunction;
    taskCtrlStruct->taskMemory[OS_MAX_STACK_SIZE/4 - LR_PREV_VALUE_POSTION] = EXEC_RETURN_VALUE;

    taskCtrlStruct->taskStackPointer = (uint32_t)(taskCtrlStruct->taskMemory + OS_MAX_STACK_SIZE/4 - OS_STACK_FRAME_SIZE);
    taskCtrlStruct->taskEntryPoint = taskFunction;                                      // Assign the function to the Entry Point
    taskCtrlStruct->taskName = taskName;                                                // Assing the taskName
    taskCtrlStruct->taskExecStatus = OS_TASK_READY;                                     // Set the task to Ready
    taskCtrlStruct->taskPriority = PRIORITY_LEVEL_1;                                    // Set the priority level to 1 (Not in used now)

    OsKernel.osTaskList[taskCount] = taskCtrlStruct;                                    // Add the task structure to the list of tasks
    taskCount++;                                                                        // Increment the task counter
    taskCtrlStruct->taskID = taskCount;                                                 // Assing task ID starting from 1

    return (retType)API_OK;
}

retType OsStartScheduler(void)
{

    return (retType)API_OK;
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{

}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{

}
#include "OS_Core.h"

/**
 * @brief Structure used to control the tasks execution.
 * Is private to OS_Core.c so is can't be manipulate from other files.
 */
typedef struct{
    u32 osLastError;                        // Last error
    OsStatus osSystemStatus;                // System status (Reset, Running, IRQ)
    u32 osScheduleExec;                     // Execution flag
    osTaskObject* osCurrTaskCallback;         // Current task executing
    osTaskObject* osNextTaskCallback;         // Next task to be executed
    osTaskObject* osTaskList[OS_MAX_TASKS];   // List of tasks
}OsKernelCtrl;

static OsKernelCtrl OsKernel;               // Create an instance of the Kernel Control Structure

/* Private functions declarations */
static void scheduler(void);
static u32 getNextContext(u32 currentStaskPointer);

retType osTaskCreate(osTaskObject* taskCtrlStruct, void* taskFunction )
{
    static u8 taskCount = 0;

    /* Check that taskFunction and taskCtrlStruct is not NULL */
    if (NULL == taskFunction || NULL == taskCtrlStruct)
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
    */
    taskCtrlStruct->taskMemory[OS_MAX_STACK_SIZE/4 - XPSR_REG_POSITION]     = XPSR_VALUE;
    taskCtrlStruct->taskMemory[OS_MAX_STACK_SIZE/4 - PC_REG_POSTION]        = (u32)taskFunction;
    taskCtrlStruct->taskMemory[OS_MAX_STACK_SIZE/4 - LR_PREV_VALUE_POSTION] = EXEC_RETURN_VALUE;

    /* 				taskStackPointer =           (Address of first element)  +        64           -         17          =  */
    taskCtrlStruct->taskStackPointer = (u32)(taskCtrlStruct->taskMemory + OS_MAX_STACK_SIZE/4 - OS_STACK_FRAME_SIZE);

    taskCtrlStruct->taskEntryPoint = taskFunction;                                      // Assign the function to the Entry Point
//    taskCtrlStruct->taskName = taskName;                                                // Assing the taskName
    taskCtrlStruct->taskExecStatus = OS_TASK_READY;                                     // Set the task to Ready
    taskCtrlStruct->taskPriority = PRIORITY_LEVEL_1;                                    // Set the priority level to 1 (Not in used now)

    OsKernel.osTaskList[taskCount] = taskCtrlStruct;                                    // Add the task structure to the list of tasks
    taskCount++;                                                                        // Increment the task counter
    taskCtrlStruct->taskID = taskCount;                                                 // Assing task ID starting from 1

    return (retType)API_OK;
}

retType osStart(void)
{
    /* Disable Systick and PendSV interrupts */
    NVIC_DisableIRQ(SysTick_IRQn);
    NVIC_DisableIRQ(PendSV_IRQn);

    OsKernel.osSystemStatus = OS_STATUS_STOPPED;       // Set the System to STOPPED for the first time.
    OsKernel.osCurrTaskCallback = NULL;      // Set the Current task to NULL the first time. This will be handled by the scheduler
    OsKernel.osNextTaskCallback = NULL;      // Set the Next task to NULL the first time. This will be handled by the scheduler

    /* Is mandatory to set the PendSV priority as lowest as possible */
    NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS)-1);

    /* Activate and config the Systick exception */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / OS_SYSTICK_TICK);

    /* Enable Systick and PendSV interrupts */
    NVIC_EnableIRQ(PendSV_IRQn);
    NVIC_EnableIRQ(SysTick_IRQn);

    return (retType)API_OK;
}

static u32 getNextContext(u32 currentStaskPointer)
{
     // Is the first time execute operating system? Yes, so will do task charged on next task.
    if (OsKernel.osSystemStatus != OS_STATUS_RUNNING)
    {
        OsKernel.osCurrTaskCallback->taskExecStatus = OS_TASK_RUNNING;
        OsKernel.osSystemStatus = OS_STATUS_RUNNING;
        return OsKernel.osCurrTaskCallback->taskStackPointer;
    }

    // Storage last stack pointer used on current task and change state to ready.
    OsKernel.osCurrTaskCallback->taskStackPointer = currentStaskPointer;
    OsKernel.osCurrTaskCallback->taskExecStatus = OS_TASK_READY;

    // Switch address memory points on current task for next task and change state of task
    OsKernel.osCurrTaskCallback = OsKernel.osNextTaskCallback;
    OsKernel.osCurrTaskCallback->taskExecStatus = OS_TASK_RUNNING;

    return OsKernel.osCurrTaskCallback->taskStackPointer;
}

/**
 * @brief This function will be executed in a exception context
 * It will do the context change for the tasks
 */
static void scheduler(void)
{
    static u8 osTaskIndex = 0;

    /* First we need to check if the kernel is running */
    if (OsKernel.osSystemStatus != OS_STATUS_RUNNING)
    {
        OsKernel.osCurrTaskCallback = OsKernel.osTaskList[0];
        osTaskIndex++;
        return;
    }

    /* If the next index in osTaskList is NULL then we did a full round table.
     * So we need to go back to the first task in the list
     */
    if (NULL != OsKernel.osTaskList[osTaskIndex] && osTaskIndex < (OS_MAX_TASKS))
    {   
        OsKernel.osNextTaskCallback = OsKernel.osTaskList[osTaskIndex];
        osTaskIndex++;
    }
    else
    {   
        /* If we are in this point is because we already did a round table */
        OsKernel.osNextTaskCallback = OsKernel.osTaskList[0];
        osTaskIndex = 0;
    }
}

/**
  * @brief This function handles Pendable request for system service.
  */
__attribute__ ((naked)) void PendSV_Handler(void)
{

    /*   When PendSV is called the sequence is the following */
    
    __ASM volatile ("push {r4-r11, lr}");
    /*
                   STACK FRAME
        ---------------------------------
     1  |             xPSR              | <= 1<<24 
        ---------------------------------
     2  |              PC               | <= Entry point (taskFunction address)
        ---------------------------------
     3  |              LR               | <= EXEC_RETURN_VALUE
        ---------------------------------
     4  |              R12              |
        ---------------------------------
     :  |                               | <= R4 - R11
        ---------------------------------
     5  |              R3               |
        ---------------------------------
     6  |              R2               |
        ---------------------------------
     7  |              R1               |
        ---------------------------------
     8  |              R0               |
        ---------------------------------
     9  |            LR IRQ             | 
        ---------------------------------
    */

    __ASM volatile ("mrs r0, msp");

    /*
                   STACK FRAME
        ---------------------------------
     1  |             xPSR              | <= 1<<24 
        ---------------------------------
     2  |              PC               | <= Entry point (taskFunction address)
        ---------------------------------
     3  |              LR               | <= EXEC_RETURN_VALUE
        ---------------------------------
     4  |              R12              |
        ---------------------------------
     :  |                               | <= R4 - R11
        ---------------------------------
     5  |              R3               |
        ---------------------------------
     6  |              R2               |
        ---------------------------------
     7  |              R1               |
        ---------------------------------
     8  |              R0               | <= MSP
        ---------------------------------
     9  |            LR IRQ             | 
        ---------------------------------
    */

    __ASM volatile ("bl %0" :: "i"(getNextContext));
    __ASM volatile ("msr msp, r0");
    __ASM volatile ("pop {r4-r11, lr}");    // Recuperados todos los valores de registros
    __ASM volatile ("bx lr");               // Se hace un branch indirect con el valor de LR que es nuevamente EXEC_RETURN

}

/**
  * @brief This function handles System tick timer.
  * This will be executed every OS_SYSTICK_TICK time.
  */
void SysTick_Handler(void)
{
    scheduler();

    /*  We need to manipulate the ICSR register (Interrupt Control and State Register)
     *  Bit 28 is the PENSVSET mask
     *  If we WRITE a 0 = Pending exception is not pending
     *  If we WRITE a 1 = Pending exception is pending
     *  So this works as a trigger for the PendSV (Pending Service Call) that we need for a context change
     */
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;


    /*
     * Instruction Synchronization Barrier; flushes the pipeline and ensures that
     * all previous instructions are completed before executing new instructions
     */
    __ISB();

    /*
     * Data Synchronization Barrier; ensures that all memory accesses are
     * completed before next instruction is executed
     */
    __DSB();

}

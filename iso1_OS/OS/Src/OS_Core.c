#include "OS_Core.h"

// #define OS_SIMPLE
#define OS_WITH_PRIORITY

osTaskObject idle;
u8 osTasksCreated = 0;

/**
 * @brief Structure used to control the tasks execution.
 * Is private to OS_Core.c so is can't be manipulate from other files.
 */
typedef struct{
    u32 osLastError;                        		// Last error
    OsStatus osSystemStatus;                		// System status (Reset, Running, IRQ)
    u32 osScheduleExec;                     		// Execution flag
    osTaskObject* osCurrTaskCallback;         		// Current task executing
    osTaskObject* osNextTaskCallback;         		// Next task to be executed
    osTaskObject* osTaskList[OS_MAX_TASKS ];   		// List of tasks
	osTaskObject* osTaskPriorityList[OS_MAX_TASKS];	// Task priorities
}OsKernelCtrl;

static OsKernelCtrl OsKernel;               // Create an instance of the Kernel Control Structure

/* Private functions declarations */
static void scheduler(void);
static u32 getNextContext(u32 currentStaskPointer);
WEAK void osIdleTask(void);
void taskSortByPriority(u8 n);

retType osTaskCreate(osTaskObject* taskCtrlStruct, void* taskFunction, OsTaskPriorityLevel priority)
{
    static u8 taskCount = 0;

    /* Check that taskFunction and taskCtrlStruct is not NULL */
    if (NULL == taskFunction || NULL == taskCtrlStruct)
    {
        return API_OS_ERROR;
    }

    /* If this is the first call, set osTaskList to NULL on each place */
    if (taskCount == 0){
        for (u8 i=0; i<OS_MAX_TASKS - 1; i++)
        {
            OsKernel.osTaskList[i] = NULL;
        }
    }
    /* If the taskList is full return Error. Added -2 because idle is task 8*/
    else if (taskCount == (OS_MAX_TASKS - 2 )) 
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
	//taskCtrlStruct->taskName = taskName;                                              // Assing the taskName
	taskCtrlStruct->taskExecStatus = OS_TASK_READY;                                     // Set the task to Ready
    taskCtrlStruct->taskPriority = priority;                                    		// Set the priority level to 1 (Not in used now)

    OsKernel.osTaskList[taskCount] = taskCtrlStruct;                                    // Add the task structure to the list of tasks
	taskCount++;                                                                        // Increment the task counter
    taskCtrlStruct->taskID = taskCount;                                                 // Assing task ID starting from 1

    return (retType)API_OK;
}

retType osStart(void)
{
	retType ret = API_OK;

#ifdef OS_WITH_PRIORITY
	// Count the number of tasks created
	for (u8 i = 0 ; i < OS_MAX_TASKS - 1 ; i++)
	{
		if ( NULL != OsKernel.osTaskList[i]) osTasksCreated++;
	}
	taskSortByPriority(osTasksCreated);
	ret = osTaskCreate(&idle, osIdleTask, PRIORITY_LEVEL_4);	 // Create IDLE task with the lowest priority
	if (ret != API_OK) return API_OS_ERROR;
#endif

    /* Disable Systick and PendSV interrupts */
    NVIC_DisableIRQ(SysTick_IRQn);
    NVIC_DisableIRQ(PendSV_IRQn);

    OsKernel.osSystemStatus = OS_STATUS_STOPPED;    // Set the System to STOPPED for the first time.
    OsKernel.osCurrTaskCallback = NULL;      		// Set the Current task to NULL the first time. This will be handled by the scheduler
    OsKernel.osNextTaskCallback = NULL;      		// Set the Next task to NULL the first time. This will be handled by the scheduler

    /* Is mandatory to set the PendSV priority as lowest as possible */
    NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS)-1);

    /* Activate and config the Systick exception */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / OS_SYSTICK_TICK);

    /* Enable Systick and PendSV interrupts */
    NVIC_EnableIRQ(PendSV_IRQn);
    NVIC_EnableIRQ(SysTick_IRQn);

    return ret;
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
	/* Start osTaskLastPriExec with 1 so it will start from the highest priority */
    static u8 osTaskIndex = 0;
	static u8 status[OS_MAX_TASKS];
	osTaskStatusType taskStatus;


    /* First we need to check if the kernel is running */
    if (OsKernel.osSystemStatus != OS_STATUS_RUNNING)
    {
        OsKernel.osCurrTaskCallback = OsKernel.osTaskList[0];
        // osTaskIndex++;
        return;
    }

#ifdef OS_SIMPLE
    /* If the next index in osTaskList is NULL then we did a full round table.
     * So we need to go back to the first task in the list
     * This implementation is for Round-Robin
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
#else
	u8 n = 0;

	/* Check if all the tasks are in Blocked state */
	for (u8 idx = 0; idx < osTasksCreated; idx++)
	{
		if (OsKernel.osTaskList[idx]->taskExecStatus == OS_TASK_BLOCKED)
		{
			n++;
		}
	}
	
	/* If all the tasks are blocked, assign the last task (IDLE) to the next task */
	if (n == osTasksCreated)
	{
		/* Assign the IDLE task to be executed */
		if (OsKernel.osCurrTaskCallback != OsKernel.osTaskList[osTasksCreated])
		{
			OsKernel.osNextTaskCallback = OsKernel.osTaskList[osTasksCreated];
		}
		return;
	}

	/* Iterate between all the tasks to determine which should be the next to be executed */
	for (u8 idx = 0; idx < osTasksCreated; idx++)
	{
		taskStatus = OsKernel.osTaskList[idx]->taskExecStatus;
		switch(taskStatus)
		{
			case OS_TASK_RUNNING:
			{
				if (osTaskIndex == osTasksCreated - 1) 
				{
					osTaskIndex = 0;
					OsKernel.osNextTaskCallback = OsKernel.osTaskList[osTaskIndex];
				}
			}
			break;
			case OS_TASK_SUSPENDED: break;
			case OS_TASK_READY:
			{
				if (idx > osTaskIndex)
				{
					/* This case means there is no blocked task at idx and we are ahead of the last task
					* executed.
					* [ Ready, Ready, osTaskIndex (Running), idx, ... , ...]
					*/

					/* Set the next task callback */
					OsKernel.osNextTaskCallback = OsKernel.osTaskList[idx];

					/* Save the last task executed
					* [ Ready, Ready, Ready, osTaskIndex (Running), ... , ...]
					*/
					osTaskIndex = idx;

					return;
				}
				else
				{
					/* 	This case means there was no blocked task with highest priority 
					*	[Ready, Ready, idx, osTaskIndex (Running), ... , ...]
					*/

					// TODO: Here I need to check if this task was blocked before the last execution
					for (u8 j = 0; j < osTasksCreated; j++)
					{
						if (status[j] == 1)
						{
							if (OsKernel.osTaskList[idx]->taskExecStatus == OS_TASK_READY)
							{
								OsKernel.osNextTaskCallback = OsKernel.osTaskList[idx];
								status[idx] = 0;
								osTaskIndex = idx;
								return;
							}
						}
					}
				}
			}
			break;

			case OS_TASK_BLOCKED:
			{
				if (idx > osTaskIndex)
				{
					/* This case means there is blocked task with lower priority than the one executing 
					 * [Ready, Ready, osTaskIndex (Running), idx, ... , ...]
					 */
				}
				else
				{
					/* This case means there is blocked task with high priority than the one executing 
					 * [Ready, idx (Blocked), ... , ...]
					 */

					// TODO: Tell the scheduler that the next time this task is Ready we need to execute in the case above

					/* Set the value for the task with high priority that needs to be executed */
					status[idx] = 1;
				}
			}
			break;
		}
	} 
#endif
}

/**
  * @brief This function handles Pendable request for system service.
  */
NAKED void PendSV_Handler(void)
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
	// osSysTickHook();

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


void taskSortByPriority(u8 n)
{	
	osTaskObject *temp = NULL;
    for (u8 i = 0; i < n - 1; i++) {
        for (u8 j = 0; j < n - i - 1; j++) 
		{
            if (OsKernel.osTaskList[j]->taskPriority > OsKernel.osTaskList[j + 1]->taskPriority) 
			{
                temp = OsKernel.osTaskList[j];
                OsKernel.osTaskList[j] = OsKernel.osTaskList[j + 1];
                OsKernel.osTaskList[j + 1] = temp;
            }
        }
	}
}


WEAK void osIdleTask(void)
{
   /*TODO: Blink LED */
   __WFI();
}

void osDelay(const uint32_t tick)
{
    (void)tick;
}

WEAK void osReturnTaskHook(void)
{
    while(1)
    {
        __WFI();
    }
}

WEAK void osSysTickHook(void)
{
    __ASM volatile ("nop");
}

WEAK void osErrorHook(void* caller)
{
    while(1)
    {
    }
}
#include "osKernel.h"
#include "osQueue.h"
#include "osSemaphore.h"

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

static OsKernelCtrl OsKernel;               		// Create an instance of the Kernel Control Structure

/* Private functions declarations */
static void scheduler(void);
static u32 getNextContext(u32 currentStaskPointer);
void taskSortByPriority(u8 n);
void osDelayCount(void);
osTaskObject* findBlockedTaskFromQueue(u8 sender);
osTaskObject* findRunningTask(void);
void osYield(void);


bool osTaskCreate(osTaskObject* taskCtrlStruct, osPriorityType priority, void* taskFunction)
{
    static u8 taskCount = 0;

    /* Check that taskFunction and taskCtrlStruct is not NULL */
    if (NULL == taskFunction || NULL == taskCtrlStruct)
    {
        return false;
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
        return false;
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

    return true;
}

void osStart(void)
{

#ifdef OS_WITH_PRIORITY
	// Count the number of tasks created
	for (u8 i = 0 ; i < OS_MAX_TASKS - 1 ; i++)
	{
		if ( NULL != OsKernel.osTaskList[i]) osTasksCreated++;
	}
	taskSortByPriority(osTasksCreated);
	osTaskCreate(&idle, OS_LOW_PRIORITY, osIdleTask);	 // Create IDLE task with the lowest priority
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
	if (OsKernel.osCurrTaskCallback->delay != 0 && OsKernel.osCurrTaskCallback->taskExecStatus == OS_TASK_BLOCKED)
	{
		// Do nothing
	}
	else
	{
    	OsKernel.osCurrTaskCallback->taskExecStatus = OS_TASK_READY;
	}

    // Switch address memory points on current task for next task and change state of task
    OsKernel.osCurrTaskCallback = OsKernel.osNextTaskCallback;
    OsKernel.osCurrTaskCallback->taskExecStatus = OS_TASK_RUNNING;

    return OsKernel.osCurrTaskCallback->taskStackPointer;
}

/**
 * @brief This function will be executed in a exception context
 * It will do the context change for the tasks.
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
		/* Assign the IDLE task to be executed. IDLE task is the last osTaskCreated so it is at that index */
		if (OsKernel.osCurrTaskCallback != OsKernel.osTaskList[osTasksCreated])
		{
			OsKernel.osNextTaskCallback = OsKernel.osTaskList[osTasksCreated];

			/* Set the osTaskIndex to the IDLE task */
			osTaskIndex = osTasksCreated;
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

			/* Not implemented yet */
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
								status[j] = 0;
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
					 * [Ready, idx (Blocked), osTaskIndex (Running), ...]
					 */

					/* Tell the scheduler that the next time this task is Ready we need to execute in the case above */
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

    /**
     * Implementación de stacking para FPU:
     *
     * Las tres primeras corresponden a un testeo del bit EXEC_RETURN[4]. La instruccion TST hace un
     * AND estilo bitwise (bit a bit) entre el registro LR y el literal inmediato. El resultado de esta
     * operacion no se guarda y los bits N y Z son actualizados. En este caso, si el bit EXEC_RETURN[4] = 0
     * el resultado de la operacion sera cero, y la bandera Z = 1, por lo que se da la condicion EQ y
     * se hace el push de los registros de FPU restantes
     */
    __ASM volatile ("tst lr, 0x10");
    __ASM volatile ("it eq");
    __ASM volatile ("vpusheq {s16-s31}");

    /**
     * Cuando se ingresa al handler de PendSV lo primero que se ejecuta es un push para
	 * guardar los registros R4-R11 y el valor de LR, que en este punto es EXEC_RETURN
	 * El push se hace al reves de como se escribe en la instruccion, por lo que LR
	 * se guarda en la posicion 9 (luego del stack frame). Como la funcion getNextContext
	 * se llama con un branch con link, el valor del LR es modificado guardando la direccion
	 * de retorno una vez se complete la ejecucion de la funcion
	 * El pasaje de argumentos a getContextoSiguiente se hace como especifica el AAPCS siendo
	 * el unico argumento pasado por RO, y el valor de retorno tambien se almacena en R0
	 *
	 * NOTA: El primer ingreso a este handler (luego del reset) implica que el push se hace sobre el
	 * stack inicial, ese stack se pierde porque no hay seguimiento del MSP en el primer ingreso
     */
    __ASM volatile ("push {r4-r11, lr}");
    __ASM volatile ("mrs r0, msp");
    __ASM volatile ("bl %0" :: "i"(getNextContext));
    __ASM volatile ("msr msp, r0");
    __ASM volatile ("pop {r4-r11, lr}");    //Recuperados todos los valores de registros

    /**
     * Implementación de unstacking para FPU:
     *
     * Habiendo hecho el cambio de contexto y recuperado los valores de los registros, es necesario
     * determinar si el contexto tiene guardados registros correspondientes a la FPU. si este es el caso
     * se hace el unstacking de los que se hizo PUSH manualmente.
     */
    __ASM volatile ("tst lr,0x10");
    __ASM volatile ("it eq");
    __ASM volatile ("vpopeq {s16-s31}");

    /* Se hace un branch indirect con el valor de LR que es nuevamente EXEC_RETURN */
    __ASM volatile ("bx lr");

}

/**
  * @brief This function handles System tick timer.
  * This will be executed every OS_SYSTICK_TICK time.
  */
void SysTick_Handler(void)
{
    scheduler();

	/* Check if there is any task blocked and with a delay */
	osDelayCount();


	/* This is a function that can be used by the User after the scheduler does it's job */
	osSysTickHook();

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
     * completed before next instruction is executed.
     */
    __DSB();

}


/**
 * @brief Private function that will sort the tasks by priorities. 
 */
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

/**
 *	@brief This function decrement the time if a task is blocked and there is a delay on it. 
 */
void osDelayCount(void)
{
	osTaskObject *task = NULL;
	
	/* Find the task */
	for (u8 i = 0; i < osTasksCreated; i++)
	{
		if(OS_TASK_BLOCKED == OsKernel.osTaskList[i]->taskExecStatus && 0 != OsKernel.osTaskList[i]->delay)
		{
			/* Found the current task */
			task = OsKernel.osTaskList[i];
			task->delay--;
			if(task->delay == 0)
			{
				task->taskExecStatus = OS_TASK_READY;
			}
		}
	}
}


void osDelay(const u32 tick)
{
	/* Disable SysTick_IRQn so is not invocated in here */
	NVIC_DisableIRQ(SysTick_IRQn);

	osTaskObject *task = NULL;
	
	/* Find the task */
	for (u8 i = 0; i < osTasksCreated; i++)
	{
		if(OsKernel.osTaskList[i]->taskExecStatus == OS_TASK_RUNNING)
		{
			/* Found the current task */
			task = OsKernel.osTaskList[i];
			break;
		}
	}

	task->taskExecStatus = OS_TASK_BLOCKED;
	task->delay=tick;

	osYield();

    NVIC_EnableIRQ(SysTick_IRQn);
}


void blockTaskFromQueue(osQueueObject *queue, u8 sender)
{
    NVIC_DisableIRQ(SysTick_IRQn);
    osTaskObject *task = NULL;
    task = findRunningTask();
    if (task != NULL)
    {
        if(sender)  task->queueBlockedFromFull = true;    // If this comes from a QueueSend
        else        task->queueBlockedFromEmpty = true;   // If this comes from a QueueReceive
        if(sender)  task->queueFull  = queue;             // This is the queue that is causing the Full blocking
        else        task->queueEmpty = queue;             // This is the queue that is causing the Empty blocking
        task->taskExecStatus = OS_TASK_BLOCKED;
    }
    osYield();
    NVIC_EnableIRQ(SysTick_IRQn);
}

void checkBlockedTaskFromQueue(osQueueObject *queue, u8 sender)
{
    NVIC_DisableIRQ(SysTick_IRQn);
    osTaskObject *task = NULL;
    task = findBlockedTaskFromQueue(sender);
    if (task != NULL)
    {
        task->taskExecStatus = OS_TASK_READY;
        if (sender) task->queueBlockedFromEmpty = false;
        else        task->queueBlockedFromFull  = false;   
    }
    osYield();
    NVIC_EnableIRQ(SysTick_IRQn);
}

osTaskObject* findBlockedTaskFromQueue(u8 sender)
{
    /* Find the task */
    for (u8 i = 0; i < osTasksCreated; i++)
    {
        if( OsKernel.osTaskList[i]->taskExecStatus == OS_TASK_BLOCKED)
        {
            switch(sender)
            {
                case 0:
                {
                    if (OsKernel.osTaskList[i]->queueBlockedFromFull == true)
                    {
                        return OsKernel.osTaskList[i];
                    }
                }
                break;

                case 1:
                {
                    if (OsKernel.osTaskList[i]->queueBlockedFromEmpty == true)
                    {
                        return OsKernel.osTaskList[i];
                    }
                }
                break;
            }
        }
    }
    return NULL;
}

osTaskObject* findRunningTask(void)
{
    osTaskObject *task = NULL;
    /* Find the task */
    for (u8 i = 0; i < osTasksCreated; i++)
    {
        if(OsKernel.osTaskList[i]->taskExecStatus == OS_TASK_RUNNING)
        {
            /* Found the current task */
            task = OsKernel.osTaskList[i];
            return task;
        }
    }
    return task;
}

void osYield(void)
{
    scheduler();
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __ISB();
    __DSB();
}

/* -----------------------------  Weak functions ----------------------------------- */
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


WEAK void osIdleTask(void)
{
   /*TODO: Blink LED */
   while(1)
   {
	__WFI();
   }
}

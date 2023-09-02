#include "OS_Core.h"

retType OsTaskCreate(const char* taskName, void* taskFunction, OsTaskCtrl* taskCtrlStruct)
{
    if (taskFunction == NULL || taskCtrlStruct == NULL) return API_OS_ERROR;
    taskCtrlStruct->taskEntryPoint = taskFunction;
    taskCtrlStruct->taskName = taskName;
    taskCtrlStruct->taskExecStatus = OS_TASK_READY;
    taskCtrlStruct->taskPriority = PRIORITY_LEVEL_1;
    return API_OK;
}

void OsStartScheduler(void)
{

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
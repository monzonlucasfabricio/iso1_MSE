/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : OS_Core.h
  * @brief          : Header for OS_Core.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OS_CORE_H__
#define __OS_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
#include "stdint.h"
#include "core_cm4.h"

/* Exported macro ------------------------------------------------------------*/
#define OS_MAX_TASKS 8
#define OS_MAX_STACK_SIZE 256
#define OS_MAX_TASK_NAME_CHAR 10

/* Exported types ------------------------------------------------------------*/

typedef uint32_t    u32;
typedef uint16_t    u16;
typedef uint8_t     u8;
typedef int32_t     i32;
typedef int16_t     i16;
typedef int8_t      i8;

typedef enum{
    API_OK          = 0,
    API_ERROR       = -1,
    API_OS_ERROR    = -2,
}retType;

/**
 * @brief Task execution status enum.
 * 
 */
typedef enum{
    OS_TASK_RUNNING     = 0,
    OS_TASK_READY       = 1,
    OS_TASK_BLOCKED     = 2,
    OS_TASK_SUSPENDED   = 3,
}OsTaskExecutionStatus;

/**
 * @brief Priority level enum.
 *
 * 
 */
typedef enum{
    PRIORITY_LEVEL_1    = 0,                // Highest Priority
    PRIORITY_LEVEL_2    = 1,
    PRIORITY_LEVEL_3    = 2,
    PRIORITY_LEVEL_4    = 3,                // Less Priority
}OsTaskPriorityLevel;


/**
 * @brief Structure used to control the Task.
 * 
 */
typedef struct{
    u32 taskMemory[OS_MAX_STACK_SIZE/4];    // Memory Size
    u32 taskStackPointer;                   // Store the task SP
    void* taskEntryPoint;                   // Entry point for the task
    OsTaskExecutionStatus taskExecStatus;   // Task current execution status
    OsTaskPriorityLevel taskPriority;       // Task priority (Not in used for now)
    u32 taskID;                             // Task ID
    char* taskName[OS_MAX_TASK_NAME_CHAR];  // Task name in string
}OsTaskCtrl;


/**
 * @brief Structure used to control the tasks execution.
 * 
 */
typedef struct{
    u32 osLastError;                        // Last error
    u32 osSystemStatus;                     // System status (Reset, Running, IRQ)
    u32 osScheduleExec;                     // Execution flag
    OsTaskCtrl* osCurrTaskCallback;         // Current task executing
    OsTaskCtrl* osNextTaskCallback;         // Next task to be executed
    OsTaskCtrl* osTaskList[OS_MAX_TASKS];   // List of tasks 
}OsKernelCtrl;

/* Exported constants --------------------------------------------------------*/


/* Exported functions prototypes ---------------------------------------------*/
void PendSV_Handler(void);
void SysTick_Handler(void);
void OsCreateTask(char* taskName, void* taskFunction, OsTaskCtrl* taskCtrlStruct);
void OsStartScheduler(void);
/* Private defines -----------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __OS_CORE_H__ */

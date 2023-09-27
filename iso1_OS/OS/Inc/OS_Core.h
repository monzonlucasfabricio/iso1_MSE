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
#include "cmsis_gcc.h"


/* Exported macro ------------------------------------------------------------*/
#define OS_MAX_TASKS            9 		    // MAX TASKS 8 + IDLE
#define OS_MAX_STACK_SIZE       256
#define OS_MAX_PRIORITY         4U          // Defines the maximum amount of priority.
#define OS_MAX_TASK_NAME_CHAR   10
#define OS_STACK_FRAME_SIZE     17
#define OS_SYSTICK_TICK         1000        // In milliseconds

/* Bits positions on Stack Frame */
#define XPSR_VALUE              1 << 24     // xPSR.T = 1
#define EXEC_RETURN_VALUE       0xFFFFFFF9  // EXEC_RETURN value. Return to thread mode with MSP, not use FPU
#define XPSR_REG_POSITION       1
#define PC_REG_POSTION          2
#define LR_REG_POSTION          3
#define R12_REG_POSTION         4
#define R3_REG_POSTION          5
#define R2_REG_POSTION          6
#define R1_REG_POSTION          7
#define R0_REG_POSTION          8
#define LR_PREV_VALUE_POSTION   9
#define R4_REG_POSTION          10
#define R5_REG_POSTION          11
#define R6_REG_POSTION          12
#define R7_REG_POSTION          13
#define R8_REG_POSTION          14
#define R9_REG_POSTION          15
#define R10_REG_POSTION         16
#define R11_REG_POSTION         17

#define WEAK __attribute__((weak))
#define NAKED __attribute__ ((naked))
/* Exported types ------------------------------------------------------------*/

typedef uint32_t    u32;
typedef uint16_t    u16;
typedef uint8_t     u8;
typedef int32_t     i32;
typedef int16_t     i16;
typedef int8_t      i8;

/**
 * @brief Return type values to check whether is an error or success.
 * 
 */
typedef enum{
    API_OK          =  0,
    API_ERROR       = -1,
    API_OS_ERROR    = -2,
}retType;

/**
 * @brief Status of the Operating System Enum 
 */
typedef enum{
    OS_STATUS_RUNNING   = 0,
    OS_STATUS_RESET     = 1,
    OS_STATUS_STOPPED   = 2,
}OsStatus;

/**
 * @brief Task execution status enum.
 * 
 */
typedef enum{
    OS_TASK_RUNNING     = 0,
    OS_TASK_READY       = 1,
    OS_TASK_BLOCKED     = 2,
    OS_TASK_SUSPENDED   = 3,
}osTaskStatusType;

/**
 * @brief Priority level enum.
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
    osTaskStatusType taskExecStatus;        // Task current execution status
    OsTaskPriorityLevel taskPriority;       // Task priority (Not in used for now)
    u32 taskID;                             // Task ID
    char* taskName[OS_MAX_TASK_NAME_CHAR];  // Task name in string
    u32 delay;
}osTaskObject;


/* Exported constants --------------------------------------------------------*/


/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief osTaskCreate helps to create a new task for the OS
 * @param osTaskObject* taskCtrlStruct
 * @param void* taskFunction
 * @param OsTaskPriorityLevel priority
 */
retType osTaskCreate(osTaskObject* taskCtrlStruct, void* taskFunction, OsTaskPriorityLevel priority);

/**
 * @brief This function needs to be invoqued after creating all the tasks 
 */
retType osStart(void);

/**
 * @param u32 tick -> time in milliseconds for the delay 
 */

void osDelay(const u32 tick);

/**
 * @brief Weak functions that can be used by the User if necesary 
 */
WEAK void osSysTickHook(void);
WEAK void osReturnTaskHook(void);
WEAK void osErrorHook(void* caller);
WEAK void osIdleTask(void);

/* Private defines -----------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __OS_CORE_H__ */

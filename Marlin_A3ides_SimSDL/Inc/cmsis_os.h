// cmsis_os.h (fake header for simulator)


#ifndef _CMSIS_OS_H
#define _CMSIS_OS_H

#include <inttypes.h>

typedef void * TaskHandle_t;

typedef TaskHandle_t osThreadId;

#define osWaitForever     0xFFFFFFFF     ///< wait forever timeout value

typedef enum  {
  osOK                    =     0,       ///< function completed; no error or event occurred.
  osEventSignal           =  0x08,       ///< function completed; signal event occurred.
  osEventMessage          =  0x10,       ///< function completed; message event occurred.
  osEventMail             =  0x20,       ///< function completed; mail event occurred.
  osEventTimeout          =  0x40,       ///< function completed; timeout occurred.
  osErrorParameter        =  0x80,       ///< parameter error: a mandatory parameter was missing or specified an incorrect object.
  osErrorResource         =  0x81,       ///< resource not available: a specified resource was not available.
  osErrorTimeoutResource  =  0xC1,       ///< resource not available within given time: a specified resource was not available within the timeout period.
  osErrorISR              =  0x82,       ///< not allowed in ISR context: the function cannot be called from interrupt service routines.
  osErrorISRRecursive     =  0x83,       ///< function called multiple times from ISR with same object.
  osErrorPriority         =  0x84,       ///< system cannot determine priority or thread has illegal priority.
  osErrorNoMemory         =  0x85,       ///< system is out of memory: it was impossible to allocate or reserve memory for the operation.
  osErrorValue            =  0x86,       ///< value of a parameter is out of range.
  osErrorOS               =  0xFF,       ///< unspecified RTOS error: run-time error but no other error message fits.
  os_status_reserved      =  0x7FFFFFFF  ///< prevent from enum down-size compiler optimization.
} osStatus;

typedef struct
{
	  osStatus                 status;     ///< status code: event or error information
} osEvent;


extern osStatus osDelay (uint32_t millisec);

extern int32_t osSignalSet (osThreadId thread_id, int32_t signal);

extern osEvent osSignalWait (int32_t signals, uint32_t millisec);

extern osThreadId osThreadGetId (void);


#endif // _CMSIS_OS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "uart_interface.hpp"   // for Dc_printf

void PrintFreeRtosHeapStats();
void PrintTaskStats();
void PrintQueueStats();
void PrintMemPoolStats();
void PrintMainHeapStackUsage();
#include "freertos_stats.hpp"
#include "freertos_mpool.h"             // for MemPool_t definition

void PrintFreeRtosHeapStats()
{
    HeapStats_t heapStats;

    vPortGetHeapStats(&heapStats);

    Dc_printf("\r\nFreeRTOS Heap Statistics:\r\n");
    Dc_printf("|%-10s|%-10s|%-12s|%-12s|%-12s|%-12s|%-12s|%-10s|\r\n",
        "TotSize",
        "FreeSize",
        "MinimumEver",
        "MaxFreeBlkSz",
        "MinFreeBlkSz",
        "NumFreeBlks",
        "SucAllocs",
        "SucFrees"
    );
    Dc_printf("|%-10u|%-10u|%-12u|%-12u|%-12u|%-12u|%-12u|%-10u|\r\n",
        configTOTAL_HEAP_SIZE,
        heapStats.xAvailableHeapSpaceInBytes,
        heapStats.xMinimumEverFreeBytesRemaining,
        heapStats.xSizeOfLargestFreeBlockInBytes,
        heapStats.xSizeOfSmallestFreeBlockInBytes,
        heapStats.xNumberOfFreeBlocks,
        heapStats.xNumberOfSuccessfulAllocations,
        heapStats.xNumberOfSuccessfulFrees
    );
}

void PrintTaskStats()
{
    TaskStatus_t * pxTaskStatusArray;
    UBaseType_t numTasks = uxTaskGetNumberOfTasks();
    UBaseType_t numRecordedTasks;
    pxTaskStatusArray = static_cast<TaskStatus_t*>( pvPortMalloc( numTasks * sizeof( TaskStatus_t ) ) );

    if( pxTaskStatusArray != NULL )
    {
        configRUN_TIME_COUNTER_TYPE totRunTimeCounter = 0;
        numRecordedTasks = uxTaskGetSystemState( pxTaskStatusArray, numTasks, &totRunTimeCounter );

        /* TODO: newlib nano printf does not support uint64_t and consumes large stack size, consider
         * substitute it with nanoprintf */
        Dc_printf("\r\nTask Statistics:\r\n");
        Dc_printf("Total Run Time: %lu us\r\n", totRunTimeCounter);
        Dc_printf("|%-16s|%-8s|%-10s|%-12s|%-18s|%-10s|%-10s|%-16s|\r\n",
            "Name",
            "State",
            "Priority",
            "BasePriority",
            "RunTime(us)",
            "(%)",
            "StackSize",
            "EverMinFreeStack"
        );
        for ( UBaseType_t i = 0; i < numRecordedTasks; i++ )
        {
            Dc_printf("|%-16s|%-8s|%-10u|%-12u|%-18lu|%-10.2f|%-10u|%-16u|\r\n",
                pxTaskStatusArray[i].pcTaskName,
                (pxTaskStatusArray[i].eCurrentState == eRunning) ? "Running" :
                (pxTaskStatusArray[i].eCurrentState == eReady) ? "Ready" :
                (pxTaskStatusArray[i].eCurrentState == eBlocked) ? "Blocked" :
                (pxTaskStatusArray[i].eCurrentState == eSuspended) ? "Suspended" : "Deleted",
                pxTaskStatusArray[i].uxCurrentPriority,
                pxTaskStatusArray[i].uxBasePriority,
                pxTaskStatusArray[i].ulRunTimeCounter,
                static_cast<double>(pxTaskStatusArray[i].ulRunTimeCounter) / static_cast<double>(totRunTimeCounter) * 100.0f,
                (pxTaskStatusArray[i].pxEndOfStack - pxTaskStatusArray[i].pxStackBase + 1) * sizeof(StackType_t),
                pxTaskStatusArray[i].usStackHighWaterMark * sizeof(StackType_t)
            );
        }

        vPortFree( pxTaskStatusArray );
    }
}

void PrintQueueStats()
{
    QueueStatus_t * pxQueueStatusArray;
    UBaseType_t numQueues = configQUEUE_REGISTRY_SIZE;
    UBaseType_t numRecordedQueues;

    pxQueueStatusArray = static_cast<QueueStatus_t*>( pvPortMalloc( numQueues * sizeof( QueueStatus_t ) ) );

    if( pxQueueStatusArray != NULL )
    {
        numRecordedQueues = uxQueueGetSystemState( pxQueueStatusArray, numQueues );

        Dc_printf("\r\nQueue Statistics:\r\n");
        Dc_printf("|%-16s|%-10s|%-10s|%-12s|%-16s|\r\n",
            "Name",
            "Length",
            "ItemSize",
            "MsgWaiting",
            "EverMaxWaiting"
        );
        for ( UBaseType_t i = 0; i < numRecordedQueues; i++ )
        {
            Dc_printf("|%-16s|%-10u|%-10u|%-12u|%-16u|\r\n",
                pxQueueStatusArray[i].pcQueueName,
                pxQueueStatusArray[i].uxLength,
                pxQueueStatusArray[i].uxItemSize,
                pxQueueStatusArray[i].uxMessagesWaiting,
                (configUSE_TRACE_FACILITY == 1) ? pxQueueStatusArray[i].ucEverMaxMessagesWaiting : 0
            );
        }

        vPortFree( pxQueueStatusArray );
    }
}

void PrintMemPoolStats()
{
    Dc_printf("\r\nMemory Pool Statistics:\r\n");

    MemPool_t * mp = g_memPoolList;
    if (mp == NULL)
    {
        Dc_printf("No memory pool available.\r\n");
        return;
    }

    Dc_printf("|%-16s|%-10s|%-10s|%-12s|%-12s|\r\n",
        "Name",
        "BlockSize",
        "BlockCount",
        "CurrentAvail",
        "EverMaxUsed"
    );

    while (mp != NULL)
    {
        Dc_printf("|%-16s|%-10u|%-10u|%-12u|%-12u|\r\n",
            mp->name,
            mp->bl_sz,
            mp->bl_cnt,
            uxQueueMessagesWaiting(mp->sem),
            mp->ever_max_used_blks
        );
        mp = mp->next;
    }
}

void PrintMainHeapStackUsage()
{
#define MAIN_HEAP_STACK_FILL_BYTE     ( 0xa5U )
    extern uint8_t _end; /* Symbol defined in the linker script represents the start of heap */
    extern uint8_t _estack; /* Symbol defined in the linker script represents the top address of stack(one byte more) */

    uint32_t unusedStart = (uint32_t)&_end;
    uint32_t unusedEnd;
    uint32_t unusedSize = 0;

    uint8_t prevByte = *(uint8_t *)(unusedStart - 1);
    uint8_t currentByte;
    bool foundNewBegin = prevByte == MAIN_HEAP_STACK_FILL_BYTE;
    uint32_t n = foundNewBegin ? 1 : 0;
    uint32_t s = foundNewBegin ? unusedStart - 1 : 0;
    uint32_t e = 0;
    Dc_printf("\r\nMain Heap and Stack Usage:\r\n");
    for (uint32_t addr = unusedStart; addr < (uint32_t)&_estack; addr++)
    {
        currentByte = *(uint8_t *)addr;
        if (prevByte != MAIN_HEAP_STACK_FILL_BYTE && currentByte == MAIN_HEAP_STACK_FILL_BYTE) {
            s = addr;
            n = 1;
            foundNewBegin = true;
        } else if (foundNewBegin && prevByte == MAIN_HEAP_STACK_FILL_BYTE && currentByte == MAIN_HEAP_STACK_FILL_BYTE) {
            n++;
        } else if (foundNewBegin && prevByte == MAIN_HEAP_STACK_FILL_BYTE && currentByte != MAIN_HEAP_STACK_FILL_BYTE) {
            e = addr - 1;
            if (n > unusedSize) {
                unusedStart = s;
                unusedEnd = e;
                unusedSize = n;
            }
            if (n > 0x100) {
                Dc_printf("Found a large unused area of %u bytes at 0x%08x to 0x%08x\r\n", n, s, e);
            }
            foundNewBegin = false;
        }
        prevByte = currentByte;
    }

    Dc_printf("|%-10s|%-15s|%-15s|%-10s|%-15s|%-16s|%-10s|\r\n",
        "HeadStart",
        "EverMaxHeapHigh",
        "EverMaxHeapUsed",
        "StackHigh",
        "EverMinStackLow",
        "EverMaxStackUsed",
        "FreeBytes"
    );
    Dc_printf("|0x%-8x|0x%-13x|%-15u|0x%-8x|0x%-13x|%-16u|%-10u|\r\n",
        (uint32_t)&_end,
        unusedStart,
        unusedStart - (uint32_t)&_end + 1,
        (uint32_t)&_estack,
        unusedEnd,
        (uint32_t)&_estack - unusedEnd,
        unusedSize
    );
}

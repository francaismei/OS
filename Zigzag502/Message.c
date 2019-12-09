#include "studentConfiguration.h"
#include "pcb.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
/*void   CheckDisk( long DiskID, long *ReturnedError )
{
    MEMORY_MAPPED_IO mmio;
    mmio.Mode = Z502CheckDisk;
    mmio.Field1 = DiskID;
    mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
    MEM_WRITE(Z502Disk, &mmio);
}*/


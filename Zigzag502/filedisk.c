#include "pcb.h"
#include "filedisk.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include             "studentConfiguration.h"

int FNode = 1;

void IncreaseAllocateFNode(void)
{
    FNode++;
}
int getFNode(void)
{
    return FNode;
}
int diskFormat(int DiskID)
{
    if (DiskID >= 8){
        return -1;
    }
    int i, j;
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    FileDisk->block_int[0][0] = DiskID << 8 | 0x5A;
    FileDisk->block_int[0][1] = (UINT16)0x0800;
    FileDisk->block_int[0][2] = (UINT16)0x8004;
    FileDisk->block_int[0][3] = (UINT16)0x0001;
    FileDisk->block_int[0][4] = (UINT16)0x0011;
    FileDisk->block_int[0][5] = (UINT16)0x0600;
    FileDisk->block_int[0][6] = (UINT16)0x0000;
    FileDisk->block_int[0][7] = (UINT16)0x2E00;
    FileDisk->block_int[1][0] = (UINT16)0xFFFF;
    FileDisk->block_int[1][1] = (UINT16)0xE0FF;
    /*FileDisk->block_int[1][1] = (UINT16)0xFFFF;
    FileDisk->block_int[1][2] = (UINT16)0xFFFF;
    FileDisk->block_int[1][3] = (UINT16)0xFFFF;
    FileDisk->block_int[1][4] = (UINT16)0xFFFF;
    FileDisk->block_int[1][5] = (UINT16)0xE0FF;*/
    for (j = 2; j < 8; j++){
        FileDisk->block_int[1][j] = (UINT16)0x0000;
    }
    for (i = 2; i < 13; i++){
        for (j = 0; j < 8; j++){
            FileDisk->block_int[i][j] = (UINT16)0x0000;
        }
    }
    for (i = 13; i < 17; i++){
        for (j = 0; j < 8; j++){
            FileDisk->block_int[i][j] = (UINT16)0xFFFF;
        }
    }
    /*for (i = 2; i < 13; i++){
        memset(FileDisk->block_int[i], 0, sizeof(int));
    }
    for (; i < 17; i++){
        memset(FileDisk->block_int[i], -1, sizeof(int));
    }*/
    FileDisk->block_int[17][0] = (UINT16)0x7200;
    FileDisk->block_int[17][1] = (UINT16)0x6F6F;
    FileDisk->block_int[17][2] = (UINT16)0x0074;
    FileDisk->block_int[17][3] = (UINT16)0x0000;
    FileDisk->block_int[17][4] = (UINT16)0x09EE;
    FileDisk->block_int[17][5] = (UINT16)0xFB00;     //1-level index
    FileDisk->block_int[17][6] = (UINT16)0x0012;
    FileDisk->block_int[17][7] = (UINT16)0x0000;
    UINT32 time = GetSampleTimeNow();
    //aprintf("time: %x\n", time);
    UINT16 msb_time = time >> 8;
    UINT16 lsb_time = time & 0xFF;
    FileDisk->block_int[17][5] |= lsb_time;
    FileDisk->block_int[17][4] = (msb_time >> 8) | (msb_time << 8);
    UINT16 temp = 0x0013;
    for (j = 0; j < 8; j++){
        FileDisk->block_int[18][j] = temp;
        temp += (UINT16)0x1;
    }
    for (i = 0x13; i < 0x1B; i++){
        for (j = 0; j < 8; j++){
            FileDisk->block_int[i][j] = 0;
        }
    }
    /*for (i = 19; i < 27; i++){
        for (j = 0; j < 8; j++){
            FileDisk->block_int[i][j] = temp;
            temp += (UINT16)0x1;
        }
    }
    for (i = 27; i < 91; i++){
        for (j = 0; j < 8; j++){
            FileDisk->block_int[i][j] = temp;
            temp += (UINT16)0x1;
        }
    }*/
    for (i = 0; i < 27; i++){
        //PHYSICAL_DISK_WRITE(DiskID, i, (char*)(FileDisk->block_char[i]));
        diskWritten(DiskID, i, FileDisk->block_char[i]);
    }
    /*PCB* pcb = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb = QNextItemInfo(2);
    strcpy(pcb->CurrentDirectory, "root");
    pcb->CurrentSector = 17;
    pcb->diskID = DiskID;
    doOneUnlck(2);
    FILEDISK* FileDisk1 = (FILEDISK*)malloc(sizeof(FILEDISK));
    char str[16];
    //diskRead(DiskID, 17, str);
    for (int i = 0; i < 19; i++){
        diskRead(DiskID, i, str);
        memcpy(FileDisk1->block_char[i], str, sizeof(str));
        for (int j = 0; j < 8; j++)
            aprintf("%X ", FileDisk1->block_int[i][j]);
        aprintf("\n");
    }*/
    //strcpy(FileDisk->block_char[19], str);
    //aprintf("%s asdasdas\n", str);
    /*for (int i = 0; i < 8; j++){
        aprintf("%X ", FileDisk->block_int[19][i]);
    }*/
    return 0;
}

int CheckDisk1(int diskID)
{
    MEMORY_MAPPED_IO mmio;
    mmio.Mode = Z502CheckDisk;
    mmio.Field1 = diskID;
    mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
    MEM_WRITE(Z502Disk, &mmio);
    return 0;
}
void diskRead(int diskID, int sector, char* str)
{
    MEMORY_MAPPED_IO mmio;
    mmio.Mode = Z502Status;
    mmio.Field1 = diskID;
    mmio.Field2 = mmio.Field3 = 0;
    MEM_READ(Z502Disk, &mmio);
    PCB* pcb_disk = (PCB* )malloc(sizeof(PCB));
    PCB* pcb_wready = (PCB* )malloc(sizeof(PCB));
    //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
    if (mmio.Field2 != DEVICE_FREE){
        doOnelck(2);
        pcb_disk = QNextItemInfo(2);
        doOneUnlck(2);
        //removeProcess(2);
        addToQueue(5 + diskID, pcb_disk);
            //aprintf("what hell?\n");
        // Disk hasn't been used - should be free
        //doOnelck(10);  //critical variable ok
        while (processToRun() == (void*)-1){
            //CALL(DoASleep(1));
            CALL(DoASleep(0));
        }
        //ok = 0;
        //doOneUnlck(10);
        mmio.Mode = Z502Status;
        mmio.Field1 = diskID;
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        if (mmio.Field2 != DEVICE_FREE){
            //aprintf("what hell?\n");
            pcb_disk = removeProcess(2);
            //pcb_disk->diskID = diskID;
            //addToQueue(5, pcb_disk);
            pcb_wready = processToRun();
            addToQueue(2, pcb_wready);
            removeProcess2(1, pcb_wready);
            mmio.Mode = Z502StartContext;
            mmio.Field1 = (long)(pcb_wready->NewContext);
            //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
            mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
            //printf("asdasdaeeeeeeeeeee\n");
            //check_state("running", pcb_wready);
            //printf("asdasdaeeeeeeeeeee\n");
            MEM_WRITE(Z502Context, &mmio);
            //  aprintf("%dfffff\n",mmio.Field4);
            if (mmio.Field4 != ERR_SUCCESS){
                printf("Start Context has an error.\n");
                exit(0);
            }
        }
        else{
            /*pcb_wready = processToRun();
            if (pcb_wready->ProcessID == pcb_disk->ProcessID){
                removeProcess2(1, pcb_disk);
            }*/
            doOnelck(1);
            pcb_wready = QItemExists(1, pcb_disk);
            doOneUnlck(1);
            if (pcb_wready != (void*)-1){
                removeProcess2(1, pcb_disk);
            }
            else{
                removeProcess2(5 + diskID, pcb_disk);
            }
        }
    }


    mmio.Mode = Z502DiskRead;
    mmio.Field1 = diskID; // Pick same disk location
    mmio.Field2 = sector;
    mmio.Field3 = (long)str;

    // Do the hardware call to read data from disk
    MEM_WRITE(Z502Disk, &mmio);
    mmio.Mode = Z502Status;
    mmio.Field1 = diskID;
    mmio.Field2 = mmio.Field3 = 0;
    MEM_READ(Z502Disk, &mmio);
    //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
    if (mmio.Field2 != DEVICE_FREE){
        doOnelck(2);
        pcb_disk = QNextItemInfo(2);
        doOneUnlck(2);
        //removeProcess(2);
        addToQueue(5 + diskID, pcb_disk);
        // Disk hasn't been used - should be free
        //doOnelck(10);  //critical variable ok
        while (processToRun() == (void*)-1){
            //CALL(DoASleep(1));
            CALL(DoASleep(0));
        }
        //ok = 0;
        //doOneUnlck(10);
        //check_state("asd", processToRun());
        mmio.Mode = Z502Status;
        mmio.Field1 = diskID;
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        if (mmio.Field2 != DEVICE_FREE){
            pcb_disk = removeProcess(2);
            //pcb_disk->diskID = diskID;
            //addToQueue(5, pcb_disk);
            pcb_wready = processToRun();
            addToQueue(2, pcb_wready);
            removeProcess2(1, pcb_wready);
            mmio.Mode = Z502StartContext;
            mmio.Field1 = (long)(pcb_wready->NewContext);
            //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
            mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
            //printf("asdasdaeeeeeeeeeee\n");
            //check_state("running", pcb_wready);
            //printf("asdasdaeeeeeeeeeee\n");
            MEM_WRITE(Z502Context, &mmio);
            //  aprintf("%dfffff\n",mmio.Field4);
            if (mmio.Field4 != ERR_SUCCESS){
                printf("Start Context has an error.\n");
                exit(0);
            }
        }
        else{
            /*pcb_wready = processToRun();
            if (pcb_wready->ProcessID == pcb_disk->ProcessID){
                removeProcess2(1, pcb_disk);
            }*/
            doOnelck(1);
            pcb_wready = QItemExists(1, pcb_disk);
            doOneUnlck(1);
            if (pcb_wready != (void*)-1){
                removeProcess2(1, pcb_disk);
            }
            else{
                removeProcess2(5 + diskID, pcb_disk);
            }
        }
    }
}

void diskWritten(int diskID, int sector, char* str)
{
    MEMORY_MAPPED_IO mmio;
    mmio.Mode = Z502Status;
    mmio.Field1 = diskID;
    mmio.Field2 = mmio.Field3 = 0;
    MEM_READ(Z502Disk, &mmio);
    PCB* pcb_disk = (PCB* )malloc(sizeof(PCB));
    PCB* pcb_wready = (PCB* )malloc(sizeof(PCB));
    //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
    if (mmio.Field2 != DEVICE_FREE){
        doOnelck(2);
        pcb_disk = QNextItemInfo(2);
        doOneUnlck(2);
        //removeProcess(2);
        addToQueue(5 + diskID, pcb_disk);
        //addToQueue(1, pcb_disk);
            //aprintf("what hell?\n");
        // Disk hasn't been used - should be free
        //doOnelck(10);  //critical variable ok
        while (processToRun() == (void*)-1){
            //CALL(DoASleep(1));
            CALL(DoASleep(0));
        }
        //ok = 0;
        //doOneUnlck(10);
        mmio.Mode = Z502Status;
        mmio.Field1 = diskID;
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        if (mmio.Field2 != DEVICE_FREE){
            //aprintf("what hell?\n");
            pcb_disk = removeProcess(2);
            //pcb_disk->diskID = SystemCallData->Argument[0];
            //addToQueue(5, pcb_disk);
            pcb_wready = processToRun();
            addToQueue(2, pcb_wready);
            removeProcess2(1, pcb_wready);
            mmio.Mode = Z502StartContext;
            mmio.Field1 = (long)(pcb_wready->NewContext);
            //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
            mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
            //printf("asdasdaeeeeeeeeeee\n");
            //check_state("running", pcb_wready);
            //printf("asdasdaeeeeeeeeeee\n");
            MEM_WRITE(Z502Context, &mmio);
            //  aprintf("%dfffff\n",mmio.Field4);
            if (mmio.Field4 != ERR_SUCCESS){
                printf("Start Context has an error.\n");
                exit(0);
            }
        }
        else{
            /*pcb_wready = processToRun();
            if (pcb_wready->ProcessID == pcb_disk->ProcessID){
                removeProcess2(1, pcb_disk);
            }*/
            doOnelck(1);
            pcb_wready = QItemExists(1, pcb_disk);
            doOneUnlck(1);
            if (pcb_wready != (void*)-1){
                removeProcess2(1, pcb_disk);
            }
            else{
                //aprintf("ARE YOU OK?\n");
                //check_state("ttt", pcb_disk);
                removeProcess2(5 + diskID, pcb_disk);
            }
        }
    }
    // Start the disk by writing a block of data


    mmio.Mode = Z502DiskWrite;
    mmio.Field1 = diskID;
    mmio.Field2 = sector;
    mmio.Field3 = (long)str;
    MEM_WRITE(Z502Disk, &mmio);


  /*  mmio.Mode = Z502Status;
    mmio.Field1 = SystemCallData->Argument[0];
    mmio.Field2 = mmio.Field3 = 0;
    MEM_READ(Z502Disk, &mmio);
    if (mmio.Field2 != DEVICE_IN_USE) {       // Disk should report being used
        printf( "Got erroneous result for Disk Status - Device is free.\n");
    }*/

    //PCB* pcb_disk = (PCB* )malloc(sizeof(PCB));
    //PCB* pcb_wready = (PCB* )malloc(sizeof(PCB));

    mmio.Mode = Z502Status;
    mmio.Field1 = diskID;
    mmio.Field2 = mmio.Field3 = 0;
    MEM_READ(Z502Disk, &mmio);
    //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
    if (mmio.Field2 != DEVICE_FREE){
        doOnelck(2);
        pcb_disk = QNextItemInfo(2);
        doOneUnlck(2);
        //removeProcess(2);
        addToQueue(5 + diskID, pcb_disk);
            //aprintf("what hell?\n");
        // Disk hasn't been used - should be free
        //doOnelck(10);  //critical variable ok
        while (processToRun() == (void*)-1){
            //CALL(DoASleep(1));
            CALL(DoASleep(0));
        }
        //ok = 0;
        //doOneUnlck(10);
        mmio.Mode = Z502Status;
        mmio.Field1 = diskID;
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        if (mmio.Field2 != DEVICE_FREE){
            //aprintf("what hell?\n");
            pcb_disk = removeProcess(2);
            //pcb_disk->diskID = diskID;
            //addToQueue(5, pcb_disk);
            pcb_wready = processToRun();
            addToQueue(2, pcb_wready);
            removeProcess2(1, pcb_wready);
            mmio.Mode = Z502StartContext;
            mmio.Field1 = (long)(pcb_wready->NewContext);
            //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
            mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
            //printf("asdasdaeeeeeeeeeee\n");
            //check_state("running", pcb_wready);
            //printf("asdasdaeeeeeeeeeee\n");
            MEM_WRITE(Z502Context, &mmio);
            //  aprintf("%dfffff\n",mmio.Field4);
            if (mmio.Field4 != ERR_SUCCESS){
                printf("Start Context has an error.\n");
                exit(0);
            }
        }
        else{
            doOnelck(1);
            pcb_wready = QItemExists(1, pcb_disk);
            doOneUnlck(1);
            if (pcb_wready != (void*)-1){
                removeProcess2(1, pcb_disk);
            }
            else{
                //check_state("wrong", pcb_disk);
                removeProcess2(5 + diskID, pcb_disk);
            }
        }
    }
}

int open_dir(int diskID, char* dir_name)
{
    if (diskID >= 8){
        return -1;
    }
    //.. implement if possible
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    if (diskID == -1){
        diskID = pcb_running->diskID;
    }
    else  pcb_running->diskID = diskID;
    int cu_sector;
    char str[8], name[14];
    cu_sector = pcb_running->CurrentSector;
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //memcpy(FileDisk->block_char[cu_sector], str, sizeof(str));
    memcpy(str, FileDisk->block_char[cu_sector] + 1, 7);
    if (strcmp(str, dir_name) == 0){
        return 0;
    }
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    int i, j;
    int flag = 1;
    int index_sector;
    //int index_sector_j;
    int char_iter;
    UINT16 bitmap_sect;
    int bit_j;
    UINT16 bit_temp;
    int bit_cnt;
    UINT16 index_location;
    //aprintf("The index is: %X\n", level);
    if (level == 0x0200){// only 1 level
            //aprintf("hahaha %X\n", cu_sector);
        diskRead(diskID, index, FileDisk->block_char[index]);
        //strcpy(FileDisk->block_char[index], str);
        for (i = 0; i < 8; i++){
            index_sector = FileDisk->block_int[index][i];
            diskRead(diskID, index_sector, FileDisk->block_char[index_sector]);
            if ((FileDisk->block_int[index_sector][0] & 0x00FF) == 0){
                flag = 0;
                break;
            }
            memcpy(name, FileDisk->block_char[index_sector] + 1, 7);
    //aprintf("%s   %s\n", name, dir_name);
            if (strcmp(name, dir_name) == 0){
                //aprintf("%s   %s\n", name, dir_name);
                //strcpy(pcb_running->CurrentDirectory, dir_name);
                pcb_running->CurrentSector = index_sector;
                pcb_running->iNode = FileDisk->block_int[index_sector][0] & 0x00FF;
                break;
            }
        }
        if (flag == 0){
            for (char_iter = 1; char_iter < strlen(dir_name) + 1; char_iter++){
                FileDisk->block_char[index_sector][char_iter] = dir_name[char_iter - 1];
            }
            FileDisk->block_int[index_sector][0] |= getFNode();
            pcb_running->iNode = getFNode();
            IncreaseAllocateFNode();
            //strcpy(pcb_running->CurrentDirectory, dir_name);
            pcb_running->CurrentSector = index_sector;
            UINT32 time = GetSampleTimeNow();
            //aprintf("time: %x\n", time);
            UINT16 msb_time = time >> 8;
            UINT16 lsb_time = time & 0xFF;
            FileDisk->block_int[index_sector][5] = (cu_sector << 11) | (0x3 << 8);
            FileDisk->block_int[index_sector][5] |= lsb_time;
            FileDisk->block_int[index_sector][4] = (msb_time >> 8) | (msb_time << 8);
            for (bitmap_sect = 0x1; bitmap_sect < 0x11; bitmap_sect++){
                diskRead(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
                //strcpy(FileDisk->block_char[bitmap_sect], str);
                if (FileDisk->block_int[bitmap_sect][7] == 0xFFFF){
                    continue;
                }
                for (bit_j = 0; bit_j < 8; bit_j++){
                    if (FileDisk->block_int[bitmap_sect][bit_j] != 0xFFFF){
                        break;
                    }
                }
                bit_temp = FileDisk->block_int[bitmap_sect][bit_j];
                bit_cnt = 0;
                while (bit_temp){
                    bit_temp &= (bit_temp - 1);
                    bit_cnt++;
                }
                index_location = (bitmap_sect - 1) << 7 | bit_j << 4 | bit_cnt;  //change 3 to 4
                FileDisk->block_int[index_sector][6] = index_location;
                //UINT16 index_id = index_location + 1;
                for (j = 0; j < 8; j++){
                    FileDisk->block_int[index_location][j] = index_location + 1 + j;
                }
                if ((bit_cnt + 9) / 16 == 0){  //change 10 to 9
                    bit_temp = FileDisk->block_int[bitmap_sect][bit_j] << 8 | FileDisk->block_int[bitmap_sect][bit_j] >> 8;
                    for (j = 15 - bit_cnt; j > 15 - bit_cnt - 9; j--){ //change 10 to 9
                        bit_temp |= 0x1 << j;
                    }
                    FileDisk->block_int[bitmap_sect][bit_j] = bit_temp << 8 | bit_temp >> 8;
                }
                else{
                    FileDisk->block_int[bitmap_sect][bit_j++] = 0xFFFF;
                    bit_temp = 0;
                    for (j = 15; j > 15 - (bit_cnt + 9 - 16); j--){  //change 10 to 9
                        bit_temp |= 0x1 << j;
                    }
                    FileDisk->block_int[bitmap_sect][bit_j] = bit_temp << 8 | bit_temp >> 8;
                }
                break;

            }
            diskWritten(diskID, index_sector, FileDisk->block_char[index_sector]);
            diskWritten(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
            diskWritten(diskID, index_location, FileDisk->block_char[index_location]);
            for (int i = index_location; i < index_location + 9; i++){
                diskWritten(diskID, i, FileDisk->block_char[i]);
            }
        }
    }
    /*else if (level == 4){
        diskRead(diskID, index, str);
        strcpy(FileDisk->block_char[index], str);
        for (i = 0; i < 8; i++){
            index_sector = FileDisk->block_int[index][i];
            diskRead(diskID, index_sector, str);
            strcpy(FileDisk->block_char[index_sector], str);
            for (j = 0; j < 8; j++){
                index_sector_j = FileDisk->block_int[index_sector][j];
                diskRead(diskID, index_sector_j, str);
                strcpy(FileDisk->block_char[index_sector_j], str);
                if ((FileDisk->block_int[index_sector_j][0] & 0xFF00) == 0){
                    flag = 0;
                    break;
                }
                memcpy(name, FileDisk->block_char[j] + 1, 7);
                if (strcmp(name, dir_name) == 0){
                    strcpy(pcb_running->CurrentDirectory, dir_name);
                    pcb_running->CurrentSector = index_sector;
                    break;
                }
            }
            if (flag == 0){
                for (char_iter = 1; char_iter < strlen(dir_name) + 1; char_iter++){
                    FileDisk->block_char[index_sector][char_iter] = dir_name[i - 1];
                }
                FileDisk->block_int[index_sector][0] = (getFNode() << 8) | 0x00FF;
                IncreaseAllocateFNode();
                strcpy(pcb_running->CurrentDirectory, dir_name);
                pcb_running->CurrentSector = index_sector;
            }
        }
    }*/
    return 0;



}

int create_dir(char* dir_name)
{
    if (strlen(dir_name) >= 8){
        return -1;
    }
    //.. implement if possible
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    int diskID = pcb_running->diskID;
    int cu_sector;
    char name[14];
    cu_sector = pcb_running->CurrentSector;
    //aprintf("sector: %X\n", cu_sector);
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //strcpy(FileDisk->block_char[cu_sector], str);
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    if (level == 0x0200){// only 1 level
        diskRead(diskID, index, FileDisk->block_char[index]);
        //strcpy(FileDisk->block_char[index], str);
        int i, j;
        int flag = 1;
        int index_sector;
        UINT16 bitmap_sect;
        int bit_j;
        UINT16 bit_temp;
        int bit_cnt;
        UINT16 index_location;
        //CheckDisk1(diskID);
        for (i = 0; i < 8; i++){
            index_sector = FileDisk->block_int[index][i];
            //aprintf("index      %d\n", index_sector);
            diskRead(diskID, index_sector, FileDisk->block_char[index_sector]);
            //aprintf("  I also want to see.\n");
            //strcpy(FileDisk->block_char[index_sector], str);
            if ((FileDisk->block_int[index_sector][0] & 0x00FF) == 0){
                flag = 0;
                break;
            }
            memcpy(name, FileDisk->block_char[i] + 1, 7);
            if (strcmp(name, dir_name) == 0){
                //strcpy(pcb_running->CurrentDirectory, dir_name);
                //pcb_running->CurrentSector = index_sector;
                break;
            }
        }
        if (flag == 0){
            for (int char_iter = 1; char_iter < strlen(dir_name) + 1; char_iter++){
                FileDisk->block_char[index_sector][char_iter] = dir_name[char_iter - 1];
            }
            FileDisk->block_int[index_sector][0] |= getFNode();
            IncreaseAllocateFNode();
            //strcpy(pcb_running->CurrentDirectory, dir_name);
            //pcb_running->CurrentSector = index_sector;
            UINT32 time = GetSampleTimeNow();
            //aprintf("time: %x\n", time);
            UINT16 msb_time = time >> 8;
            UINT16 lsb_time = time & 0xFF;
            FileDisk->block_int[index_sector][5] = (cu_sector << 11) | (0x3 << 8);
            //aprintf("  I want to see.  %X\n", ((UINT16)cu_sector << 11) | (0x011 << 8));
            FileDisk->block_int[index_sector][5] |= lsb_time;
            FileDisk->block_int[index_sector][4] = (msb_time >> 8) | (msb_time << 8);
            for (bitmap_sect = 0x1; bitmap_sect < 0x11; bitmap_sect++){
                //aprintf("UUUUUUUU\n");
                diskRead(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
                //strcpy(FileDisk->block_char[bitmap_sect], str);
                //aprintf("%X \n", FileDisk->block_int[bitmap_sect][7]);
                if (FileDisk->block_int[bitmap_sect][7] == 0xFFFF){
                    continue;
                }
                //aprintf("UUU   %X\n", bitmap_sect);
                for (bit_j = 0; bit_j < 8; bit_j++){
                    if (FileDisk->block_int[bitmap_sect][bit_j] != 0xFFFF){
                        break;
                    }
                }
                bit_temp = FileDisk->block_int[bitmap_sect][bit_j];
                bit_cnt = 0;
                while (bit_temp){
                    bit_temp &= (bit_temp - 1);
                    bit_cnt++;
                }
                index_location = (bitmap_sect - 1) << 7 | bit_j << 4 | bit_cnt; //bit_j - 1 change to bit_j
                FileDisk->block_int[index_sector][6] = index_location;
                //UINT16 index_id = index_location + 1;
                for (j = 0; j < 8; j++){
                    FileDisk->block_int[index_location][j] = index_location + 1 + j;
                }
                for (i = index_location + 1; i < index_location + 8; i++){
                    for (j = 0; j < 8; j++){
                        FileDisk->block_int[i][j] = 0;
                    }
                }
                if ((bit_cnt + 9) / 16 == 0){
                    bit_temp = FileDisk->block_int[bitmap_sect][bit_j] << 8 | FileDisk->block_int[bitmap_sect][bit_j] >> 8;
                    for (j = 15 - bit_cnt; j > 15 - bit_cnt - 9; j--){
                        bit_temp |= 0x1 << j;
                    }
                    FileDisk->block_int[bitmap_sect][bit_j] = bit_temp << 8 | bit_temp >> 8;
                }
                else{
                    FileDisk->block_int[bitmap_sect][bit_j++] = 0xFFFF;
                    bit_temp = 0;
                    for (j = 15; j > 15 - (bit_cnt + 9 - 16); j--){
                        bit_temp |= 0x1 << j;
                    }
                    FileDisk->block_int[bitmap_sect][bit_j] = bit_temp << 8 | bit_temp >> 8;
                }
                break;

            }
            diskWritten(diskID, index_sector, FileDisk->block_char[index_sector]);
            diskWritten(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
            diskWritten(diskID, index_location, FileDisk->block_char[index_location]);
            for (int i = index_location; i < index_location + 9; i++){
                diskWritten(diskID, i, FileDisk->block_char[i]);
            }
            return ERR_SUCCESS;
        }
        else{
            return -1;
        }
    }
}
int open_file(char* file_name, int* inode)
{
    if (strlen(file_name) >= 8){
        return -1;
    }
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    int diskID = pcb_running->diskID;
    int cu_sector;
    char str[8], name[14];
    cu_sector = pcb_running->CurrentSector;
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //memcpy(FileDisk->block_char[cu_sector], str, sizeof(str));
    memcpy(str, FileDisk->block_char[cu_sector] + 1, 7);
    if (strcmp(str, file_name) == 0){
        *inode = FileDisk->block_int[cu_sector][0] & 0x00FF;
        return 0;
    }
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    int i, j, j_1, j_2;
    int flag = 1;
    int index_sector;
    //int index_sector_j;
    int char_iter;
    UINT16 bitmap_sect;
    int bit_j;
    UINT16 bit_temp;
    int bit_cnt;
    UINT16 index_location;
    //aprintf("The index is: %X\n", level);
    if (level == 0x0200){// only 1 level
            //aprintf("hahaha %X\n", cu_sector);
        diskRead(diskID, index, FileDisk->block_char[index]);
        //strcpy(FileDisk->block_char[index], str);
        for (i = 0; i < 8; i++){
            index_sector = FileDisk->block_int[index][i];
            diskRead(diskID, index_sector, FileDisk->block_char[index_sector]);
            if ((FileDisk->block_int[index_sector][0] & 0x00FF) == 0){
                flag = 0;
                break;
            }
            memcpy(name, FileDisk->block_char[index_sector] + 1, 7);
    //aprintf("%s   %s\n", name, dir_name);
            if (strcmp(name, file_name) == 0){
                //aprintf("%s   %s\n", name, dir_name);
                pcb_running->previousSector = cu_sector;
                //strcpy(pcb_running->CurrentDirectory, dir_name);
                pcb_running->iNode = FileDisk->block_int[index_sector][0] & 0x00FF;
                *inode = pcb_running->iNode;
                pcb_running->CurrentSector = index_sector;
                break;
            }
        }
        int reminder_1, reminder_2, new_bitmap_sector;
        if (flag == 0){
            for (int char_iter = 1; char_iter < strlen(file_name) + 1; char_iter++){
                FileDisk->block_char[index_sector][char_iter] = file_name[char_iter - 1];
            }
            FileDisk->block_int[index_sector][0] |= getFNode();
            *inode = getFNode();
            pcb_running->iNode = getFNode();
            IncreaseAllocateFNode();
            pcb_running->previousSector = cu_sector;
            //strcpy(pcb_running->CurrentDirectory, file_name);
            pcb_running->CurrentSector = index_sector;
            UINT32 time = GetSampleTimeNow();
            //aprintf("time: %x\n", time);
            UINT16 msb_time = time >> 8;
            UINT16 lsb_time = time & 0xFF;
            FileDisk->block_int[index_sector][5] = (cu_sector << 11) | (0x6 << 8);
            FileDisk->block_int[index_sector][5] |= lsb_time;
            FileDisk->block_int[index_sector][4] = (msb_time >> 8) | (msb_time << 8);
            for (bitmap_sect = 0x1; bitmap_sect < 0x11; bitmap_sect++){
                diskRead(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
                //strcpy(FileDisk->block_char[bitmap_sect], str);
                if (FileDisk->block_int[bitmap_sect][7] == 0xFFFF){
                    continue;
                }
                for (bit_j = 0; bit_j < 8; bit_j++){
                    if (FileDisk->block_int[bitmap_sect][bit_j] != 0xFFFF){
                        break;
                    }
                }
                bit_temp = FileDisk->block_int[bitmap_sect][bit_j];
                bit_cnt = 0;
                while (bit_temp){
                    bit_temp &= (bit_temp - 1);
                    bit_cnt++;
                }
                index_location = (bitmap_sect - 1) << 7 | bit_j << 4 | bit_cnt;
                FileDisk->block_int[index_sector][6] = index_location;
                //UINT16 index_id = index_location + 1;
                for (j = 0; j < 8; j++){
                    FileDisk->block_int[index_location][j] = index_location + 1 + j;
                }
                for (j_1 = 0; j_1 < 8; j_1++){
                    for (j_2 = 0; j_2 < 8; j_2++){
                        FileDisk->block_int[index_location + j_1 + 1][j_2] = index_location + 9 + 8 * j_1 + j_2;
                    }
                }
                for (j = 0; j < 8; j++){
                    for (j_1 = 0; j_1 < 8; j_1++){
                        for (j_2 = 0; j_2 < 8; j_2++){
                            FileDisk->block_int[index_location + 9 + j * 8 + j_1][j_2] = index_location + 73 + j * 64 + j_1 * 8 + j_2;
                        }
                    }
                }
                for (j = 0; j < 64; j++){
                    for (j_1 = 0; j_1 < 8; j_1++){
                        for (j_2 = 0; j_2 < 8; j_2++){
                            FileDisk->block_int[index_location + 73 + j * 8 + j_1][j_2] = 0;
                        }
                    }
                }
                reminder_1 = 128 - (bit_cnt | bit_j << 4);
                new_bitmap_sector = (585 - reminder_1) >> 7;
                reminder_2 = (585 - reminder_1) & 0x7F;
                for (j = 0; j <= new_bitmap_sector; j++){
                    for (j_2 = 0; j_2 < 8; j_2++){
                        FileDisk->block_int[bitmap_sect + j][j_2] = 0xFFFF;
                    }
                }
                for (j = 0; j < (reminder_2 >> 4); j++){
                    FileDisk->block_int[bitmap_sect + new_bitmap_sector + 1][j] = 0xFFFF;
                }
                bit_temp = 0;
                for (bit_j = 15; bit_j > 15 - (reminder_2 & 0x0F); bit_j--){
                    bit_temp |= 0x1 << bit_j;
                }
                FileDisk->block_int[bitmap_sect + new_bitmap_sector + 1][j] = bit_temp << 8 | bit_temp >> 8;
                //0x0249 = 585 = 1 + 8 + 64 + 512

                break;

            }
            diskWritten(diskID, index_sector, FileDisk->block_char[index_sector]);
            for (int i = bitmap_sect; i <= bitmap_sect + new_bitmap_sector + 1; i++){
                diskWritten(diskID, i, FileDisk->block_char[i]);
            }
            for (int i = index_location; i <= index_location + 584; i++){
                diskWritten(diskID, i, FileDisk->block_char[i]);
            }
            //diskWritten(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
            //diskWritten(diskID, index_location, FileDisk->block_char[index_location]);
        }
    }
    return 0;
}
int create_file(char* file_name)
{
    if (strlen(file_name) >= 8){
        return -1;
    }
    //.. implement if possible
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    int diskID = pcb_running->diskID;
    int cu_sector;
    char name[14];
    cu_sector = pcb_running->CurrentSector;
    //aprintf("sector: %X\n", cu_sector);
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //strcpy(FileDisk->block_char[cu_sector], str);
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    //aprintf("Index: %X\n", level);
    if (level == 0x0200){// only 1 level
        diskRead(diskID, index, FileDisk->block_char[index]);
        //strcpy(FileDisk->block_char[index], str);
        int i, j, j_1, j_2;
        int flag = 1;
        int index_sector;
        UINT16 bitmap_sect;
        int bit_j;
        UINT16 bit_temp;
        int bit_cnt;
        UINT16 index_location;
        for (i = 0; i < 8; i++){
            index_sector = FileDisk->block_int[index][i];
            diskRead(diskID, index_sector, FileDisk->block_char[index_sector]);
            //strcpy(FileDisk->block_char[index_sector], str);
            if ((FileDisk->block_int[index_sector][0] & 0x00FF) == 0){
                flag = 0;
                break;
            }
            memcpy(name, FileDisk->block_char[i] + 1, 7);
            if (strcmp(name, file_name) == 0){
                //strcpy(pcb_running->CurrentDirectory, file_name);
                pcb_running->CurrentSector = index_sector;
                break;
            }
        }
        int reminder_1, reminder_2, new_bitmap_sector;
        if (flag == 0){
            for (int char_iter = 1; char_iter < strlen(file_name) + 1; char_iter++){
                FileDisk->block_char[index_sector][char_iter] = file_name[char_iter - 1];
            }
            FileDisk->block_int[index_sector][0] |= getFNode();
            IncreaseAllocateFNode();
            //strcpy(pcb_running->CurrentDirectory, file_name);
            //pcb_running->CurrentSector = index_sector;
            UINT32 time = GetSampleTimeNow();
            //aprintf("time: %x\n", time);
            UINT16 msb_time = time >> 8;
            UINT16 lsb_time = time & 0xFF;
            FileDisk->block_int[index_sector][5] = (cu_sector << 11) | (0x6 << 8);
            FileDisk->block_int[index_sector][5] |= lsb_time;
            FileDisk->block_int[index_sector][4] = (msb_time >> 8) | (msb_time << 8);
            for (bitmap_sect = 0x1; bitmap_sect < 0x11; bitmap_sect++){
                diskRead(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
                //strcpy(FileDisk->block_char[bitmap_sect], str);
                if (FileDisk->block_int[bitmap_sect][7] == 0xFFFF){
                    continue;
                }
                for (bit_j = 0; bit_j < 8; bit_j++){
                    if (FileDisk->block_int[bitmap_sect][bit_j] != 0xFFFF){
                        break;
                    }
                }
                bit_temp = FileDisk->block_int[bitmap_sect][bit_j];
                bit_cnt = 0;
                while (bit_temp){
                    bit_temp &= (bit_temp - 1);
                    bit_cnt++;
                }
                index_location = (bitmap_sect - 1) << 7 | bit_j << 4 | bit_cnt;
                FileDisk->block_int[index_sector][6] = index_location;
                //UINT16 index_id = index_location + 1;
                for (j = 0; j < 8; j++){
                    FileDisk->block_int[index_location][j] = index_location + 1 + j;
                }
                for (j_1 = 0; j_1 < 8; j_1++){
                    for (j_2 = 0; j_2 < 8; j_2++){
                        FileDisk->block_int[index_location + j_1 + 1][j_2] = index_location + 9 + 8 * j_1 + j_2;
                    }
                }
                for (j = 0; j < 8; j++){
                    for (j_1 = 0; j_1 < 8; j_1++){
                        for (j_2 = 0; j_2 < 8; j_2++){
                            FileDisk->block_int[index_location + 9 + j * 8 + j_1][j_2] = index_location + 73 + j * 64 + j_1 * 8 + j_2;
                        }
                    }
                }
                for (j = 0; j < 64; j++){
                    for (j_1 = 0; j_1 < 8; j_1++){
                        for (j_2 = 0; j_2 < 8; j_2++){
                            FileDisk->block_int[index_location + 73 + j * 8 + j_1][j_2] = 0;
                        }
                    }
                }
                reminder_1 = 128 - (bit_cnt | bit_j << 4);
                new_bitmap_sector = (585 - reminder_1) >> 7;
                reminder_2 = (585 - reminder_1) & 0x7F;
                for (j = 0; j <= new_bitmap_sector; j++){
                    for (j_2 = 0; j_2 < 8; j_2++){
                        FileDisk->block_int[bitmap_sect + j][j_2] = 0xFFFF;
                    }
                }
                for (j = 0; j < (reminder_2 >> 4); j++){
                    FileDisk->block_int[bitmap_sect + new_bitmap_sector + 1][j] = 0xFFFF;
                }
                bit_temp = 0;
                for (bit_j = 15; bit_j > 15 - (reminder_2 & 0x0F); bit_j--){
                    bit_temp |= 0x1 << bit_j;
                }
                FileDisk->block_int[bitmap_sect + new_bitmap_sector + 1][j] = bit_temp << 8 | bit_temp >> 8;
                //0x0249 = 585 = 1 + 8 + 64 + 512

                break;

            }
            diskWritten(diskID, index_sector, FileDisk->block_char[index_sector]);
            for (int i = bitmap_sect; i <= bitmap_sect + new_bitmap_sector + 1; i++){
                diskWritten(diskID, i, FileDisk->block_char[i]);
            }
            for (int i = index_location; i <= index_location + 584; i++){
                diskWritten(diskID, i, FileDisk->block_char[i]);
            }
            //diskWritten(diskID, bitmap_sect, FileDisk->block_char[bitmap_sect]);
            //diskWritten(diskID, index_location, FileDisk->block_char[index_location]);
            return ERR_SUCCESS;
        }
        else{
            return -1;
        }
    }
}
int write_file(int inode, int logical_addr, char* writtenBuffer)
{
    /*if (strlen(writtenBuffer) > 17){
        return -1;
    }*/
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    int diskID = pcb_running->diskID;
    int cu_sector;
    char name[14];
    cu_sector = pcb_running->CurrentSector;
    if (inode != pcb_running->iNode){
        return -1;
    }
    //aprintf("sector: %X\n", cu_sector);
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //strcpy(FileDisk->block_char[cu_sector], str);
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    if (level == 0x0600){
        int col1 = logical_addr >> 6;
        int col2 = (logical_addr & 63) >> 3;
        int col3 = logical_addr & 7;
        //aprintf("%d %d %d\n", col1, col2, col3);
        diskRead(diskID, index, FileDisk->block_char[index]);
        UINT16 index1 = FileDisk->block_int[index][col1];
        //aprintf("index1: %d\n", index1);
        diskRead(diskID, index1, FileDisk->block_char[index1]);
        UINT16 index2 = FileDisk->block_int[index1][col2];
        //aprintf("index2: %d\n", index2);
        diskRead(diskID, index2, FileDisk->block_char[index2]);
        UINT16 index3 = FileDisk->block_int[index2][col3];
        //aprintf("index3: %d\n", index3);
        diskWritten(diskID, index3, writtenBuffer);
    }
    return 0;
}

int read_file(int inode, int logical_addr, char* writtenBuffer)
{
    //aprintf("the real len: %d\n", strlen(writtenBuffer));
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    int diskID = pcb_running->diskID;
    int cu_sector;
    //char name[14];
    cu_sector = pcb_running->CurrentSector;
    //aprintf("sector: %X\n", cu_sector);
    if (inode != pcb_running->iNode){
        return -1;
    }
    //aprintf("sector: %X\n", cu_sector);
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //strcpy(FileDisk->block_char[cu_sector], str);
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    //aprintf("index:  %X\n", index);
    if (level == 0x0600){
            //aprintf("we can see the devil\n");
        int col1 = logical_addr >> 6;
        int col2 = (logical_addr & 63) >> 3;
        int col3 = logical_addr & 7;
        diskRead(diskID, index, FileDisk->block_char[index]);
        UINT16 index1 = FileDisk->block_int[index][col1];
        //aprintf("index1: %d\n", index1);
        diskRead(diskID, index1, FileDisk->block_char[index1]);
        UINT16 index2 = FileDisk->block_int[index1][col2];
        //aprintf("index2: %d\n", index2);
        diskRead(diskID, index2, FileDisk->block_char[index2]);
        UINT16 index3 = FileDisk->block_int[index2][col3];
        //aprintf("index3: %d\n", index3);
        diskRead(diskID, index3, writtenBuffer);
        /*for (int j = 0; j < 8; j++){
            index_1 = FileDisk->block_int[index][j];
            diskRead(diskID, index_1, FileDisk->block_char[index_1]);
            for (int j_1 = 0; j_1 < 8; j_1++){
                index_2 = FileDisk->block_int[index_1][j_1];
                diskRead(diskID, index_2, FileDisk->block_char[index_2]);
                for (int j_2 = 0; j_2 < 8; j_2++){
                    index_3 = FileDisk->block_int[index_2][j_2];
                    diskRead(diskID, index_3, FileDisk->block_char[index_3]);
                    if (strcmp(FileDisk->block_char[index_3], "") == 0){

                    }
                }
            }
        }*/

    }
    return 0;
}
int close_file(int inode)
{
    PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
    doOnelck(2);
    pcb_running = QNextItemInfo(2);
    doOneUnlck(2);
    FILEDISK* FileDisk = (FILEDISK* )malloc(sizeof(FILEDISK));
    int diskID = pcb_running->diskID;
    int cu_sector;
    //char name[14];
    cu_sector = pcb_running->CurrentSector;
    if (inode != pcb_running->iNode){
        return -1;
    }
    //aprintf("sector: %X\n", cu_sector);
    diskRead(diskID, cu_sector, FileDisk->block_char[cu_sector]);
    //strcpy(FileDisk->block_char[cu_sector], str);
    UINT16 level = FileDisk->block_int[cu_sector][5] & 0x0600;
    UINT16 index = FileDisk->block_int[cu_sector][6];
    if (level == 0x0600){
        //aprintf("we can see the devil\n");
        int cnt = 0;
        for (int j = 0; j < 8; j++){
            diskRead(diskID, index, FileDisk->block_char[index]);
            int index_1 = FileDisk->block_int[index][j];
            diskRead(diskID, index_1, FileDisk->block_char[index_1]);
            for (int j_1 = 0; j_1 < 8; j_1++){
                int index_2 = FileDisk->block_int[index_1][j_1];
                diskRead(diskID, index_2, FileDisk->block_char[index_2]);
                for (int j_2 = 0; j_2 < 8; j_2++){
                    int index_3 = FileDisk->block_int[index_2][j_2];
                    diskRead(diskID, index_3, FileDisk->block_char[index_3]);
                            //aprintf("index3:             %d\n", index_3);
                    if (strcmp(FileDisk->block_char[index_3], "") == 0){
                        FileDisk->block_int[cu_sector][7] = cnt << 4;
                        diskWritten(diskID, cu_sector, FileDisk->block_char[cu_sector]);
                        pcb_running->CurrentSector = pcb_running->previousSector;
                        //CheckDisk1(diskID);
                        return 0;
                    }
                    cnt++;
                    //aprintf("cnt   :%d\n", cnt);
                }
            }
        }

    }
    return 0;
}


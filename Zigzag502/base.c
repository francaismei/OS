/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 4.51 August  2018: Minor bug fixes
 4.60 February2019: Test for the ability to alloc large amounts of memory
 ************************************************************************/
#include             "pcb.h"
#include             "filedisk.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include             "studentConfiguration.h"

//  This is a mapping of system call nmemonics with definitions

//PCB pcb_of_running_for_uniprocessor;
//int ok = 0;
char *call_names[] = {"MemRead  ", "MemWrite ", "ReadMod  ", "GetTime  ",
        "Sleep    ", "GetPid   ", "Create   ", "TermProc ", "Suspend  ",
        "Resume   ", "ChPrior  ", "Send     ", "Receive  ", "PhyDskRd ",
        "PhyDskWrt", "DefShArea", "Format   ", "CheckDisk", "OpenDir  ",
        "OpenFile ", "CreaDir  ", "CreaFile ", "ReadFile ", "WriteFile",
        "CloseFile", "DirContnt", "DelDirect", "DelFile  " };



/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the Operating System.
 NOTE WELL:  Just because the timer or the disk has interrupted, and
         therefore this code is executing, it does NOT mean the
         action you requested was successful.
         For instance, if you give the timer a NEGATIVE time - it
         doesn't know what to do.  It can only cause an interrupt
         here with an error.
         If you try to read a sector from a disk but that sector
         hasn't been written to, that disk will interrupt - the
         data isn't valid and it's telling you it was confused.
         YOU MUST READ THE ERROR STATUS ON THE INTERRUPT
 ************************************************************************/
void InterruptHandler(void) {
    INT32 DeviceID;
    INT32 Status;

    MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

    static BOOL  remove_this_from_your_interrupt_code = TRUE; /** TEMP **/
    static INT32 how_many_interrupt_entries = 0;              /** TEMP **/

    // Get cause of interrupt
    mmio.Mode = Z502GetInterruptInfo;
    mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
    MEM_READ(Z502InterruptDevice, &mmio);
    DeviceID = mmio.Field1;
    Status = mmio.Field2;
    //QPrint(5);
    //aprintf("%d %d\n ",DeviceID, Status);
    /*if (mmio.Field4 != ERR_SUCCESS) {
        aprintf( "The InterruptDevice call in the InterruptHandler has failed.\n");
        aprintf("The DeviceId and Status that were returned are not valid.\n");
    }*/
    // HAVE YOU CHECKED THAT THE INTERRUPTING DEVICE FINISHED WITHOUT ERROR?
    PCB *pcb_wakeup = (PCB* )malloc(sizeof(PCB));
    PCB *pcb_pointer = (PCB* )malloc(sizeof(PCB));

    PCB *pcb_min = (PCB* )malloc(sizeof(PCB));
    int Item;
    //aprintf("hahahahahaha\n");
    if (DeviceID == TIMER_INTERRUPT){
        //aprintf("hahahahahaha\n");
            if (Status){

                aprintf("possible err:   %d\n", Status);
                //exit(0);
            }
        //PCB pcb_wakeup_1;
        //QPrint(0);
        //pcb_wakeup_1 = removeProcess(0, pcb_wakeup);
        pcb_wakeup = removeProcess(0);
        pcb_min = minTimeQueue();

        //int setTime = GetSampleTimeNow();
        doOnelck(3);
        //aprintf("roc\n");
        for (Item = 0; ; Item++){
            pcb_pointer = QWalk(3, Item);
            if (pcb_pointer == (void*)-1){
                doOneUnlck(3);
                break;
            }
            if (pcb_pointer == pcb_wakeup){
                doOneUnlck(3);
                if (pcb_min != -1){
                    mmio.Mode = Z502Start;
                    mmio.Field1 = pcb_min->WaitingTime - GetSampleTimeNow();
                    if (mmio.Field1 <= 0)  mmio.Field1 = 1;
                    mmio.Field2 = mmio.Field3 = 0;
                    MEM_WRITE(Z502Timer, &mmio);
                }
                return;
            }
        }
        doOneUnlck(3);
        //aprintf("roc\n");
        addToQueue(1, pcb_wakeup);
        //check_state("roc", pcb_wakeup);
        //aprintf("roc\n");
        //ok = 1;
        if (pcb_min != (void*)-1){
            mmio.Mode = Z502Start;
            int setTime = pcb_min->WaitingTime - GetSampleTimeNow();
            while (setTime <= 0){
                //aprintf("DDDDDDDDDDD\n");
                //mmio.Field1 = 1;
                removeProcess2(0, pcb_min);
                addToQueue(1, pcb_min);
                pcb_min = minTimeQueue();
                if (pcb_min == (void*)-1){
                    return;
                }
                setTime = pcb_min->WaitingTime - GetSampleTimeNow();
            }
            //aprintf("confused???INTER:  %d\n", setTime);
            mmio.Field1 = setTime;
            mmio.Field2 = mmio.Field3 = 0;
            //if (mmio.Field1 > 0){
            MEM_WRITE(Z502Timer, &mmio);
            //}
        }
        /*else{
            QPrint(0);
            if (pcb_min!=(void*)-1) aprintf("FFFFFFFFFFF\n");
        }*/
    }
    for (Item = 0; Item < 8; Item++){
        if (DeviceID == DISK_INTERRUPT + Item){
            if (Status){
                CheckDisk1(Item);
                //aprintf("possible err:   %d\n", mmio.Field4);
                exit(0);
            }
            //aprintf("Device ID: %d\n", DeviceID);
            //doOnelck(5 + Item);
            //QPrint(5 + Item);
            //doOneUnlck(5 + Item);
            pcb_wakeup = removeProcess(5 + Item);

            //aprintf("wahahahahahaha  %s\n", pcb_wakeup->ProcessName);
            addToQueue(1, pcb_wakeup);
            //QPrint(1);
            //check_state(pcb_wakeup, "qwe");
            break;

            /*for (I = 0; ; I++){
                doOnelck(4 + Item);
                pcb_pointer = QWalk(4 + Item, I);
                doOneUnlck(4 + Item);
                if (pcb_pointer == (void*)-1){
                    break;
                }
                if (pcb_pointer->diskID == Item){
                    pcb_wakeup = removeProcess2(4 + Item, pcb_pointer);
                    addToQueue(1, pcb_wakeup);
                    //doOnelck(10);
                    //ok = 1;
                    //doOneUnlck(10);
                    return;
                }

            }*/
        }
    }
    //if (mmio.Field2)
    /** REMOVE THESE SIX LINES **/
    //how_many_interrupt_entries++; /** TEMP **/
    /*if (remove_this_from_your_interrupt_code && (how_many_interrupt_entries < 10)) {
        aprintf("InterruptHandler: Found device ID %d with status %d\n",
                DeviceID, Status);
    }*/
    //aprintf("interrupt end\n");
}           // End of InterruptHandler

/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
    INT32 DeviceID;
    INT32 Status;
    MEMORY_MAPPED_IO mmio;       // Enables communication with hardware
    short *PAGE_TBL_ADDR;
    static BOOL remove_this_from_your_fault_code = TRUE;
    static INT32 how_many_fault_entries = 0;

    // Get cause of fault
    mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
    mmio.Mode = Z502GetInterruptInfo;
    MEM_READ(Z502InterruptDevice, &mmio);
    DeviceID = mmio.Field1;
    Status   = mmio.Field2;
    if (DeviceID == INVALID_MEMORY){
        mmio.Mode = Z502GetPageTable;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Context, &mmio);
        PAGE_TBL_ADDR = (short *) mmio.Field1;
        PAGE_TBL_ADDR[Status] |= (PTBL_VALID_BIT) | Status;
        //PAGE_TBL_ADDR[Status]
    }

 /*   while  ( mmio.Field4 == ERR_SUCCESS ) {                  // We've found a valid interrupt
        //InterruptingDevice = mmio.Field1;
        // Do your interrupt handling work
         //

        mmio.Mode = Z502GetInterruptInfo;               // See if there's another Interrupt
        mmio.Field1 = mmio.Field2 = mmio.Field3  = mmio.Field4 = 0;
        MEM_READ( Z502InterruptDevice, &mmio );
}*/
    // This causes a print of the first few faults - and then stops printing!
    // You can adjust this as you wish.  BUT this code as written here gives
    // an indication of what's happening but then stops printing for long tests
    // thus limiting the output.
    how_many_fault_entries++;
    if (remove_this_from_your_fault_code && (how_many_fault_entries < 10)) {
            aprintf("FaultHandler: Found device ID %d with status %d\n",
                            (int) mmio.Field1, (int) mmio.Field2);
    }

} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
    short call_type;
    static short do_print = 10;
    short i;
    MEMORY_MAPPED_IO mmio;
    void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
    int Status;
    //printf("----------------------\n");
    //QPrint(0);
    int PID = 0;
    int QID;
    int t;

    PCB* pcb_ready_to_sleep = (PCB* )malloc(sizeof(PCB));

    char ProcessName_Test[MAX_PROCESS_NAME];

    call_type = (short) SystemCallData->SystemCallNumber;
    if (do_print > 0) {
        aprintf("SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
            //Value = (long)*SystemCallData->Argument[i];
            aprintf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
                    (unsigned long) SystemCallData->Argument[i],
                    (unsigned long) SystemCallData->Argument[i]);
        }
        do_print--;
    }
    switch(call_type){
    case SYSNUM_GET_TIME_OF_DAY:
        mmio.Mode = Z502ReturnValue;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Clock, &mmio);
        *(long *)SystemCallData->Argument[0] = mmio.Field1;
        break;
    case SYSNUM_TERMINATE_PROCESS:
        if (SystemCallData->Argument[0] == -2){
            mmio.Mode = Z502Action;
            mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
            MEM_WRITE(Z502Halt, &mmio);
        }
        else if (SystemCallData->Argument[0] == -1){
            /*doOnelck(2);
            PCB* pcb_terminate = (PCB* )malloc(sizeof(PCB));
            pcb_terminate = QWalk(2, 0);
            removeProcess(2, pcb_terminate);
            doOneUnlck(2);*/
            //check_state("asd", QNextItemInfo(2));
            //check_state("ter", QNextItemInfo(2));
            //doOnelck(1);QPrint(1);doOneUnlck(1);
            removeProcess(2);
            mmio.Mode = Z502Status;
            mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Timer, &mmio);
            Status = mmio.Field1;
            if (Status != DEVICE_IN_USE)
                printf("Got expected (DEVICE_FREE) result for Status of Timer\n");
            else
                printf("Timer is running\n");

            int Ca;
            //QPrint(0);QPrint(1);QPrint(2);
            for (Ca = 0; Ca < 13; Ca++){
                doOnelck(Ca);
                if (QWalk(Ca, 0) != (void*)-1){
                    doOneUnlck(Ca);
                    break;
                }
                doOneUnlck(Ca);
            }
            if (Ca == 13){
                printf("game over\n");
                mmio.Mode = Z502Action;
                mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
                MEM_WRITE(Z502Halt, &mmio);
            }
            /*if (processToRun() == (void*)-1){
                mmio.Mode = Z502Action;
                mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
                MEM_WRITE(Z502Halt, &mmio);
            }*/

            /*while (processToRun() == (void*)-1){
                CALL(DoASleep(0));
            }*/
            /*if (processToRun() == (void*)-1){
                aprintf("What a hell!\n");
            }
            else{
                aprintf("What a f****!\n");
            check_state("woo", processToRun());
            }*/
            //*(long *)SystemCallData->Argument[1] = ERR_SUCCESS;
            //else{
            PCB* pcb_oready = (PCB* )malloc(sizeof(PCB));

            while (processToRun() == (void*)-1){
                CALL(DoASleep(0));
            }
            QID = 1;

            /*if (processToRun() != (void*)-1){
                //PCB* pcb_oready = (PCB* )malloc(sizeof(PCB));
                QID = 1;
            }
            else{
                //aprintf("what can be done?\n");
                for (Ca = 5; Ca < 13; Ca++){
                    if (getTheHeadOfQueue(Ca) != (void*)-1){
                        QID = Ca;
                        break;
                    }
                }

                if (Ca == 13){
                    while (processToRun() == (void*)-1){
                        CALL(DoASleep(0));
                    }
                    QID = 1;
                }
            }


                /*while (ifItemExist(2, &t) && t!= 1){
                    CALL(DoASleep(0));
                }*/
                //doOnelck(1);
                //pcb_oready = ifItemExist(2, &t);
                //pcb_oready = removeProcess2(QID, pcb_oready);
                //pcb_oready = processToRun();
            pcb_oready = removeProcess(QID);
            //doOneUnlck(1);
            /*if (pcb_oready == (void*)-1){
                QID = 0;
                pcb_oready = removeProcess(QID);

                //doOneUnlck(0);
            }
            if (pcb_oready == (void*)-1){
                QID = 3;
                pcb_oready = removeProcess(QID);

                //doOneUnlck(0);
            }
            if (pcb_oready == (void*)-1){
                QID = 4;
                pcb_oready = removeProcess(QID);

                //doOneUnlck(0);
            }
            if (pcb_oready == (void*)-1){
                QID = 5;
                pcb_oready = removeProcess(QID);

                //doOneUnlck(0);
            }*/
            mmio.Mode = Z502StartContext;
            mmio.Field1 = (long)(pcb_oready->NewContext);
            mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
            //removeProcess(QID);
            addToQueue(2, pcb_oready);

            //removeProcess2(QID, pcb_oready);
            //check_state("woo", pcb_oready);
            MEM_WRITE(Z502Context, &mmio);
            if (mmio.Field4 != ERR_SUCCESS){
                    printf("Start Context has an error.\n");
                    exit(0);
            }
            //}

        }
        else{
            *(long *)SystemCallData->Argument[1] = terminateProcess(SystemCallData->Argument[0]);
        }
        break;
    case SYSNUM_GET_PROCESS_ID:
        //printf("%skkkkkkkkkkkkkk", SystemCallData->Argument[0]);
        //PID = getProcessID("");
        //QCreate("ProcessQueue");
        //int SucessCreate = osCreateProcess("");
        //QPrint(0);
        *(long *)SystemCallData->Argument[2] = getProcessID(SystemCallData->Argument[0],
                                                            SystemCallData->Argument[1]);
        /*if (strlen(SystemCallData->Argument[0]) <= MAX_PROCESS_NAME){
            strcpy(ProcessName_Test, (char*)SystemCallData->Argument[0]);
            if (strcmp(ProcessName_Test, "") == 0){
                //*(long *)SystemCallData->Argument[1] = pcb_of_running_for_uniprocessor.ProcessID;
                *(long *)SystemCallData->Argument[1] = ((PCB* )QWalk(2, 0))->ProcessID;
                *(long *)SystemCallData->Argument[2] = ERR_SUCCESS;
            }
            else{
                PID = getProcessID(ProcessName_Test);
                //printf("%d\n", PID);
                if (PID == 0){
                    aprintf("crazy!\n");
                    *(long *)SystemCallData->Argument[2] = -1;
                }
                else{

                    *(long *)SystemCallData->Argument[1] = PID;
                    *(long *)SystemCallData->Argument[2] = ERR_SUCCESS;
                }
            }
        }
        else{
            *(long *)SystemCallData->Argument[2] = -1;
        }*/
        break;
    case SYSNUM_CREATE_PROCESS:
        *(long *)SystemCallData->Argument[4] = osCreateProcess(SystemCallData->Argument[0],
                                                            SystemCallData->Argument[2],
                                                            SystemCallData->Argument[1],
                                                            SystemCallData->Argument[3]);
        break;
    case SYSNUM_SLEEP:

        /*
        mmio.Mode = Z502Action;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_WRITE(Z502Idle, &mmio);       //  Let the interrupt for this timer occur
        DoASleep(10);
        aprintf("asdasdasdasd\n");*/
        //PCB* pcb_ready_to_sleep = (PCB* )malloc(sizeof(PCB));
        doOnelck(2);
        pcb_ready_to_sleep = (PCB* )QWalk(2, 0);
        doOneUnlck(2);

        //equalTo(&pcb_ready_to_sleep, *(PCB* )QWalk(2, 0));
        mmio.Mode = Z502ReturnValue;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Clock, &mmio);

        pcb_ready_to_sleep->WaitingTime = mmio.Field1 + (long)SystemCallData->Argument[0];
//aprintf("TIME1:  %d\n", GetSampleTimeNow());
//aprintf("TIME2:  %d\n", mmio.Field1);
        //aprintf("%d  sssss%d    sssss%d\n", pcb_ready_to_sleep->WaitingTime, (long)SystemCallData->Argument[0], mmio.Field1);
        //pcb_of_running_for_uniprocessor.WaitingTime = GetSampleTimeNow() + SystemCallData->Argument[0];
        timerWaitingAdd(pcb_ready_to_sleep);
        //removeProcess(2);
        /*mmio.Mode = Z502Start;
        mmio.Field1 = SystemCallData->Argument[0];
        mmio.Field2 = mmio.Field3 = 0;
        MEM_WRITE(Z502Timer, &mmio);*/
        //aprintf("what\n");
        //removeProcess2(2, pcb_ready_to_sleep);
        /*
        PCB* pcb_on_ready = (PCB* )malloc(sizeof(PCB));
        doOnelck(1);
        pcb_on_ready = QWalk(1, 0);
        doOnelck(1);*/
        //equalTo(&pcb_on_ready, *(PCB* )QWalk(1, 0));
        /*mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long)pcb_on_ready->startingAddress;
        mmio.Field3 = (long)PageTable;
        mmio.Field4 = 0;
        MEM_WRITE(Z502Context, &mmio);
        pcb_on_ready->NewContext = mmio.Field1;
        mmio.Field2 = 0;*/

        //mmio = pcb_ready_to_sleep.NewContext
        //check_state(0, 1);
        Dispatcher();
        //equalTo(&pcb_of_running_for_uniprocessor, )
        /*mmio.Mode = Z502Status;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Timer, &mmio);
        Status = mmio.Field1;
        if (Status == 4)
            printf("Got expected result for Status of Timer\n");

        fflush(stdout);*/
        //InterruptHandler();
        break;
    case SYSNUM_PHYSICAL_DISK_WRITE:

        mmio.Mode = Z502Status;
        mmio.Field1 = SystemCallData->Argument[0];
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
            addToQueue(5 + (long)SystemCallData->Argument[0], pcb_disk);
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
            mmio.Field1 = SystemCallData->Argument[0];
            mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Disk, &mmio);
            if (mmio.Field2 != DEVICE_FREE){
                //aprintf("what hell?\n");
                pcb_disk = removeProcess(2);
                //pcb_disk->diskID = SystemCallData->Argument[0];
                //addToQueue(5, pcb_disk);
                pcb_wready = processToRun();
                addToQueue(2, pcb_wready);
                //removeProcess(1);
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
                    removeProcess2(5 + (long)SystemCallData->Argument[0], pcb_disk);
                }
            }
        }
        // Start the disk by writing a block of data


        mmio.Mode = Z502DiskWrite;
        mmio.Field1 = (long)SystemCallData->Argument[0];
        mmio.Field2 = (long)SystemCallData->Argument[1];
        mmio.Field3 = (long)SystemCallData->Argument[2];
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
        mmio.Field1 = SystemCallData->Argument[0];
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
        if (mmio.Field2 != DEVICE_FREE){
            doOnelck(2);
            pcb_disk = QNextItemInfo(2);
            doOneUnlck(2);
            //removeProcess(2);
            addToQueue(5 + (long)SystemCallData->Argument[0], pcb_disk);
            //check_state("wer", pcb_disk);
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
            mmio.Field1 = SystemCallData->Argument[0];
            mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Disk, &mmio);
            if (mmio.Field2 != DEVICE_FREE){
                //aprintf("what hell?\n");
                pcb_disk = removeProcess(2);
                //pcb_disk->diskID = SystemCallData->Argument[0];
                //addToQueue(5, pcb_disk);
                pcb_wready = processToRun();
                addToQueue(2, pcb_wready);
                //removeProcess(1);
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
                    check_state("wrong", pcb_disk);
                    removeProcess2(5 + (long)SystemCallData->Argument[0], pcb_disk);
                }
            }
        }
        //}
        /*mmio.Mode = Z502Action;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_WRITE(Z502Idle, &mmio);

        mmio.Field2 = DEVICE_IN_USE;
        while (mmio.Field2 != DEVICE_FREE) {
            mmio.Mode = Z502Status;
            mmio.Field1 = SystemCallData->Argument[0];
            mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Disk, &mmio);
        }*/
        break;
    case SYSNUM_PHYSICAL_DISK_READ:
        mmio.Mode = Z502Status;
        mmio.Field1 = SystemCallData->Argument[0];
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
        if (mmio.Field2 != DEVICE_FREE){
            doOnelck(2);
            pcb_disk = QNextItemInfo(2);
            doOneUnlck(2);
            //removeProcess(2);
            addToQueue(5 + (long)SystemCallData->Argument[0], pcb_disk);
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
            mmio.Field1 = SystemCallData->Argument[0];
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
                    removeProcess2(5 + (long)SystemCallData->Argument[0], pcb_disk);
                }
            }
        }


        mmio.Mode = Z502DiskRead;
        mmio.Field1 = (long)SystemCallData->Argument[0]; // Pick same disk location
        mmio.Field2 = (long)SystemCallData->Argument[1];
        mmio.Field3 = (long)SystemCallData->Argument[2];

        // Do the hardware call to read data from disk
        MEM_WRITE(Z502Disk, &mmio);

        mmio.Mode = Z502Status;
        mmio.Field1 = SystemCallData->Argument[0];
        mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Disk, &mmio);
        //printf("Disk Test 1: Time = %d ",  GetSampleTimeNow());
        if (mmio.Field2 != DEVICE_FREE){
            doOnelck(2);
            pcb_disk = QNextItemInfo(2);
            doOneUnlck(2);
            //removeProcess(2);
            addToQueue(5 + (long)SystemCallData->Argument[0], pcb_disk);
                //aprintf("what hell?\n");
            // Disk hasn't been used - should be free
            //doOnelck(10);  //critical variable ok
            while (processToRun() == (void*)-1){
                //CALL(DoASleep(1));
                CALL(DoASleep(0));
            }
            //aprintf("Have you survived?\n");
            //ok = 0;
            //doOneUnlck(10);
            //check_state("asd", processToRun());
            mmio.Mode = Z502Status;
            mmio.Field1 = SystemCallData->Argument[0];
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
                    removeProcess2(5 + (long)SystemCallData->Argument[0], pcb_disk);
                }
            }
        }
        break;
    case SYSNUM_CHECK_DISK:
        mmio.Mode = Z502CheckDisk;
        mmio.Field1 = SystemCallData->Argument[0];
        mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
        MEM_WRITE(Z502Disk, &mmio);
        //CheckDisk(SystemCallData->Argument[0], SystemCallData->Argument[1]);
        //SystemCallData->Argument[1]
        break;
    case SYSNUM_SUSPEND_PROCESS:
        *(long *)SystemCallData->Argument[1] = suspendProcess(SystemCallData->Argument[0]);

        break;
    case SYSNUM_RESUME_PROCESS:
        *(long *)SystemCallData->Argument[1] = resumeProcess(SystemCallData->Argument[0]);
        break;
    case SYSNUM_CHANGE_PRIORITY:
        *(long *)SystemCallData->Argument[2] = changePriority(SystemCallData->Argument[0],
                                                              SystemCallData->Argument[1]);
        break;
    case SYSNUM_SEND_MESSAGE:
        *(long *)SystemCallData->Argument[3] = sendMessage(SystemCallData->Argument[0],
                                                           SystemCallData->Argument[1],
                                                           SystemCallData->Argument[2]);
        messageDispatch(SystemCallData->Argument[0]);
        break;
    case SYSNUM_RECEIVE_MESSAGE:
        *(long *)SystemCallData->Argument[5] = receiveMessage(SystemCallData->Argument[0],
                                                              SystemCallData->Argument[1],
                                                              SystemCallData->Argument[2],
                                                              SystemCallData->Argument[3],
                                                              SystemCallData->Argument[4]);
        break;
    case SYSNUM_FORMAT:
        *(long *)SystemCallData->Argument[1] = diskFormat(SystemCallData->Argument[0]);
        break;
    case SYSNUM_OPEN_DIR:
        *(long *)SystemCallData->Argument[2] = open_dir(SystemCallData->Argument[0],
                                                        SystemCallData->Argument[1]);
        break;
    case SYSNUM_CREATE_DIR:
        *(long *)SystemCallData->Argument[1] = create_dir(SystemCallData->Argument[0]);
        break;
    case SYSNUM_CREATE_FILE:
        *(long *)SystemCallData->Argument[1] = create_file(SystemCallData->Argument[0]);
        break;
    case SYSNUM_OPEN_FILE:
        *(long *)SystemCallData->Argument[2] = open_file(SystemCallData->Argument[0],
                                                        SystemCallData->Argument[1]);
        break;
    case SYSNUM_CLOSE_FILE:
        *(long *)SystemCallData->Argument[1] = close_file(SystemCallData->Argument[0]);
        break;
    case SYSNUM_WRITE_FILE:
        *(long *)SystemCallData->Argument[3] = write_file(SystemCallData->Argument[0],
                                                          SystemCallData->Argument[1],
                                                          SystemCallData->Argument[2]);
        break;
    case SYSNUM_READ_FILE:
        *(long *)SystemCallData->Argument[3] = read_file(SystemCallData->Argument[0],
                                                         SystemCallData->Argument[1],
                                                         SystemCallData->Argument[2]);
        break;
    case SYSNUM_DIR_CONTENTS:
        break;
    default:
        printf("ERROR! call_type not recognized!\n");
        printf("Call_type is - %i\n", call_type);
    }//End of switch
}                                               // End of svc

/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
    // Every process will have a page table.  This will be used in
    // the second half of the project.
    void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
    INT32 i;
    MEMORY_MAPPED_IO mmio;

    QCreate("TimerQueue");
    QCreate("ReadyQueue");
    QCreate("RunningQueue");
    //PCB pcb;
    QCreate("SuspendQueue");

    QCreate("MessageQueue");
    QCreate("DiskQueue1");
    QCreate("DiskQueue2");
    QCreate("DiskQueue3");
    QCreate("DiskQueue4");
    QCreate("DiskQueue5");
    QCreate("DiskQueue6");
    QCreate("DiskQueue7");
    QCreate("DiskQueue8");

    QCreate("RecordMessage");
    // Demonstrates how calling arguments are passed thru to here
    //pcb.PageTable = PageTable;
    aprintf("Program called with %d arguments:", argc);
    for (i = 0; i < argc; i++)
        aprintf(" %s", argv[i]);
    aprintf("\n");
    aprintf("Calling with argument 'sample' executes the sample program.\n");

    // Here we check if a second argument is present on the command line.
    // If so, run in multiprocessor mode.  Note - sometimes people change
    // around where the "M" should go.  Allow for both possibilities


    if (argc > 2) {
        if ((strcmp(argv[1], "M") ==0) || (strcmp(argv[1], "m")==0)) {
            strcpy(argv[1], argv[2]);
            strcpy(argv[2],"M\0");
        }
        if ((strcmp(argv[2], "M") ==0) || (strcmp(argv[2], "m")==0)) {
            aprintf("Simulation is running as a MultProcessor\n\n");
            mmio.Mode = Z502SetProcessorNumber;
            mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
            mmio.Field2 = (long) 0;
            mmio.Field3 = (long) 0;
            mmio.Field4 = (long) 0;
            MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
        }
    } else {
        aprintf("Simulation is running as a UniProcessor\n");
        aprintf("Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
    }

    //  Some students have complained that their code is unable to allocate
    //  memory.  Who knows what's going on, other than the compiler has some
    //  wacky switch being used.  We try to allocate memory here and stop
    //  dead if we're unable to do so.
    //  We're allocating and freeing 8 Meg - that should be more than
    //  enough to see if it works.
    void *Temporary = (void *) calloc( 8, 1024 * 1024);
    if ( Temporary == NULL )  {  // Not allocated
	    printf( "Unable to allocate memory in osInit.  Terminating simulation\n");
	    exit(0);
    }
    free(Temporary);
    //  Determine if the switch was set, and if so go to demo routine.
    //  We do this by Initializing and Starting a context that contains
    //     the address where the new test is run.
    //  Look at this carefully - this is an example of how you will start
    //     all of the other tests.

    if ((argc > 1) && (strcmp(argv[1], "sample") == 0)) {
        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) SampleCode;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        MEM_WRITE(Z502Context, &mmio);     // Start up the context

    } // End of handler for sample code - This routine should never return here
    //  By default test0 runs if no arguments are given on the command line
    //  Creation and Switching of contexts should be done in a separate routine.
    //  This should be done by a "OsMakeProcess" routine, so that
    //  test0 runs on a process recognized by the operating system.

    if ((argc > 1) && strcmp(argv[1], "test0") == 0) {
        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test0;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        //osCreateProcess("qwe");
        //osCreateProcess("uiu");
        //getProcessID("qwe");
        //getProcessID("uiu");


        MEM_WRITE(Z502Context, &mmio);     // Start up the context
    }
    if ((argc > 1) && strcmp(argv[1], "test1") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test1;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test1"));
        QInsertOnTail(2, runningProcess("test1"));
        /*QCreate("ProcessQueue");
        int SucessCreate = osCreateProcess("");
        QPrint(0);
        if (SucessCreate)*/
        MEM_WRITE(Z502Context, &mmio);
        /*
        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test1;
        mmio.Field3 = (long) PageTable;
        mmio.Field4 = 0;
        MEM_WRITE(Z502Context, &mmio);
        if (mmio.Field4 != ERR_SUCCESS){
            printf("Start Context has an error.\n");
            exit(0);
        }
        //pcb.ProcessID = 2;
        //pcb.NewContext = mmio.Field1;                          // Store the context in an OS structure where YOU can find it
        mmio.Mode = Z502StartContext;                          // Set up to start the context
        mmio.Field2 = 0;
        MEM_WRITE(Z502Context, &mmio);                         // Start up the context
        if (mmio.Field4 != ERR_SUCCESS){                   // It's worth checking that your calls are successful.
            printf( "Start Context has an error\n");
            exit(0);
        }
        */
        //printf("asdasdasdasd\n");

        //if (SucessCreate)

    }
    if ((argc > 1) && strcmp(argv[1], "test2") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test2;
        mmio.Field3 = (long) PageTable;
        //printf("%dhahah\n", mmio.Field2);
        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        //printf("%djjj\n", mmio.Field1);
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
        pcb_running = runningProcess("test2");
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        QInsertOnTail(2, pcb_running);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test2"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        //printf("%d\n", pcb_of_running_for_uniprocessor.ProcessID);
        //printf("%slalala\n",pcb_of_running_for_uniprocessor.ProcessName);

        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test3") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test3;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
        pcb_running = runningProcess("test3");
        pcb_running->NewContext = mmio.Field1;
        QInsertOnTail(2, pcb_running);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test3"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test4") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test4;
        mmio.Field3 = (long) PageTable;
        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));
        pcb_running = runningProcess("test4");
        pcb_running->NewContext = mmio.Field1;
        pcb_running->startingAddress = (long) test4;

        QInsertOnTail(2, pcb_running);

        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test4"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test5") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test5;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test5");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test5;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test6") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test6;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test6");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test6;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test7") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test7;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test7");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test7;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test8") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test8;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test8");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test8;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test9") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test9;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test9");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test9;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test10") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test10;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test10");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test10;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test11") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test11;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test11");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test11;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test12") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test12;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test12");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test12;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test13") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test13;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test13");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test13;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test14") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test14;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test14");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test14;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test21") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test21;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test21");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test21;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

    if ((argc > 1) && strcmp(argv[1], "test22") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test22;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test22");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test22;
        //strcpy(pcb_running->CurrentDirectory, "root");
        pcb_running->CurrentSector = 0x11;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test23") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test23;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test23");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test23;
        //strcpy(pcb_running->CurrentDirectory, "root");
        pcb_running->CurrentSector = 0x11;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test24") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test24;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test24");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test24;
        //strcpy(pcb_running->CurrentDirectory, "root");
        pcb_running->CurrentSector = 0x11;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test25") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test25;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test25");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test25;
        //strcpy(pcb_running->CurrentDirectory, "root");
        pcb_running->CurrentSector = 0x11;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test26") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test26;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test26");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test26;
        //strcpy(pcb_running->CurrentDirectory, "root");
        pcb_running->CurrentSector = 0x11;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }
    if ((argc > 1) && strcmp(argv[1], "test41") == 0){

        mmio.Mode = Z502InitializeContext;
        mmio.Field1 = 0;
        mmio.Field2 = (long) test41;
        mmio.Field3 = (long) PageTable;

        MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence

        mmio.Mode = Z502StartContext;
        // Field1 contains the value of the context returned in the last call
        // Suspends this current thread
        mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
        PCB* pcb_running = (PCB* )malloc(sizeof(PCB));

        pcb_running = runningProcess("test41");
        //aprintf("%s\n",pcb_running->ProcessName);
        pcb_running->NewContext = mmio.Field1;
        pcb_running->PageTable = PageTable;
        pcb_running->startingAddress = test41;
        //aprintf("asdasdasda\n");
        QInsertOnTail(2, pcb_running);
        //QPrint(2);
        //equalTo(&pcb_of_running_for_uniprocessor, runningProcess("test5"));
        //pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
        MEM_WRITE(Z502Context, &mmio);
    }

}
                                           // End of osInit
void Dispatcher(void)
{
    MEMORY_MAPPED_IO mmio;
    //void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
    //int QID = 1;
    PCB* pcb_on_ready = (PCB* )malloc(sizeof(PCB));
    //PCB pcb_is_running = (PCB* )malloc(sizeof(PCB));

    /**************************************************************************
Do not consider priority first, just use FIFO, but warn that removing item uses priority first.

 **************************************************************************/
    //doOnelck(QID);
    //pcb_on_ready = processToRun();
    //doOneUnlck(QID);
            /**************************************************************************
Do not consider priority first, just use FIFO, but warn that removing item uses priority first.

 **************************************************************************/
    //printf("I have a dream\n");
    //QPrint(0);
    //while(!ok && pcb_on_ready == (void* )-1){
    while(processToRun() == (void*)-1){
        CALL(DoASleep(0));
        //doOnelck(QID);
        //pcb_on_ready = QWalk(QID, 0);
        //doOneUnlck(QID);
        /*mmio.Mode = Z502Status;
        mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
        MEM_READ(Z502Timer, &mmio);
        if (mmio.Field1 == DEVICE_FREE){
            InterruptHandler();
            break;
        }*/
        //CALL(DoASleep(10));
        //doOnelck(QID);
        //pcb_on_ready = QWalk(QID, 0);
        //doOneUnlck(QID);
    }
        /**************************************************************************
Do not consider priority first, just use FIFO, but warn that removing item uses priority first.

 **************************************************************************/
    //ok = 0;
//aprintf("heheh\n");
    pcb_on_ready = processToRun();
    if (pcb_on_ready == (void*)-1){
        aprintf("what happens?????\n");
        exit(0);
    }
//aprintf("hehehhhhhhhhhhhhhhhhhhhhh\n");
            /**************************************************************************
Do not consider priority first, just use FIFO, but warn that removing item uses priority first.

 **************************************************************************/
    //equalTo(&pcb_on_ready, *(PCB* )QWalk(QID, 0));
/*
    mmio.Mode = Z502InitializeContext;
    mmio.Field1 = 0;
    mmio.Field2 = (long)(pcb_on_ready->startingAddress);
    mmio.Field3 = (long)PageTable;
    mmio.Field4 = 0;

    MEM_WRITE(Z502Context, &mmio);
*/
    //pcb_is_running = QWalk(2, 0);
/*    mmio.Mode = Z502GetCurrentContext;
    mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
    MEM_READ(Z502Context, &mmio);*/
    mmio.Mode = Z502StartContext;
    mmio.Field1 = (long)(pcb_on_ready->NewContext);
    //pcb_on_ready->NewContext = mmio.Field1;
    //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
    mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
    //printf("asdasdaeeeeeeeeeee\n");
    //check_state("before", pcb_on_ready);
    removeProcess(2);
    removeProcess2(1, pcb_on_ready);
    //check_state("mid", pcb_on_ready);
    addToQueue(2, pcb_on_ready);
    //check_state("after", pcb_on_ready);

//printf("asdasdaeeeeeeeeeee\n");
    //removeProcess2(1, pcb_on_ready);
    //check_state("ru", pcb_on_ready);
//printf("asdasdaeeeeeeeeeee\n");
    MEM_WRITE(Z502Context, &mmio);
  //  aprintf("%dfffff\n",mmio.Field4);
    if (mmio.Field4 != ERR_SUCCESS){
        printf("Start Context has an error.\n");
        exit(0);
    }
    //printf("sdasdasdasd\n");
    /*
    for (QID = 0; QID < GetNumberOfAllocatedQueues(); QID++){
        if (strcmp(QGetName(QID), "ReadyQueue") == 0){
            break;
        }
    }*/
    /*QPrint(1);
    PCB* pcb_head = (PCB *)malloc(sizeof(PCB));
    while (QWalk(QID, 0) == (void* )-1){
        DoASleep(10);
    }
    pcb_head = QRemoveHead(QID);
    printf("%d\ntttttt", pcb_head->ProcessID);
    mmio.Mode = Z502InitializeContext;
    mmio.Field1 = 0;
    mmio.Field2 = (long) pcb_head->startingAddress;
    MEM_WRITE(Z502Context, &mmio);
    mmio.Mode = Z502StartContext;
    mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
    equalTo(&pcb_of_running_for_uniprocessor, *pcb_head);
    pcb_of_running_for_uniprocessor.NewContext = mmio.Field1;
    MEM_WRITE(Z502Context, &mmio);*/

}

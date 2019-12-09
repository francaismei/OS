#include "pcb.h"
#include "filedisk.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
#include             "studentConfiguration.h"



int ProcessAllocateID = PROCESS_INITIAL_ID + 1;

/*void equalTo(PCB* pcb_copy, PCB pcb)
{
    pcb_copy->ProcessID = pcb.ProcessID;
    strcpy(pcb_copy->ProcessName, pcb.ProcessName);
    pcb_copy->NewContext = pcb.NewContext;
    pcb_copy->priority = pcb.priority;
    pcb_copy->startingAddress = pcb.startingAddress;
    pcb_copy->WaitingTime = pcb.WaitingTime;
    pcb_copy->PageTable = pcb.PageTable;
}

int sendValue(int flag)
{
    return flag;
}*/
PCB* runningProcess(char* ProcessName)
{
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    strcpy(pcb->ProcessName, ProcessName);
    pcb->ProcessID = PROCESS_INITIAL_ID;
    pcb->priority = 10;
    return pcb;
}

void IncrementAllocatePID(void)
{
    ProcessAllocateID++;
}

int getProcessAllocateID(void)
{
    return ProcessAllocateID;
}

int osCreateProcess(char* ProcessName, int priority, void* startingAddress, int *ProcessID)
{
    MEMORY_MAPPED_IO mmio;
    void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
    if (strlen(ProcessName) > MAX_PROCESS_NAME){
        return -1;
    }
    if (priority == -3){
        return -1;
    }
    int QID;
    for (QID = 0; QID < GetNumberOfAllocatedQueues(); QID++){
        if (strcmp(QGetName(QID), "ReadyQueue") == 0){
            break;
        }
    }
    if (QID == GetNumberOfAllocatedQueues()){
        QCreate("ReadyQueue");
    }
    int Item;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    doOnelck(QID);

    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        if (strcmp(pcb_pointer->ProcessName, ProcessName) == 0){
            doOneUnlck(QID);
            return -1;
        }
    }
    doOneUnlck(QID);
    if (Item > MAX_PROCESS_NUMBER)  return -1;
    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    strcpy(pcb->ProcessName, ProcessName);
    pcb->priority = priority;
    pcb->startingAddress = startingAddress;
    //pcb->CurrentSector = 0x11;
//    QPrint(QID);
    /*if (strcmp(ProcessName, "") == 0){

        pcb->ProcessID = PROCESS_INITIAL_ID;
        QInsertOnTail(QID, pcb);
    }*/
    //else{

    mmio.Mode = Z502InitializeContext;
    mmio.Field1 = 0;
    mmio.Field2 = (long)(pcb->startingAddress);
    mmio.Field3 = (long)PageTable;
    mmio.Field4 = 0;
    //aprintf("fffff\n");
    MEM_WRITE(Z502Context, &mmio);
    //aprintf("fffff\n");
    if ( mmio.Field4 != ERR_SUCCESS )  {                   // It's worth checking that your calls are successful.
        printf( "Initialize Context has an error\n");
        exit(0);
    }
    //aprintf("create context1 : %d\n", mmio.Field1);
    pcb->NewContext = mmio.Field1;
    //aprintf("create context2 : %d\n", pcb->NewContext);
    pcb->CurrentSector = 0x11;
    pcb->PageTable = PageTable;
    pcb->ProcessID = getProcessAllocateID();
    *ProcessID = pcb->ProcessID;
    IncrementAllocatePID();

    doOnelck(QID);

    /*for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        /*if (pcb_pointer->priority < pcb->priority){
            break;
        }
    }

    QInsert(QID, Item, pcb);*/
    QInsertOnTail(QID, pcb);
    //QPrint(QID);
    doOneUnlck(QID);
    //QInsertOnTail(QID, pcb);
    /*if (QItemExists(QID, pcb) == (void *)-1){
        QInsertOnTail(QID, pcb);
        QPrint(QID);
    }
    else{
        return -1;
    }*/
    //}
    //free(pcb);

    //MEM_WRITE(Z502GetProcessorID, &mmio);
    //PCB* pcb = (PCB*)malloc(sizeof(PCB));

    //mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
    //MEM_WRITE(Z502Context, &mmio);
    check_state("Create", pcb);
    return 0;
}



int getProcessID(char* ProcessName, int *PID)
{
    //QPrint(0);
    if (strlen(ProcessName) > MAX_PROCESS_NAME){
        return -1;
    }
    if (strcmp(ProcessName, "") == 0){
        doOnelck(2);
        *PID = ((PCB *)QNextItemInfo(2))->ProcessID;
        doOneUnlck(2);
        return ERR_SUCCESS;
    }
    //int PID = -1;
    int QID;
    int Item;

    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    /*for (QID = 0; QID < 13; QID++){
        doOnelck(QID);
    }*/

    for (QID = 0; QID < 13; QID++){
        doOnelck(QID);
        for (Item = 0; ; Item++){
            //doOnelck(QID);
            pcb = (PCB* )QWalk(QID, Item);
            //doOneUnlck(QID);
            //QPrint(QID);
            if (pcb == (void *)-1)  break;
            if (strcmp(pcb->ProcessName, ProcessName) == 0){
                *PID = pcb->ProcessID;
                for (int j = 0; j <= QID; j++){
                    doOneUnlck(j);
                }
                //aprintf("okkkkk?\n");
                return ERR_SUCCESS;
            }
        }
    }

    for (int j = 0; j < QID; j++){
        doOneUnlck(j);
    }
    /*aprintf("Really nothing?    %s\n", ProcessName);
    doOnelck(0);
    QPrint(0);
    doOneUnlck(0);
    doOnelck(1);
    QPrint(1);
    doOneUnlck(1);
    doOnelck(2);
    QPrint(2);
    doOneUnlck(2);
    doOnelck(7);
    QPrint(7);
    doOneUnlck(7);*/
    return -1;
}

int terminateProcess(int ProcessID)
{
    int QID;
    for (QID = 0; QID < GetNumberOfAllocatedQueues(); QID++){
        if (strcmp(QGetName(QID), "ReadyQueue") == 0){
            break;
        }
    }
    int Item;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    doOnelck(QID);
    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        if (pcb_pointer->ProcessID == ProcessID){
            QRemoveItem(QID, pcb_pointer);
            break;
        }
    }
    doOneUnlck(QID);
    return 0;
}
int timerWaitingAdd(PCB* pcb)
{
    MEMORY_MAPPED_IO mmio;
    /*int QID;
    for (QID = 0; QID < GetNumberOfAllocatedQueues(); QID++){
        if (strcmp(QGetName(QID), "TimerQueue") == 0){
            break;
        }
    }
    if (QID == GetNumberOfAllocatedQueues()){
        QCreate("TimerQueue");
    }
    //int Item;
    //PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    //PCB* pcb_pointer_1 = (PCB* )malloc(sizeof(PCB));
    doOnelck(QID);*/
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    int QID = 0;
    //aprintf("confused:  %d\n", pcb->WaitingTime);
    //aprintf("TIME3:  %d\n", GetSampleTimeNow());

    int setTime = pcb->WaitingTime - GetSampleTimeNow();
    if (setTime <= 0){
        aprintf("Possible\n");
        addToQueue(1, pcb);
    }
    else{
        pcb_pointer = minTimeQueue();
        //addToQueue(0, pcb);
        //check_state("f***", pcb);
        addToQueue(QID, pcb);
        //check_state("f**", pcb);
        if (pcb_pointer == (void*)-1 || pcb_pointer->WaitingTime > pcb->WaitingTime){
            mmio.Mode = Z502Start;
            //mmio.Field1 = pcb->WaitingTime - GetSampleTimeNow();
            mmio.Field1 = setTime;
            mmio.Field2 = mmio.Field3 = 0;
            //aprintf("confused????:  %d\n", setTime);
            MEM_WRITE(Z502Timer, &mmio);
            /*if (mmio.Field1 > 0){
                MEM_WRITE(Z502Timer, &mmio);
            }
            else{
                addToQueue(1, pcb);
            }*/

        }

    }
    //QInsertOnTail(QID, pcb);
    //QInsert(QID, Item + 1, pcb_pointer_1);
    //QInsertOnTail(QID, pcb_pointer_1);
    //QPrint(QID);

    //doOneUnlck(QID);
    return 0;
}

PCB* processToRun(void)
{
    int QID = 1;
    doOnelck(QID);
    if (QWalk(QID, 0) == (void* )-1){
        doOneUnlck(QID);
        return (void*)-1;
    }
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    int minn, Item;
    pcb = QNextItemInfo(QID);
    minn = pcb->priority;
    for (Item = 1; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        if (pcb_pointer->priority < minn){
            minn = pcb_pointer->priority;
            pcb = pcb_pointer;
        }
    }
    doOneUnlck(QID);
    return pcb;
}
/*PCB* timerWaitingDelete(void)
{
    int QID;
    for (QID = 0; QID < GetNumberOfAllocatedQueues(); QID++){
        if (strcmp(QGetName(QID), "TimerQueue") == 0){
            break;
        }
    }
    //int Item, flag = 0;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    pcb_pointer = QWalk(QID, 0);
    doOnelck(QID);
    QRemoveHead(QID);
    doOneUnlck(QID);
    return pcb_pointer;
    //return 0;
}*/

int addToQueue(int QID, PCB* pcb)
{
    //int QID = 1;
    doOnelck(QID);
//    PCB *pcb_pointer = (PCB* )malloc(sizeof(PCB));
/*
    int Item;
    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        /*if (pcb_pointer->priority < pcb->priority){
            break;
        }
    }*/
    QInsertOnTail(QID, pcb);
    //QInsert(QID, Item, pcb);
    doOneUnlck(QID);
}

/*int timerWaitingDelete(char* ProcessName)
{
    if (strlen(ProcessName) > MAX_PROCESS_NAME){
        return 0;
    }
    int QID;
    for (QID = 0; QID < GetNumberOfAllocatedQueues(); QID++){
        if (strcmp(QGetName(QID), "TimerQueue") == 0){
            break;
        }
    }
    int Item, flag = 0;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        if (strcmp(pcb_pointer->ProcessName, ProcessName) == 0){
            QRemoveItem(QID, pcb_pointer);
            flag = 1;
            break;
        }
    }
    if (!flag)  return 0;
    return 1;
}*/
PCB* removeProcess(int QID)
{
    //int QID = 2;
    doOnelck(QID);
    if (QNextItemInfo(QID) == (void*)-1)  return (void*)-1;
    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    int minn, Item;
    if (QID != 0 && QID != 1){
        pcb = QRemoveHead(QID);
        doOneUnlck(QID);
        return pcb;
    }
    pcb = QNextItemInfo(QID);
    if (QID == 0)  minn = pcb->WaitingTime;
    else  minn = pcb->priority;
    for (Item = 1; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1)  break;
        if (QID == 0 && pcb_pointer->WaitingTime < minn){
            minn = pcb_pointer->WaitingTime;
            pcb = pcb_pointer;
        }
        if (QID == 1 && pcb_pointer->priority < minn){
            minn = pcb_pointer->priority;
            pcb = pcb_pointer;
        }
    }
    QRemoveItem(QID, pcb);
    doOneUnlck(QID);
    return pcb;
    /*PCB pcb_copy;
    equalTo(&pcb_copy, *pcb);
    //QPrint(QID);
    PCB* pcb_test = (PCB* )malloc(sizeof(PCB));

    pcb_test = QRemoveItem(QID, pcb);
    doOneUnlck(QID);
    return pcb_test;
    //return pcb_copy;*/
}

PCB* removeProcess2(int QID, PCB *pcb)
{
    //int QID = 2;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    doOnelck(QID);
    pcb_pointer = QItemExists(QID, pcb);
    if (pcb_pointer == (void* )-1){
        //aprintf("Item not exist! %d\n", QID);
        return (void*)-1;
    }

    QRemoveItem(QID, pcb);

    /*pcb_pointer = QItemExists(QID, pcb);
    if (pcb_pointer == (void* )-1){
        //aprintf("Should be like this\n");

    }
    else  aprintf("fuccccccccccccc\n");*/
    doOneUnlck(QID);
    return pcb;
    //return pcb_copy;
}

PCB* minTimeQueue(void)
{
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    int Item;
    int QID = 0;

    doOnelck(QID);
    if (QWalk(QID, 0) == (void* )-1){
        doOneUnlck(QID);
        return (void*)-1;
    }
    pcb = QNextItemInfo(0);
    int minn = pcb->WaitingTime;
    for (Item = 1; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1){
            break;
        }
        if (minn > pcb_pointer->WaitingTime){
            minn = pcb_pointer->WaitingTime;
            pcb = pcb_pointer;
        }
    }
    doOneUnlck(QID);
    return pcb;
}

int suspendProcess(int pid)
{
    int t;
    if (pid == -1 || ifItemExist(pid, &t) == (void*)-1){
        return -1;
    }
    int Item, QID = 1;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));

    //for (QID = 0; QID < 3; QID++){
    doOnelck(QID);
    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1){
            doOneUnlck(QID);
            break;
        }
        if (pcb_pointer->ProcessID == pid){
            addToQueue(3, pcb_pointer);
            removeProcess2(QID, pcb_pointer);
            doOneUnlck(QID);
            check_state("suspend", pcb_pointer);
            return ERR_SUCCESS;
        }
    }
    QID = 0;
    doOnelck(QID);
    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1){
            doOneUnlck(QID);
            break;
        }
        if (pcb_pointer->ProcessID == pid){
            addToQueue(3, pcb_pointer);
            removeProcess2(QID, pcb_pointer);
            doOneUnlck(QID);
            check_state("suspend", pcb_pointer);
            return ERR_SUCCESS;
        }
    }
    return -1;

}

int resumeProcess(int pid)
{
    int t;
    if (pid == -1 || ifItemExist(pid, &t) == (void*)-1){
        return -1;
    }
    int Item, QID = 3;
    PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));

    //for (QID = 0; QID < 3; QID++){
    doOnelck(QID);
    for (Item = 0; ; Item++){
        pcb_pointer = QWalk(QID, Item);
        if (pcb_pointer == (void* )-1){
            doOneUnlck(QID);
            break;
        }
        if (pcb_pointer->ProcessID == pid){
            addToQueue(1, pcb_pointer);
            removeProcess2(QID, pcb_pointer);
            doOneUnlck(QID);
            check_state("resume", pcb_pointer);
            return ERR_SUCCESS;
        }
    }
    return -1;

}

int numberOFItemOnQueue(int QID)
{
    int Item;
    //PCB* pcb_pointer = (PCB* )malloc(sizeof(PCB));
    doOnelck(QID);
    for (Item = 0; ; Item++){
        //pcb_pointer = QWalk(QID, Item);
        if (QWalk(QID, Item) == (void* )-1){
            break;
        }
    }
    doOneUnlck(QID);
    return Item;

}

PCB* getTheHeadOfQueue(int QID)
{
    doOnelck(QID);
    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    pcb = QNextItemInfo(QID);
    doOneUnlck(QID);
    return pcb;
}
/*int numberOfProcess(void)
{
    int i, s = 0;
    for (i = 0; i < 6; i++){
        s += numberOFItemOnQueue(i);
    }
    //aprintf("what a hell!!!\n%d", s);
    return s;
}*/
PCB* ifItemExist(int pid, int* k)
{
    int i, Item;
    PCB *pcb = (PCB* )malloc(sizeof(PCB));
    for (i = 0; i < 13; i++){
        doOnelck(i);
        for (Item = 0; ; Item++){
            pcb = QWalk(i, Item);
            if (pcb == (void*)-1)  break;
            if (pcb->ProcessID == pid){
                k = i;
                doOneUnlck(i);
                return pcb;
            }
        }
        doOneUnlck(i);
    }
    return (void*)-1;
}

int changePriority(int pid, int new_priority)
{
    int t;
    if (pid == -1){
        doOnelck(2);
        ((PCB* )QWalk(2, 0))->priority = new_priority;
        doOneUnlck(2);
        check_state("PRIORITY", (PCB* )QNextItemInfo(2));
        return ERR_SUCCESS;
    }
    PCB* pcb = (PCB* )malloc(sizeof(PCB));
    pcb = ifItemExist(pid, &t);
    if (pcb == (void*)-1 || new_priority < 0){
        return -1;
    }
    pcb->priority = new_priority;
    check_state("PRIORITY", pcb);
    return ERR_SUCCESS;

}
int sendMessage(int targetPID, char* messageBuffer, int messageSendLength)
{
    int t;
    int QID = 13;
    if (ifItemExist(targetPID, &t) == (void*)-1 && targetPID != -1){
        return -1;
    }
    if (numberOFItemOnQueue(QID) > 12){
        return -1;
    }
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    if (strlen(messageBuffer) <= messageSendLength && messageSendLength <= LEGAL_MESSAGE_LENGTH){
        Message* ms = (Message*)malloc(sizeof(Message));
        ms->targetPID = targetPID;
        doOnelck(2);
        pcb = QWalk(2, 0);
        ms->sourcePID = pcb->ProcessID;
        doOneUnlck(2);
        ms->sendLength = messageSendLength;


        strcpy(ms->buffer, messageBuffer);
        doOnelck(13);
        QInsertOnTail(QID, ms);
        doOneUnlck(13);
        return ERR_SUCCESS;
        /*if (pcb->ProcessID == targetPID){
            QPrint2(6);
            return ERR_SUCCESS;
        }

            return ERR_SUCCESS;
        }*/
    }
    return -1;
}

void messageDispatch(int targetPID)
{
    int Item;
    PCB* pcb = (PCB*)malloc(sizeof(PCB));
    MEMORY_MAPPED_IO mmio;
    doOnelck(2);
    pcb = QNextItemInfo(2);
    doOneUnlck(2);
    doOnelck(4);
    PCB* pcb_pointer = (PCB*)malloc(sizeof(PCB));
    if (targetPID == -1){
        pcb_pointer = QWalk(4, 0);
        if (pcb_pointer != (void*)-1){
            doOneUnlck(4);
            //removeProcess2(2, pcb);
            removeProcess(2);
            addToQueue(1, pcb);
            mmio.Mode = Z502StartContext;
            mmio.Field1 = (long)(pcb_pointer->NewContext);
            //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
            mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
            //printf("asdasdaeeeeeeeeeee\n");
            addToQueue(2, pcb_pointer);
            //printf("asdasdaeeeeeeeeeee\n");
            removeProcess2(4, pcb_pointer);
            check_state("run", pcb_pointer);
            //printf("asdasdaeeeeeeeeeee\n");
            MEM_WRITE(Z502Context, &mmio);
            //  aprintf("%dfffff\n",mmio.Field4);
            if (mmio.Field4 != ERR_SUCCESS){
                printf("Start Context has an error.\n");
                exit(0);
            }
        }
        else{
            doOneUnlck(4);
        }

    }
    else{
        //QPrint(4);
        for (Item = 0; ; Item++){
            pcb_pointer = QWalk(4, Item);
            if (pcb_pointer == (void*)-1){
                //aprintf("that is impossible  %d\n", Item);
                break;
            }
            //aprintf("message Dis %d   %d\n", pcb_pointer->ProcessID, targetPID);
            if (pcb_pointer->ProcessID == targetPID){
                doOneUnlck(4);
                removeProcess(2);
                mmio.Mode = Z502StartContext;
                mmio.Field1 = (long)(pcb_pointer->NewContext);
                //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
                mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
                //printf("asdasdaeeeeeeeeeee\n");
                addToQueue(2, pcb_pointer);
                //printf("asdasdaeeeeeeeeeee\n");
                removeProcess2(4, pcb_pointer);
                check_state("run", pcb_pointer);
                //printf("asdasdaeeeeeeeeeee\n");
                MEM_WRITE(Z502Context, &mmio);
                //  aprintf("%dfffff\n",mmio.Field4);
                if (mmio.Field4 != ERR_SUCCESS){
                    printf("Start Context has an error.\n");
                    exit(0);
                }
            }
        }
        doOneUnlck(4);
    }
}
int receiveMessage(int sourcePID, char* messageBuffer, int receiveLength, int* sendLength, int* realSend)
{
    int t;
    MEMORY_MAPPED_IO mmio;
    while(1){
        if (ifItemExist(sourcePID, &t) == (void*)-1 && sourcePID != -1 || receiveLength > LEGAL_MESSAGE_LENGTH){
            return -1;
        }
        PCB* pcb = (PCB* )malloc(sizeof(PCB));
        doOnelck(2);
        //pcb = QWalk(2, 0);
        pcb = QNextItemInfo(2);
        //aprintf("initial%d\n", pcb->ProcessID);
        int target = pcb->ProcessID;
        //aprintf("initialtar%d\n", target);
        doOneUnlck(2);

        int QID = 13;
        Message* ms_pointer = (Message* )malloc(sizeof(Message));
    //aprintf("%d   %d\n", sourcePID, target);
        int Item;
        doOnelck(QID);
        //QPrint2(6);
        if (sourcePID != -1){
        //QPrint2(6);
        //printf("source != -1?\n");
            for (Item = 0; ; Item++){
                ms_pointer = QWalk(QID, Item);
                if (ms_pointer == (void* )-1){
                    break;
                }
                if (ms_pointer->sourcePID == sourcePID &&
                    (ms_pointer->targetPID == target || ms_pointer->targetPID == -1)){
                    strcpy(messageBuffer, ms_pointer->buffer);
                    if (strlen(messageBuffer) > receiveLength){
                        doOneUnlck(QID);
                        return -1;
                    }
                    *sendLength = ms_pointer->sendLength;
                    QRemoveItem(QID, ms_pointer);
                    doOneUnlck(QID);

                    return ERR_SUCCESS;
                }
            }
            if (ms_pointer == (void* )-1){
                doOneUnlck(QID);
                //QID = 2;
                //doOnelck(QID);
                addToQueue(4, pcb);
                removeProcess2(2, pcb);
                PCB* pcb_source = (PCB* )malloc(sizeof(PCB));
                pcb_source = ifItemExist(pcb_source, &t);
                if (t != 1){
                    aprintf("No process on the ready Q!\n");
                    exit(0);
                }
                //pcb_source = QItemExists(1, ifItemExist(sourcePID));
                //doOneUnlck(1);
                //pcb_source = removeProcess2(1, ifItemExist(sourcePID));
                removeProcess2(1, pcb_source);
                /*if (pcb_source == (void*)-1){
                    aprintf("No process on the ready Q!\n");
                    exit(0);
                }*/
                addToQueue(2, pcb_source);
                //doOneUnlck(4);
                mmio.Mode = Z502StartContext;
                mmio.Field1 = (long)(pcb_source->NewContext);
                //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
                mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
                //printf("asdasdaeeeeeeeeeee\n");
                check_state("run", pcb_source);
                //printf("asdasdaeeeeeeeeeee\n");
                MEM_WRITE(Z502Context, &mmio);
                //  aprintf("%dfffff\n",mmio.Field4);
                if (mmio.Field4 != ERR_SUCCESS){
                    printf("Start Context has an error.\n");
                    exit(0);
                }
            }
        }
        else{
            for (Item = 0; ; Item++){
                ms_pointer = QWalk(QID, Item);
                if(ms_pointer == (void*)-1){
                    break;
                }
                //aprintf("%d   %d\n", ms_pointer->targetPID, target);
                if (ms_pointer->targetPID == target || ms_pointer->targetPID == -1){
                    strcpy(messageBuffer, ms_pointer->buffer);
                    if (strlen(messageBuffer) > receiveLength){
                        doOneUnlck(QID);
                        return -1;
                    }
                    *sendLength = ms_pointer->sendLength;
                    *realSend = ms_pointer->sourcePID;
                    QRemoveItem(QID, ms_pointer);
                    doOneUnlck(QID);
                    return ERR_SUCCESS;
                }
            }
            if (ms_pointer == (void*)-1){

                doOneUnlck(QID);
                //QID = 2;
                //doOnelck(QID);
                addToQueue(4, pcb);
                removeProcess2(2, pcb);

                //doOneUnlck(4);
                PCB* pcb_source = (PCB* )malloc(sizeof(PCB));
                /*doOnelck(1);
                pcb_source = QNextItemInfo(1);
                doOneUnlck(1);
                if (pcb_source != (void*)-1){
                    removeProcess2(1, pcb_source);
                }
                else{
                    doOnelck(3);
                    pcb_source = QNextItemInfo(1);
                    doOneUnlck(3);
                }*/
                pcb_source = removeProcess(1);
                addToQueue(2, pcb_source);
                mmio.Mode = Z502StartContext;
                mmio.Field1 = (long)(pcb_source->NewContext);
                //mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
                mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
                //aprintf("%d    here\n", pcb_source->ProcessID);

                //printf("asdasdaeeeeeeeeeee\n");
                check_state("run", pcb_source);
                //aprintf("asdasdaeeeeeeeeeee\n");
                MEM_WRITE(Z502Context, &mmio);
                //  aprintf("%dfffff\n",mmio.Field4);
                if (mmio.Field4 != ERR_SUCCESS){
                    printf("Start Context has an error.\n");
                    exit(0);
                }

            }
        }
    }


}



void doOnelck(int QID)
{
    INT32 LockResult;
    READ_MODIFY(MEMORY_INTERLOCK_BASE + QID, DO_LOCK, SUSPEND_UNTIL_LOCKED,
                &LockResult);
}

void doOneUnlck(int QID)
{
    INT32 LockResult;
    READ_MODIFY(MEMORY_INTERLOCK_BASE + QID, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
                &LockResult);
}

typedef struct
{
    int ID;
    int priority;
}pair;

void check_state(char* action, PCB* target_process)
{
    pair ready[50];
    SP_INPUT_DATA SPData;
    memset(&SPData, 0, sizeof(SP_INPUT_DATA));

	strcpy(SPData.TargetAction, action);

	doOnelck(2);
	SPData.CurrentlyRunningPID = ((PCB*)QWalk(2, 0) == ((void*) -1))? 0: ((PCB*)QWalk(2, 0))->ProcessID;
	doOneUnlck(2);
	//aprintf("1 is good\n");
	SPData.TargetPID = target_process->ProcessID;
	// The NumberOfRunningProcesses as used here is for a future implementation
	// when we are running multiple processors.  For right now, set this to 0
	// so it won't be printed out.
	SPData.NumberOfRunningProcesses = 0;
    int i, j, k;
    pair t;

    doOnelck(1);
	SPData.NumberOfReadyProcesses = numberOFItemOnQueue(1);
	   // Processes ready to run

	for (i = 0; i < SPData.NumberOfReadyProcesses; i++) {
		//SPData.ReadyProcessPIDs[i] = ((PCB*)QWalk(1, i))->ProcessID;

        ready[i].priority = ((PCB*)QWalk(1, i))->priority;
        ready[i].ID = ((PCB*)QWalk(1, i))->ProcessID;
    }
    for (i = 1; i < SPData.NumberOfReadyProcesses; i++){
        for (j = i - 1; j >= 0; j--){
            if (ready[j].priority <= ready[i].priority)
                break;
        }
        k = j;
        t.priority = ready[i].priority;
        t.ID = ready[i].ID;
        for (j = i - 1; j > k; j--){
            ready[j + 1].priority = ready[j].priority;
            ready[j + 1].ID = ready[j].ID;
        }
        ready[j + 1].priority = t.priority;
        ready[j + 1].ID = t.ID;
    }
    for (i = 0; i < SPData.NumberOfReadyProcesses; i++){
        SPData.ReadyProcessPIDs[i] = ready[i].ID;
    }
    doOneUnlck(1);
//aprintf("1 is also good\n");
    doOnelck(3);
	SPData.NumberOfProcSuspendedProcesses = numberOFItemOnQueue(3);
	for (i = 0; i < SPData.NumberOfProcSuspendedProcesses; i++) {
		SPData.ProcSuspendedProcessPIDs[i] = ((PCB*)QWalk(3, i))->ProcessID;
	}
	doOneUnlck(3);

	doOnelck(4);
	SPData.NumberOfMessageSuspendedProcesses = numberOFItemOnQueue(4);
	for (i = 0; i < SPData.NumberOfMessageSuspendedProcesses; i++) {
		SPData.MessageSuspendedProcessPIDs[i] = ((PCB*)QWalk(4, i))->ProcessID;
	}
	doOneUnlck(4);

	doOnelck(0);
	SPData.NumberOfTimerSuspendedProcesses = numberOFItemOnQueue(0);
	for (i = 0; i < SPData.NumberOfTimerSuspendedProcesses; i++) {
		SPData.TimerSuspendedProcessPIDs[i] = ((PCB*)QWalk(0, i))->ProcessID;
	}
	doOneUnlck(0);

	doOnelck(5);
	SPData.NumberOfDiskSuspendedProcesses1 = numberOFItemOnQueue(5);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses1; i++) {
		SPData.DiskSuspendedProcessPIDs1[i] = ((PCB*)QWalk(5, i))->ProcessID;
	}
	doOneUnlck(5);

	doOnelck(6);
	SPData.NumberOfDiskSuspendedProcesses2 = numberOFItemOnQueue(6);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses2; i++) {
		SPData.DiskSuspendedProcessPIDs2[i] = ((PCB*)QWalk(6, i))->ProcessID;
	}
	doOneUnlck(6);

	doOnelck(7);
	SPData.NumberOfDiskSuspendedProcesses3 = numberOFItemOnQueue(7);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses3; i++) {
		SPData.DiskSuspendedProcessPIDs3[i] = ((PCB*)QWalk(7, i))->ProcessID;
	}
	doOneUnlck(7);

	doOnelck(8);
	SPData.NumberOfDiskSuspendedProcesses4 = numberOFItemOnQueue(8);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses4; i++) {
		SPData.DiskSuspendedProcessPIDs4[i] = ((PCB*)QWalk(8, i))->ProcessID;
	}
	doOneUnlck(8);

	doOnelck(9);
	SPData.NumberOfDiskSuspendedProcesses5 = numberOFItemOnQueue(9);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses5; i++) {
		SPData.DiskSuspendedProcessPIDs5[i] = ((PCB*)QWalk(9, i))->ProcessID;
	}
	doOneUnlck(9);

	doOnelck(10);
	SPData.NumberOfDiskSuspendedProcesses6 = numberOFItemOnQueue(10);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses6; i++) {
		SPData.DiskSuspendedProcessPIDs6[i] = ((PCB*)QWalk(10, i))->ProcessID;
	}
	doOneUnlck(10);

	doOnelck(11);
	SPData.NumberOfDiskSuspendedProcesses7 = numberOFItemOnQueue(11);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses7; i++) {
		SPData.DiskSuspendedProcessPIDs7[i] = ((PCB*)QWalk(11, i))->ProcessID;
	}
	doOneUnlck(11);

	doOnelck(12);
	SPData.NumberOfDiskSuspendedProcesses8 = numberOFItemOnQueue(12);
	for (i = 0; i < SPData.NumberOfDiskSuspendedProcesses8; i++) {
		SPData.DiskSuspendedProcessPIDs8[i] = ((PCB*)QWalk(12, i))->ProcessID;
	}
	doOneUnlck(12);

	SPData.NumberOfTerminatedProcesses = 0;   // Not used at this time
	CALL(SPPrintLine(&SPData));
}

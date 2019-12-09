#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED

#define MAX_PROCESS_NAME 50

#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE

#define    PROCESS_INITIAL_ID         2
#define    MAX_PROCESS_NUMBER         15

#define         LEGAL_MESSAGE_LENGTH           (INT16)64

typedef struct
{
    char ProcessName[MAX_PROCESS_NAME];
    int ProcessID;

    //char CurrentDirectory[7];
    int iNode;
    int diskID;
    int CurrentSector;
    int previousSector;
    //int ProcessCounter;
    void* PageTable;
    long NewContext;
    int WaitingTime;
    int priority;
    void* startingAddress;
}PCB;

//void equalTo(PCB* pcb_copy, PCB pcb);
PCB* runningProcess(char* ProcessName);
void IncrementAllocatePID(void);
int osCreateProcess(char* ProcessName, int priority, void* startingAddress, int *ProcessID);
int getProcessID(char* ProcessName, int *PID);
int timerWaitingAdd(PCB* pcb);
//PCB* timerWaitingDelete(void);
int terminateProcess(int ProcessID);
PCB* removeProcess(int QID);
PCB* removeProcess2(int QID, PCB *pcb);
//void runningReturnNull(PCB* pcb);
int addToQueue(int QID, PCB* pcb);
PCB* minTimeQueue(void);
int suspendProcess(int pid);
int resumeProcess(int pid);
PCB* processToRun(void);
int numberOFItemOnQueue(int QID);
PCB* ifItemExist(int pid, int* k);
int changePriority(int pid, int new_priority);
//int sendValue(int flag);
//int numberOfProcess(void);
PCB* getTheHeadOfQueue(int QID);

void check_state(char* action, PCB* target_process);
#endif // PCB_H_INCLUDED

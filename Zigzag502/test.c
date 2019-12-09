/************************************************************************
 test.c

 These programs are designed to test YOUR OS502 functionality

 Revision History:
 1.0 August 1990
 1.1 December 1990: Tests cleaned up; 1b, 1e - 1k added
 Portability attempted.
 1.2 December 1991: Still working on portability.
 1.3 July     1992: tests1i/1j made reentrant.
 1.4 December 1992: Minor changes - portability
 tests2c/_M added.  2f/2g rewritten
 1.5 August   1993: Produced new test2g to replace old
 2g that did a swapping test.
 1.6 June     1995: Test Cleanup.
 1.7 June     1999: Add test0, minor fixes.
 2.0 January  2000: Lots of small changes & bug fixes
 2.1 March    2001: Minor Bug fixes.
                    Rewrote GetSkewedRandomNumber
 2.2 July     2002: Make appropriate for undergrads
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 3.13 November 2004: Minor fix defining USER
 3.41 August  2009: Additional work for multiprocessor + 64 bit
 3.53 November 2011: Changed test2c so data structure used
 ints (4 bytes) rather than longs.
 3.61 November 2012: Fixed a number of bugs in test2g and test2gx
 (There are probably many more)
 3.70 December 2012: Rename test2g to test2h - it still does
 shared memory.  Define a new test2g that runs
 multiple copies of test2f.
 4.03 December 2013: Changes to test 2e and 2f.
 4.10 December 2013: Changes to test 2g regarding a clean ending.
 4.10 July     2014: Numerous small changes to tests.  Major rewrite of Test2h
 4.11 November 2014: Bug fix to test2c and test2e
 4.12 November 2014: Bug fix to test2e
 4.20 January 2015: Start work to make thread safe and therefore support
 multiprocessors.
 4.30 January 2016:  Totally redefine the test cases
 4.31 November 2016: Minor bugfixes
 4.32 November 2016: More Minor bugfixes
 4.33 November 2016: More Minor bugfixes
 4.40 March    2017: Many bugfixes that caused tests to misbehave.
 4.50 April    2018: Tests totally rearranged
 4.60 March    2019: New tests added - many bugfixes
 ************************************************************************/

#define          USER
#define          TEST_VERSION      "4.60"

#include         "global.h"
#include         "protos.h"
#include         "syscalls.h"

#include         <time.h>
#include         "stdio.h"
#include         "string.h"
#include         "stdlib.h"
#include         "math.h"

//      Prototypes for internally called routines.

void testD(void);
void testE(void);
void testS(void);
void testX(void);
void testZ(void);

void ErrorExpected(INT32, char[]);
void SuccessExpected(INT32, char[]);
//void GetSkewedRandomNumber(long *, long);
void Test44_Statistics(long Pid, long PageNumber, int Mode);

/**************************************************************************
 Test0
 Exercises the system calls for GET_TIME_OF_DAY and TERMINATE_PROCESS
NOTE:  The "aprintf" function used here is just a printf that's used
       atomically.  Only one printf will happen at a time.
 **************************************************************************/
void test0(void) {
	long ReturnedTime;
	long ErrorReturned;
	aprintf("Release %s:  Test 0\n", TEST_VERSION);
	GET_TIME_OF_DAY(&ReturnedTime);

	aprintf("Time of day is %ld\n", ReturnedTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

	// We should never get to this line since the TERMINATE_PROCESS call
	// should cause the program to end.
	aprintf("ERROR: Test should be terminated but isn't.\n");
}     // End of test0

/**************************************************************************
 Test1
 Exercises GET_TIME_OF_DAY, GET_PROCESS_ID and TERMINATE_PROCESS
 The incremental code required in the Operating System is a structure
 that will hold various process-specific properties.  This is called
 a Process Control Block (or PCB).
 GET_PROCESS_ID - as implemented here, should return the process ID
 of the current process.  In later code, we'll test the return of the PID of
 other processes.
 **************************************************************************/
void test1(void) {
	long ReturnedTime;
	long ErrorReturned;          // System call returns error here
	long MyProcessID;            // Used as return of process id's.

	aprintf("Release %s:  Test 1\n", TEST_VERSION);
	GET_TIME_OF_DAY(&ReturnedTime);
	aprintf("Time of day is %ld\n", ReturnedTime);

	//  Now test the call GET_PROCESS_ID - We ask for our own PID
	GET_PROCESS_ID("", &MyProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	aprintf("The PID of this process is %ld\n", MyProcessID);

	TERMINATE_PROCESS(-1, &ErrorReturned);
	aprintf("ERROR: Test should be terminated but isn't.\n");
}          // End of test1

/**************************************************************************
 Test2
 Exercises GET_TIME_OF_DAY, SLEEP, GET_PROCESS_ID and TERMINATE_PROCESS
 What should happen here is that the difference between the time1 and time2
 will be GREATER than SleepTime.  This is because a timer interrupt takes
 AT LEAST the time specified.
 GET_PROCESS_ID - as implemented here, should return the process ID
 of the current process.  Later on we'll test the return of the PID of
 other processes.
 **************************************************************************/
void test2(void) {
	long ErrorReturned;          // System call returns error here
	long MyProcessID;            // Used as return of process id's.
	long SleepTime = 100;
	long Time1, Time2;

	aprintf("This is Release %s:  Test 2\n", TEST_VERSION);
	GET_TIME_OF_DAY(&Time1);
	SLEEP(SleepTime);
	GET_TIME_OF_DAY(&Time2);

	aprintf("Sleep Time = %ld, elapsed time= %ld\n", SleepTime, Time2 - Time1);

	//      Now test the call GET_PROCESS_ID for ourselves
	GET_PROCESS_ID("", &MyProcessID, &ErrorReturned);     // Legal
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	GET_TIME_OF_DAY(&Time2);
	aprintf("TEST 2, PID %ld, Ends at Time %ld\n", MyProcessID, Time2);

	TERMINATE_PROCESS(-1, &ErrorReturned);

	aprintf("ERROR: Test should be terminated but isn't.\n");
}          // End of test2

/**************************************************************************
 Test3
 Exercises GET_TIME_OF_DAY, PHYSICAL_DISK_WRITE, PHYSICAL_DISK_READ
 and TERMINATE_PROCESS
 This test requires that your process wait for the disk action to
 complete.  This MEANS the test will NOT complete in a very short time.
 **************************************************************************/
// This structure is used throughout this code to format the data written
// to the disk.
typedef union {
	char char_data[PGSIZE ];
	UINT32 int_data[PGSIZE / sizeof(int)];
} DISK_DATA;

void test3(void) {
	long ErrorReturned;         // System call returns error here
	long Time1, Time2;          // The start and end of time for disk action
	long OurProcessID;          // Returned by the system call

	DISK_DATA *DataWritten;     // Structure holding data written to disk
	DISK_DATA *DataRead;        // Structure holding data read from disk
	long DiskID;                // Which disk we're using
	long Sector;                // Which sector on the disk we're using
	INT32 CheckValue = 1234;    // Used here to ensure it's a disk buffer

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test3: Pid %ld\n", TEST_VERSION, OurProcessID);

	DataWritten = (DISK_DATA *) calloc(1, sizeof(DISK_DATA));
	DataRead = (DISK_DATA *) calloc(1, sizeof(DISK_DATA));
	if (DataRead == 0)
		aprintf("Something screwed up allocating space in TEST 3\n");

	DiskID = OurProcessID % MAX_NUMBER_OF_DISKS;
	Sector = OurProcessID + 2;
	DataWritten->int_data[0] = DiskID;
	DataWritten->int_data[1] = CheckValue;
	DataWritten->int_data[2] = Sector;
	DataWritten->int_data[3] = OurProcessID;
	GET_TIME_OF_DAY(&Time1);
	PHYSICAL_DISK_WRITE(DiskID, Sector, (char* )(DataWritten->char_data));
	GET_TIME_OF_DAY(&Time2);
	aprintf("Time to do disk write = %ld\n", Time2 - Time1);
	GET_TIME_OF_DAY(&Time1);
	PHYSICAL_DISK_READ(DiskID, Sector, (char* )(DataRead->char_data));
	GET_TIME_OF_DAY(&Time2);
	aprintf("Time to do disk read = %ld\n", Time2 - Time1);

	if ((DataRead->int_data[0] != DataWritten->int_data[0])
			|| (DataRead->int_data[1] != DataWritten->int_data[1])
			|| (DataRead->int_data[2] != DataWritten->int_data[2])
			|| (DataRead->int_data[3] != DataWritten->int_data[3])) {
		aprintf("ERROR in Physical Disk Read and Write, \n");
		aprintf("Written:  %d,  %d,  %d,  %d\n", DataWritten->int_data[0],
				DataWritten->int_data[1], DataWritten->int_data[2],
				DataWritten->int_data[3]);
		aprintf("Read:     %d,  %d,  %d,  %d\n", DataRead->int_data[0],
				DataRead->int_data[1], DataRead->int_data[2],
				DataRead->int_data[3]);
	} else {
		aprintf("SUCCESS in Physical Disk Read and Write\n");
	}
	// The CHECK_DISK system call can be used as a debugging tool.
	// It is implemented in svc simply by calling the hardware function.
	// There will be a file produced that shows what is physically
	//    on your disk.
	CHECK_DISK(DiskID, &ErrorReturned);
	TERMINATE_PROCESS(-1, &ErrorReturned);
	aprintf("ERROR: Test should be terminated but isn't.\n");
}          // End of test3

/**************************************************************************
 Test4
 Exercises the
 CREATE_PROCESS - both legal and illegal
 TERMINATE_PROCESS  (of a process other than ourselves)
 GET_PROCESS_ID - both legal and illegal

 This test tries lots of different inputs for create_process.
 In particular, there are tests for each of the following:

 1. use of illegal priorities
 2. use of a process name of an already existing process.
 3. creation of a LARGE number of processes, showing that
 there is a limit somewhere ( you run out of some
 resource ) in which case you take appropriate action.

 Test the following items for get_process_id:

 1. Various legal process id inputs.
 2. An illegal/non-existent name.

 This test requires that you further expand your ProcessControlBlock
 **************************************************************************/

#define         ILLEGAL_PRIORITY                -3
#define         LEGAL_PRIORITY                  10

void test4(void) {
	long ProcessID1;            // Used as return of process id's.
	long ProcessID2;            // Used as return of process id's.
	long OurProcessID;
	long NumberOfProcesses = 0; // Counter of # of processes created.
	long ErrorReturned;         // Used as return of error code.
	long TimeNow;               // Holds ending time of test
	char ProcessName[16];       // Holds generated process name

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test4: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Try to create a process with an illegal priority.
	CREATE_PROCESS("test4_a", testX, ILLEGAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	ErrorExpected(ErrorReturned, "CREATE_PROCESS");

	// Create two processes with same name - 1st succeeds, 2nd fails
	// Then terminate the process that has been created
	CREATE_PROCESS("two_the_same", testX, LEGAL_PRIORITY, &ProcessID2,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	CREATE_PROCESS("two_the_same", testX, LEGAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	ErrorExpected(ErrorReturned, "CREATE_PROCESS");
	TERMINATE_PROCESS(ProcessID2, &ErrorReturned);
	SuccessExpected(ErrorReturned, "TERMINATE_PROCESS");

	// Loop until an error is found on the CREATE_PROCESS system call.
	// Since the call itself is legal, we must get an error
	// because we exceed some limit - that limit is one imposed by
	// the number of processes supported by the OS.  In this case
	// 15 will be a good number.
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		NumberOfProcesses++; /* Generate next unique program name*/
		sprintf(ProcessName, "Test4_%ld", NumberOfProcesses);
		aprintf("Creating process \"%s\"\n", ProcessName);
		CREATE_PROCESS(ProcessName, testX, LEGAL_PRIORITY, &ProcessID1,
				&ErrorReturned);
	}
    //QPrint(1);
	//  When we get here, we've created all the processes we can.
	//  So the OS should have given us an error
	ErrorExpected(ErrorReturned, "CREATE_PROCESS");
	aprintf("%ld processes were created in all.\n", NumberOfProcesses);

	//      Now test the call GET_PROCESS_ID for ourselves
	GET_PROCESS_ID("", &ProcessID2, &ErrorReturned);     // Legal
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	aprintf("The PID of this process is %ld\n", ProcessID2);

	// Try GET_PROCESS_ID on another existing process
	strcpy(ProcessName, "Test4_1");
	GET_PROCESS_ID(ProcessName, &ProcessID1, &ErrorReturned); /* Legal */
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	aprintf("The PID of target process is %ld\n", ProcessID1);

	// Try GET_PROCESS_ID on a non-existing process
	GET_PROCESS_ID("bogus_name", &ProcessID1, &ErrorReturned); // Illegal
	ErrorExpected(ErrorReturned, "GET_PROCESS_ID");

	GET_TIME_OF_DAY(&TimeNow);
	aprintf("TEST 4, PID %ld, Ends at Time %ld\n", ProcessID2, TimeNow);
	TERMINATE_PROCESS(-2, &ErrorReturned);  // -2 means end the simulation
}         // End of test4

/**************************************************************************
 test5
 Test timer.  We create a process that contains a number of timer
 actions.

 **************************************************************************/
#define         PRIORITY_TEST5                 10

void test5(void) {
	long CurrentTime;
	long ErrorReturned;
	long OurProcessID;
	long ProcessID;
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test5: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Create a process to perform many actions
	CREATE_PROCESS("testX", testX, PRIORITY_TEST5, &ProcessID, &ErrorReturned);

	// We loop here.  As long as the testX process exists, the OS should
	// return success.  When the call fails, it means that the testX
	// process has terminated.
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("testX", &ProcessID, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 5:   Ends at Time %ld\n", CurrentTime);
	// Terminates us and our child - though the child should be gone
	TERMINATE_PROCESS(-1, &ErrorReturned);
}           // End of test5

/**************************************************************************
 Test6

 Exercises the SUSPEND_PROCESS and RESUME_PROCESS commands

 This test should try lots of different inputs for the system calls
 SUSPEND_PROCESS and RESUME_PROCESS.
 In particular, there should be tests for each of the following:

 1. use of illegal process id.
 2. what happens when you suspend yourself - is it legal?  The answer
    to this depends on the OS architecture and is up to the developer.
 3. suspending an already suspended process.
 4. resuming a process that's not suspended.

 there are probably lots of other conditions possible.

 **************************************************************************/
#define         LEGAL_PRIORITY_6               10

void test6(void) {
	long TargetProcessID;
	long OurProcessID;
	long TimeNow;               // Holds ending time of test
	long ErrorReturned;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test6: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Make a legal target process
	CREATE_PROCESS("test6_a", testX, LEGAL_PRIORITY_6, &TargetProcessID,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	// Try to Suspend an Illegal PID
	SUSPEND_PROCESS((INT32 )9999, &ErrorReturned);
	ErrorExpected(ErrorReturned, "SUSPEND_PROCESS");

	// Try to Resume an Illegal PID
	RESUME_PROCESS((INT32 )9999, &ErrorReturned);
	ErrorExpected(ErrorReturned, "RESUME_PROCESS");

	// Suspend a legal PID
	SUSPEND_PROCESS(TargetProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "SUSPEND_PROCESS");

	// Suspend already suspended PID
	SUSPEND_PROCESS(TargetProcessID, &ErrorReturned);
	ErrorExpected(ErrorReturned, "SUSPEND_PROCESS");

	// Do a legal resume of the process we have suspended
	RESUME_PROCESS(TargetProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "RESUME_PROCESS");

	// Resume an already resumed process
	RESUME_PROCESS(TargetProcessID, &ErrorReturned);
	ErrorExpected(ErrorReturned, "RESUME_PROCESS");

	// Try to resume ourselves
	RESUME_PROCESS(OurProcessID, &ErrorReturned);
	ErrorExpected(ErrorReturned, "RESUME_PROCESS");

	// It may or may not be legal to suspend ourselves;
	// architectural decision.   It can be a useful technique
	// as a way to pass off control to another process.
	// If you decide that you want to be able to suspend yourself,
	//    then you will need to have a check that shows this is
	//    the last available process; and the act of suspending
	//    the last process results in exiting the simulation.
	SUSPEND_PROCESS(-1, &ErrorReturned);

	/* If we returned "SUCCESS" here, then there is an inconsistency;
	 * success implies that the process was suspended.  But if we
	 * get here, then we obviously weren't suspended.  Therefore
	 * this must be an error.                                    */
	ErrorExpected(ErrorReturned, "SUSPEND_PROCESS");

	GET_TIME_OF_DAY(&TimeNow);
	aprintf("TEST 6, PID %ld, Ends at Time %ld\n", OurProcessID, TimeNow);

	TERMINATE_PROCESS(-2, &ErrorReturned);
}                                                // End of test6

/**************************************************************************
 test7

 Successfully execute SUSPEND_PROCESS and RESUME_PROCESS system calls.
 The operating system will cause the target processes to become
 NOT ready but go on a suspend Q.  Note to do this you will NOT be
 asking the hardware to suspend the target process; in fact, that
 target process is not currently executing.
 This test assumes that test6 runs successfully.

 In particular, show what happens to scheduling when processes
 are temporarily suspended.

 This test works by starting up a number of processes at different
 priorities.  Then some of them are suspended.  Then some are resumed.

 IMPORTANT:  You must be able to make the results of this test VISIBLE!
 This means using SchedulerPrinter at the right place so that the ready
 and suspended queues are displayed - this will show that the processes
 really have been suspended.  If you don't show this, the instructor
 can't evaluate this work.
 **************************************************************************/
#define         PRIORITY7_1             5
#define         PRIORITY7_2            10
#define         PRIORITY7_3            15
#define         PRIORITY7_4            20
#define         PRIORITY7_5            25

void test7(void) {
	long OurProcessID;         // PID of this process
	long ProcessID1;           // Created processes
	long ProcessID2;
	long ProcessID3;
	long ProcessID4;
	long ProcessID5;
	long SleepTime = 300;
	long TimeNow;               // Holds ending time of test
	long ErrorReturned;
	int Iterations;

	// Get OUR PID
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test7: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Make legal targets
	CREATE_PROCESS("test7_a", testX, PRIORITY7_1, &ProcessID1,
			&ErrorReturned);
	CREATE_PROCESS("test7_b", testX, PRIORITY7_2, &ProcessID2,
			&ErrorReturned);
	CREATE_PROCESS("test7_c", testX, PRIORITY7_3, &ProcessID3,
			&ErrorReturned);
	CREATE_PROCESS("test7_d", testX, PRIORITY7_4, &ProcessID4,
			&ErrorReturned);
	CREATE_PROCESS("test7_e", testX, PRIORITY7_5, &ProcessID5,
			&ErrorReturned);

	// Let the 5 processes go for a while
	SLEEP(SleepTime);

	// Do a set of suspends/resumes four times
	for (Iterations = 0; Iterations < 4; Iterations++) {
		// Suspend 3 of the pids and see what happens - we should see
		// scheduling behavior where the processes are yanked out of the
		// ready and the waiting states, and placed into the suspended state.

		SUSPEND_PROCESS(ProcessID1, &ErrorReturned);
		SUSPEND_PROCESS(ProcessID3, &ErrorReturned);
		SUSPEND_PROCESS(ProcessID5, &ErrorReturned);

		// Sleep so we can watch the scheduling action
		SLEEP(SleepTime);

		RESUME_PROCESS(ProcessID1, &ErrorReturned);
		RESUME_PROCESS(ProcessID3, &ErrorReturned);
		RESUME_PROCESS(ProcessID5, &ErrorReturned);
	}

	//   Wait for children to finish, then quit
	SLEEP((INT32 )10000);
	GET_TIME_OF_DAY(&TimeNow);
	aprintf("TEST 7, PID %ld, Ends at Time %ld\n", OurProcessID, TimeNow);
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                        // End of test7


/**************************************************************************
Test8  Successfully change the priority of a process

 There are TWO ways we can see that the priorities have changed:
 1. When you change the priority, it should be possible to see
    the scheduling behaviour of the system change; processes
    that used to be scheduled first are no longer first.  This will be
    visible in the ready Q as shown by the scheduler printer.
 2. The processes with more favorable priority should schedule first so
    they should finish first.

 IMPORTANT:  You must be able to make the results of this test VISIBLE!
 This means using SchedulerPrinter at the right place so that the ready
 queue is displayed - this will show that the processes really have
 changed priority.  The ordering of the processes on the ready queue
 should be modified.  If you don't show this, the instructor
 can't evaluate this work.
 **************************************************************************/
#define         MOST_FAVORABLE_PRIORITY         1
#define         FAVORABLE_PRIORITY             10
#define         NORMAL_PRIORITY                20
#define         LEAST_FAVORABLE_PRIORITY       30

void test8(void)  {
	long OurProcessID;         // PID of this process
	long ProcessID1;           // Created processes
	long ProcessID2;
	long ProcessID3;
	long ProcessID4;
	long ProcessID5;
	long ProcessID6;
	long SleepTime = 600;
	long CurrentTime;
	long ErrorReturned;
	long Ourself = -1;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test8: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Make our priority high
	CHANGE_PRIORITY(Ourself, MOST_FAVORABLE_PRIORITY, &ErrorReturned);

	// Make legal targets
	printf( "TEST 8: Processes started with priority %d\n", NORMAL_PRIORITY);
	CREATE_PROCESS("test8_a", testX, NORMAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	CREATE_PROCESS("test8_b", testX, NORMAL_PRIORITY, &ProcessID2,
			&ErrorReturned);
	CREATE_PROCESS("test8_c", testX, NORMAL_PRIORITY, &ProcessID3,
			&ErrorReturned);
	CREATE_PROCESS("test8_d", testX, NORMAL_PRIORITY, &ProcessID4,
			&ErrorReturned);
	CREATE_PROCESS("test8_e", testX, NORMAL_PRIORITY, &ProcessID5,
			&ErrorReturned);
	CREATE_PROCESS("test8_f", testX, NORMAL_PRIORITY, &ProcessID6,
			&ErrorReturned);

	//      Sleep awhile to watch the scheduling
	SLEEP(SleepTime);

	//  Now change the priority - it should be possible to see
	//  that the priorities have been changed for processes that
	//  are ready and for processes that are sleeping.

	aprintf( "TEST 8: Process %ld now has priority %d\n", ProcessID1,
			FAVORABLE_PRIORITY);
	CHANGE_PRIORITY( ProcessID1, FAVORABLE_PRIORITY, &ErrorReturned);

	aprintf( "TEST 8: Process %ld now has priority %d\n", ProcessID3,
			LEAST_FAVORABLE_PRIORITY);
	CHANGE_PRIORITY( ProcessID3, LEAST_FAVORABLE_PRIORITY, &ErrorReturned);

	//      Sleep awhile to watch the scheduling
	SLEEP(SleepTime);

	//  Now change the priority - it should be possible to see
	//  that the priorities have been changed for processes that
	//  are ready and for processes that are sleeping.

	aprintf( "TEST 8: Process %ld now has priority %d\n", ProcessID1,
			LEAST_FAVORABLE_PRIORITY);
	CHANGE_PRIORITY(ProcessID1, LEAST_FAVORABLE_PRIORITY, &ErrorReturned);

	aprintf( "TEST 8: Process %ld now has priority %d\n", ProcessID2,
			FAVORABLE_PRIORITY);
	CHANGE_PRIORITY(ProcessID2, FAVORABLE_PRIORITY, &ErrorReturned);

	//     Sleep awhile to watch the scheduling
	SLEEP(SleepTime);

	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 8:   Ends at Time %ld\n", CurrentTime);

	// Terminate everyone
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                               // End of test8
/**************************************************************************

 Test9   SEND_MESSAGE and RECEIVE_MESSAGE with errors.

 This has the same kind of error conditions that previous tests did;
 bad PIDs, bad message lengths, illegal buffer addresses, etc.
 Your imagination can go WILD on this one.

 This is a good time to mention an important aspect of the OS and
 scheduling.
 As you know, after doing a switch_context, the hardware passes
 control to the code after SwitchContext.  In other words, a process
 that is being rescheduled "disappears" into SwitchContext.  But
 it "reappears" after some other process causes that "disappeared"
 process to be scheduled.
 So at that "reappearing" location is a good place to put your message
 code so as to do the rendevous work that's necessary to match up sends
 and receives.

 Why do this:        Suppose process A has sent a message to
 process B.  It so happens that you may well want
 to do some preparation in process B once it's
 registers are in memory, but BEFORE it executes
 the test.  In other words, it allows you to
 complete the work for the send to process B.

 We use testX as our target process; but since it doesn't have any
 messaging code, it never actually sends or receives messages so it
 will never actually be scheduled.

 **************************************************************************/

#define         LEGAL_MESSAGE_LENGTH           (INT16)64
#define         ILLEGAL_MESSAGE_LENGTH         (INT16)1000

#define         MOST_FAVORABLE_PRIORITY         1
#define         NORMAL_PRIORITY                20

long    OurProcessID;
long    ErrorReturned;
long    TargetProcessID;
long    CurrentTime;
typedef struct {
    long    target_pid;
    long    source_pid;
    long    actual_source_pid;
    long    send_length;
    long    receive_length;
    long    actual_send_length;
    long    loop_count;
    char    msg_buffer[LEGAL_MESSAGE_LENGTH ];
} TEST9_DATA;

void test9(void) {
    TEST9_DATA *td; // Use as ptr to data */

    // Here we maintain the data to be used by this process when running
    // on this routine.  This code should be re-entrant.

    td = (TEST9_DATA *) calloc(1, sizeof(TEST9_DATA));
    if (td == 0) {
        printf("Something screwed up allocating space in test9\n");
    }

    td->loop_count = 0;

    // Get OUR PID
    GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
    printf("Release %s:Test 9: Pid %ld\n", CURRENT_REL, OurProcessID);

    // Make our priority high
    CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &ErrorReturned);

    // Make a legal target
    CREATE_PROCESS("test9_a", testX, NORMAL_PRIORITY, &TargetProcessID,
            &ErrorReturned);

    // Send a message to illegal process
    td->target_pid = 9999;
    td->send_length = 8;
    SEND_MESSAGE(td->target_pid, "message", td->send_length, &ErrorReturned);
    ErrorExpected(ErrorReturned, "SEND_MESSAGE");

    // Try an illegal message length
    td->target_pid = TargetProcessID;
    td->send_length = ILLEGAL_MESSAGE_LENGTH;
    SEND_MESSAGE(td->target_pid, "message", td->send_length, &ErrorReturned);
    ErrorExpected(ErrorReturned, "SEND_MESSAGE");

    //      Receive from illegal process
    td->source_pid = 9999;
    td->receive_length = LEGAL_MESSAGE_LENGTH;
    RECEIVE_MESSAGE(td->source_pid, td->msg_buffer, td->receive_length,
            &(td->actual_send_length), &(td->actual_source_pid), &ErrorReturned);
    ErrorExpected(ErrorReturned, "RECEIVE_MESSAGE");

    //      Receive with illegal buffer size
    td->source_pid = TargetProcessID;
    td->receive_length = ILLEGAL_MESSAGE_LENGTH;
    RECEIVE_MESSAGE(td->source_pid, td->msg_buffer, td->receive_length,
            &(td->actual_send_length), &(td->actual_source_pid), &ErrorReturned);
    ErrorExpected(ErrorReturned, "RECEIVE_MESSAGE");

    //      Send a legal ( but long ) message to self
    td->target_pid = OurProcessID;
    td->send_length = LEGAL_MESSAGE_LENGTH;
    SEND_MESSAGE(td->target_pid, "a long but legal message", td->send_length,
            &ErrorReturned);
    SuccessExpected(ErrorReturned, "SEND_MESSAGE");
    td->loop_count++;      // Count the number of legal messages sent

    //   Receive this long message, which should error because the receive buffer is too small
    td->source_pid = OurProcessID;
    td->receive_length = 10;
    RECEIVE_MESSAGE(td->source_pid, td->msg_buffer, td->receive_length,
            &(td->actual_send_length), &(td->actual_source_pid), &ErrorReturned);
    ErrorExpected(ErrorReturned, "RECEIVE_MESSAGE");

    // Keep sending legal messages until the architectural
    // limit for buffer space is exhausted.  In order to pass
    // a later test, this number should be at least EIGHT

    ErrorReturned = ERR_SUCCESS;
    while (ErrorReturned == ERR_SUCCESS) {
        td->target_pid = TargetProcessID;
        td->send_length = LEGAL_MESSAGE_LENGTH;
        SEND_MESSAGE(td->target_pid, "Legal Test9 Message", td->send_length, &ErrorReturned);
        td->loop_count++;
    }  // End of while

    printf("A total of %ld messages were enqueued.\n", td->loop_count - 1);

    GET_TIME_OF_DAY(&CurrentTime);
    aprintf("TEST 9:   Ends at Time %ld\n", CurrentTime);

    TERMINATE_PROCESS(-2, &ErrorReturned);
}                                              // End of test9

/**************************************************************************

 Test10   SEND_MESSAGE and RECEIVE_MESSAGE Successfully.

 Creates three other processes, each running their own code.
 RECEIVE and SEND messages are winged back and forth at them.

 The SEND and RECEIVE system calls as implemented by this test
 imply the following behavior:

 SENDER = PID A          RECEIVER = PID B,

 Designates source_pid =
 target_pid =          A            C             -1
 ----------------+------------+------------+--------------+
 |            |            |              |               |
 | B          |  Message   |     X        |   Message     |
 |Transmitted |            | Transmitted  |               |
 ----------------+------------+------------+--------------+
 |            |            |              |               |
 | C          |     X      |     X        |       X       |
 |            |            |              |               |
 ----------------+------------+------------+--------------+
 |            |            |              |               |
 | -1         |   Message  |     X        |   Message     |
 | Transmitted|            | Transmitted  |               |
 ----------------+------------+------------+--------------+
 A broadcast ( target_pid = -1 ) means send to everyone BUT yourself.
 ANY of the receiving processes can handle a broadcast message.
 A receive ( source_pid = -1 ) means receive from anyone.
 **************************************************************************/

#define         LEGAL_MESSAGE_LENGTH            (INT16)64
#define         ILLEGAL_MESSAGE_LENGTH          (INT16)1000
#define         MOST_FAVORABLE_PRIORITY         1
#define         NORMAL_PRIORITY                20

long    TargetProcessID1, TargetProcessID2, TargetProcessID3;
long    OurProcessID;
long    ErrorReturned;
long    CurrentTime;

typedef struct {
    long    target_pid;
    long    source_pid;
    long    actual_source_pid;
    long    send_length;
    long    receive_length;
    long    actual_send_length;
    long    send_loop_count;
    long    receive_loop_count;
    char    msg_buffer[LEGAL_MESSAGE_LENGTH ];
    char    msg_sent[LEGAL_MESSAGE_LENGTH ];
} TEST10_DATA;

void test10(void) {
    int Iteration;
    TEST10_DATA *td;           // Use as pointer to data

    // Here we maintain the data to be used by this process when running
    // on this routine.  This code should be re-entrant.

    td = (TEST10_DATA *) calloc(1, sizeof(TEST10_DATA));
    if (td == 0) {
        printf("Something screwed up allocating space in test1j\n");
    }
    td->send_loop_count = 0;
    td->receive_loop_count = 0;

    // Get OUR PID and print it
    GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
    printf("Release %s:Test 10: Pid %ld\n", CURRENT_REL, OurProcessID);

    // Make our priority high
    CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &ErrorReturned);
    SuccessExpected(ErrorReturned, "CHANGE_PRIORITY");

    // Make 3 legal targets
    CREATE_PROCESS("test10_1", testE, NORMAL_PRIORITY, &TargetProcessID1,
            &ErrorReturned);
    SuccessExpected(ErrorReturned, "CREATE_PROCESS");
    CREATE_PROCESS("test10_2", testE, NORMAL_PRIORITY, &TargetProcessID2,
            &ErrorReturned);
    SuccessExpected(ErrorReturned, "CREATE_PROCESS");
    CREATE_PROCESS("test10_3", testE, NORMAL_PRIORITY, &TargetProcessID3,
            &ErrorReturned);
    SuccessExpected(ErrorReturned, "CREATE_PROCESS");

    //      Send/receive a legal message to each child
    for (Iteration = 1; Iteration <= 3; Iteration++) {
        if (Iteration == 1) {
            td->target_pid = TargetProcessID1;
            strcpy(td->msg_sent, "message to #1");
        }
        if (Iteration == 2) {
            td->target_pid = TargetProcessID2;
            strcpy(td->msg_sent, "message to #2");
        }
        if (Iteration == 3) {
            td->target_pid = TargetProcessID3;
            strcpy(td->msg_sent, "message to #3");
        }
        td->send_length = 20;
        SEND_MESSAGE(td->target_pid, td->msg_sent, td->send_length, &ErrorReturned);
        SuccessExpected(ErrorReturned, "SEND_MESSAGE");

        td->source_pid = -1;
        td->receive_length = LEGAL_MESSAGE_LENGTH;
        RECEIVE_MESSAGE(td->source_pid, td->msg_buffer, td->receive_length,
                &(td->actual_send_length), &(td->actual_source_pid),
                &ErrorReturned);
        SuccessExpected(ErrorReturned, "RECEIVE_MESSAGE");

        if (strcmp(td->msg_buffer, td->msg_sent) != 0)
            printf("ERROR - msg sent != msg received.\n");

        if (td->actual_source_pid != td->target_pid )
            printf("ERROR - source PID not correct.\n");

        if (td->actual_send_length != td->send_length)
            printf("ERROR - send length not sent correctly.\n");
    }    // End of for loop

    //      Keep sending legal messages until the architectural (OS)
    //      limit for buffer space is exhausted.

    ErrorReturned = ERR_SUCCESS;
    while (ErrorReturned == ERR_SUCCESS) {
        td->target_pid = -1;
        sprintf(td->msg_sent, "This is message %ld", td->send_loop_count);
        td->send_length = 20;
        SEND_MESSAGE(td->target_pid, td->msg_sent, td->send_length, &ErrorReturned);

        td->send_loop_count++;
    }
    td->send_loop_count--;
    printf("A total of %ld messages were enqueued.\n", td->send_loop_count);

    //  Now receive back from the other processes the same number of messages we've just sent.
    while (td->receive_loop_count < td->send_loop_count) {
        td->source_pid = -1;
        td->receive_length = LEGAL_MESSAGE_LENGTH;
        RECEIVE_MESSAGE(td->source_pid, td->msg_buffer, td->receive_length,
                &(td->actual_send_length), &(td->actual_source_pid),
                &ErrorReturned);
        SuccessExpected(ErrorReturned, "RECEIVE_MESSAGE");
        printf("Receive from PID = %ld: length = %ld: msg = %s:\n",
                td->actual_source_pid, td->actual_send_length, td->msg_buffer);
        td->receive_loop_count++;
    }

    printf("A total of %ld messages were received.\n",
            td->receive_loop_count);

    GET_TIME_OF_DAY(&CurrentTime);
    aprintf("TEST 10:   Ends at Time %ld\n", CurrentTime);
    TERMINATE_PROCESS(-2, &ErrorReturned);

}                                                 // End of test10

/**************************************************************************
 Test11 causes disk usage.  It does so by Scheduling the code in
 testD.  This produces multiple IO whereas test3 only does one read
 and one write.

 You will need a way to get the data read back from the disk
 into the buffer defined by the user process.  This can most
 easily be done after the process is rescheduled and about
 to return to user code.

 WARNING:  This test assumes previous tests run successfully
 **************************************************************************/
#define         PRIORITY_TEST11                 10

void test11(void) {
	long CurrentTime;
	long ErrorReturned;
	long OurProcessID;        // The test11 ProcessID
	long TestDProcessID;      // The child testD processID
	long SleepTime = 5000;
	long DiskID;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test11: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Create a process that runs testD
	CREATE_PROCESS("testD", testD, PRIORITY_TEST11, &TestDProcessID, &ErrorReturned);

	// Loop here, waiting until the GET_PROCESS_ID call fails.
	// When it fails, that means the process running testD has terminated.
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		GET_PROCESS_ID("testD", &TestDProcessID, &ErrorReturned);
		SLEEP(SleepTime);
	}
	// We want to do a CHECK_DISK on the disk that was used by
	// testD.  So we use the same algorithm it did so find which
	// disk that was.  DO NOT CHANGE THIS ALGORITHM

	// The CHECK_DISK will let you see what is written on the disk
	// so you are able to do diagnostics.
    DiskID = (TestDProcessID / 2) % MAX_NUMBER_OF_DISKS;
	CHECK_DISK(DiskID, &ErrorReturned);

	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 11:   Ends at Time %ld\n", CurrentTime);

        // Terminates us and our child - though the child should be gone
	TERMINATE_PROCESS(-1, &ErrorReturned);
}                                                         // End of test11

/**************************************************************************
 Test12
 Tests multiple copies of testX running simultaneously.
 These child processes run with the same priority in order to show FCFS
 scheduling behavior;

 WARNING:  This test assumes previous tests run successfully
 **************************************************************************/

void test12(void) {
	long CurrentTime;
	long ProcessID;            // Contains a Process ID
	long ReturnedPID;           // Value of PID returned by System Call
	long OurProcessID;
	long ErrorReturned;
	long Iteration;
	long ChildPriority = 20;
	char ProcessName[16];       // Holds generated process name
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test12: Pid %ld\n", TEST_VERSION, OurProcessID);

	for (Iteration = 0; Iteration < 5; Iteration++) {
		sprintf(ProcessName, "Test12_%ld", Iteration);
		aprintf("Creating process \"%s\"\n", ProcessName);
		CREATE_PROCESS(ProcessName, testX, ChildPriority, &ProcessID,
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	}

	// Now we sleep, see if one of the five processes has terminated, and
	// continue the cycle until all of them are gone.  This allows the testX
	// processes to exhibit scheduling.
	// We know that the process terminated when we do a GET_PROCESS_ID and
	// receive an error on the system call.

	ErrorReturned = ERR_SUCCESS;
	for (Iteration = 0; Iteration < 5; Iteration++) {
		ErrorReturned = ERR_SUCCESS;
		sprintf(ProcessName, "Test12_%ld", Iteration);
		while (ErrorReturned == ERR_SUCCESS) {
			GET_PROCESS_ID(ProcessName, &ReturnedPID, &ErrorReturned);
			if (ErrorReturned != ERR_SUCCESS)
				break;
			SLEEP(SleepTime);
		}
	}
	// When we get here, all child processes have terminated
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 12:   Ends at Time %ld\n", CurrentTime);
	// Terminates us and any children - though children should be gone
	TERMINATE_PROCESS(-1, &ErrorReturned);
}                                                     // End test12

/**************************************************************************
 Test13
 Tests multiple copies of testD running simultaneously.
 These child processes run with the same priority in order to show FCFS
 scheduling behavior;

 WARNING:  This test assumes previous tests run successfully
 **************************************************************************/

void test13(void) {
	long CurrentTime;
	long ProcessID;             // Contains a Process ID
	long ReturnedPID;           // Value of PID returned by System Call
	long OurProcessID;
	long ErrorReturned;
	long Iteration;
	long ChildPriority = 10;
	char ProcessName[16];       // Holds generated process name
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test13: Pid %ld\n", TEST_VERSION, OurProcessID);

	for (Iteration = 0; Iteration < 5; Iteration++) {
		sprintf(ProcessName, "Test13_%ld", Iteration);
		aprintf("Creating process \"%s\"\n", ProcessName);
		CREATE_PROCESS(ProcessName, testD, ChildPriority, &ProcessID,
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	}

	// Now we sleep, see if one of the five processes has terminated, and
	// continue the cycle until all of them are gone.  This allows the testX
	// processes to exhibit scheduling.
	// We know that the process terminated when we do a GET_PROCESS_ID and
	// receive an error on the system call.
	ErrorReturned = ERR_SUCCESS;
	for (Iteration = 0; Iteration < 5; Iteration++) {
		ErrorReturned = ERR_SUCCESS;
		sprintf(ProcessName, "Test13_%ld", Iteration);
		while (ErrorReturned == ERR_SUCCESS) {
			GET_PROCESS_ID(ProcessName, &ReturnedPID, &ErrorReturned);
			if (ErrorReturned != ERR_SUCCESS)
				break;
			SLEEP(SleepTime);
		}
	}
	// When we get here, all child processes have terminated
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 13:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned); // Terminate all
}                                                     // End test13

/**************************************************************************
 Test14
 Creates a process running test12 and a process running test13.
 These in turn run copies of testX and testD producing many
 processes running disk and timer tests.

 WARNING:  This test assumes previous tests run successfully
 **************************************************************************/

void test14(void) {
	long CurrentTime;
	long ReturnedPID;           // Value of PID returned by System Call
	long OurProcessID;
	long ErrorReturned;
	long ChildPriority = 10;
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test14: Pid %ld\n", TEST_VERSION, OurProcessID);

	CREATE_PROCESS("test14_a", test12, ChildPriority, &ReturnedPID,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	CREATE_PROCESS("test14_b", test13, ChildPriority, &ReturnedPID,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	// Now we sleep, see if one of the two child processes has terminated, and
	// continue the cycle until one of them is gone.  This allows the testX
	// and testD processes to exhibit scheduling.
	// We know that the process terminated when we do a GET_PROCESS_ID and
	// receive an error on the system call.

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test14_a", &ReturnedPID, &ErrorReturned);
	}
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test14_b", &ReturnedPID, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 14:   Ends at Time %ld\n", CurrentTime);
	// Who knows what processes might be lurking around.  The "-2" in
	// this call says we should end the simulation.
	TERMINATE_PROCESS(-2, &ErrorReturned);
}                                                     // End test14

/**************************************************************************
 Test21
 Causes a disk to be formatted, and then checks to see that that format
 is correct by issuing a CHECK_DISK call.  The CHECK_DISK call generates
 a file CheckDiskData in the current directory that allows you to examine
 the contents of the disk you have produced.
 For you as an Operating System builder, all you have to do is take in
 the CHECK_DISK system call and from there call the Hardware interface for
 this function.
 **************************************************************************/

void test21(void) {
	long CurrentTime;
	long OurProcessID;
	long DiskID;           // Disk ID we want to format and check
	long ErrorReturned;

	DiskID = 0;
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test21: Pid %ld\n", TEST_VERSION, OurProcessID);

	FORMAT(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "FORMAT");
	CHECK_DISK(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHECK_DISK");

	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 21:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);
}                                                     // End test21

/**************************************************************************
 Test22 - Open (Create) of directories and files
 Performs the following operations:
 1.  Format a disk.
 2.  OPEN_DIR of the directory "root" which causes that
 to become the Current Directory.
 3.  Create a new Directory as a subsidiary directory of root, in this
 example with a name of "Test22".
 4.  Create two new Files within the directory "root".
 5.  Open_Dir the directory "Test22".
 6.  Create a new file within the directory "Test22".
 7.  Produce a CheckDisk output file.

 **************************************************************************/
void test22(void) {
	long CurrentTime;
	long OurProcessID;
	long DiskID = 1;           // Disk ID we want to format and check
	long ErrorReturned;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test22: Pid %ld\n", TEST_VERSION, OurProcessID);

	FORMAT(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "FORMAT");

	OPEN_DIR(DiskID, "root", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR of root");

	CREATE_DIR("Test22", &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_DIR of Test22");

	CREATE_FILE("file1", &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_FILE named file1");

	CREATE_FILE("file2", &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_FILE named file2");

	OPEN_DIR(-1, "Test22", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR of Test22");

	CREATE_FILE("file1", &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_FILE named file1");

	// We're all done - record the structure of the disk
	CHECK_DISK(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHECK_DISK");

	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 22:   Ends at Time %ld\n", CurrentTime);

	// This test has no children so termination should be easy.
	TERMINATE_PROCESS(-1, &ErrorReturned);
}                                                     // End test22
/**************************************************************************
 Test23  Open, Write, Read, Close
 Performs the following operations:
 1.  Format a disk.
 2.  OPEN_DIR of the directory "root" which causes that
 to become the Current Directory.
 3.  Create a new file as a subsidiary  of root, in this
 example with a name of "Test23".
 4.  Write multiple blocks to that file.
 5.  Read back those blocks and ensure they are correct.
 6.  Close the file
 7.  Run CheckDisk to ensure the disk really got cleaned up.
 **************************************************************************/
void test23(void) {
	long CurrentTime;
	long OurProcessID;
	long DiskID;           // Disk ID we want to format and check
	long ErrorReturned;
	long Inode;
	char WriteBuffer[PGSIZE ];
	char ReadBuffer[PGSIZE ];
	int ErrorFound;
	int Index, Index2;

	DiskID = 2;
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test23: Pid %ld\n", TEST_VERSION, OurProcessID);

	FORMAT(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "FORMAT");

	OPEN_DIR(DiskID, "root", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR of root");

	OPEN_FILE("Test23", &Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_FILE");

	// Write a series of blocks to the file
	for (Index = 0; Index < 7; Index++) {
		WriteBuffer[0] = 42;
		for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
			WriteBuffer[Index2] = Index;
		}
		WRITE_FILE(Inode, (long )Index, &WriteBuffer, &ErrorReturned);
	}
	CLOSE_FILE(Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CLOSE_FILE");

	OPEN_FILE("Test23", &Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_FILE");
	// Read a series of blocks from the file
	for (Index = 0; Index < 7; Index++) {
		WriteBuffer[0] = 42;
		for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
			WriteBuffer[Index2] = Index;
		}
		READ_FILE(Inode, (long )Index, &ReadBuffer, &ErrorReturned);
		ErrorFound = FALSE;
		for (Index2 = 0; Index2 < PGSIZE ; Index2++) {
			if (ReadBuffer[Index2] != WriteBuffer[Index2])
				ErrorFound = TRUE;
		}
		if (ErrorFound == TRUE) {
			aprintf("There was an error in ReadFile - ");
			aprintf("we didn't read back the data successfully\n");
		} else {
			aprintf("Test23 - Successfully read logical block %d\n", Index);
		}
	}
	CLOSE_FILE(Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CLOSE_FILE");

	// We're all done - record the structure of the disk
	CHECK_DISK(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHECK_DISK");

	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 23:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

}                                           // End of Test23

/**************************************************************************
 Test24 (Open, Write, Read, Close)
 Performs the following operations:
 1.  Run Test23 - wait until test23 terminates leaving behind a file created
         from its use of the system.
 2.  OPEN_DIR of the directory "root" which causes that
 to become the Current Directory.
 3.  Create a file "Test24".
 4.  DirContents which lists the contents of the directory.
 5.  Run CheckDisk to ensure the disk really got cleaned up.
 **************************************************************************/
void test24(void) {
	long ProcessID;            // Used as ID for child process
	long OurProcessID;         // Used for our process
	long ErrorReturned;         // Used as return of error code.
	long TimeNow;               // Holds ending time of test
	long SleepTime = 1000;      // How long between checks for child
	long Inode;
	long DiskID;
	char WriteBuffer[PGSIZE ];
	char ReadBuffer[PGSIZE ];
	int ErrorFound;
	int Index, Index2;

	DiskID = 2;
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test24: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Try to create a process to some work that's already been successful
	CREATE_PROCESS("test23", test23, LEGAL_PRIORITY, &ProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS of test23 by test24");

	// Wait until this child process has successfully formatted the disk
	//    and completed all its work.
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test23", &ProcessID, &ErrorReturned);
	}
	aprintf("Program TEST 24 has seen the termination of TEST 23\n");

	OPEN_DIR(DiskID, "root", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR of root in test24");

	// Open the file generated by the child process
	OPEN_FILE("Test23", &Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_FILE of file test23 by process test24");

	for (Index = 0; Index < 7; Index++) {
		WriteBuffer[0] = 42;
		for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
			WriteBuffer[Index2] = Index;
		}
		READ_FILE(Inode, (long )Index, &ReadBuffer, &ErrorReturned);
		ErrorFound = FALSE;
		for (Index2 = 0; Index2 < PGSIZE ; Index2++) {
			if (ReadBuffer[Index2] != WriteBuffer[Index2])
				ErrorFound = TRUE;
		}
		if (ErrorFound == TRUE) {
			aprintf("There was an error in ReadFile - ");
			aprintf("we didn't read back the data successfully\n");
		} else {
			aprintf("Test24: Successfully read logical block %d\n", Index);
		}
	}

	OPEN_FILE("Test24", &Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_FILE of file Test24");

	CREATE_DIR("Dir24", &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_DIR of directory Dir 24");

	// See the Z502FileSystem specification for what should
	// be printed by this System Call
	DIR_CONTENTS(&ErrorReturned);
	SuccessExpected(ErrorReturned, "DIR_CONTENTS in test24");

	// We're all done - record the structure of the disk
	CHECK_DISK(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHECK_DISK");

	GET_TIME_OF_DAY(&TimeNow);
	aprintf("TEST 24, PID %ld, Ends at Time %ld\n", OurProcessID, TimeNow);
	// The intent here is to end the simulation.  We have no idea what
	// processes may or may not still exist.
	TERMINATE_PROCESS(-2, &ErrorReturned)
}    // End of Test24

/**************************************************************************
 Test25  Open, Write, Read, Close
 Performs the following operations:
 1.  Format a disk.
 2.  OPEN_DIR of the directory "root" which causes that
     to become the Current Directory.
 3.  Create a new file as a subsidiary  of root, in this
     example with a name of "Test25".
 4.  Write MANY blocks to that file.  This is essentially a test
     to make sure you understand index levels and multiple
     blocks in a file.
 5.  Read back those blocks and ensure they are correct.
 6.  Close the file
 7.  Run CheckDisk to ensure the disk really got cleaned up.
 **************************************************************************/
void test25(void) {
	long CurrentTime;
	long OurProcessID;
	long DiskID = 2;           // Disk ID we want to format and check
	long ErrorReturned;
	long Inode;
	char WriteBuffer[PGSIZE ];
	char ReadBuffer[PGSIZE ];
	int ErrorFound;
	int Index, Index2;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test25: Pid %ld\n", TEST_VERSION, OurProcessID);

	FORMAT(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "FORMAT");

	OPEN_DIR(DiskID, "root", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR of root");

	OPEN_FILE("Test25", &Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_FILE");

	// Write a large number of blocks to the file
	for (Index = 0; Index < 250; Index++) {
		WriteBuffer[0] = 42;
		for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
			WriteBuffer[Index2] = Index;
		}
		WRITE_FILE(Inode, (long )Index, &WriteBuffer, &ErrorReturned);
	}
	CLOSE_FILE(Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CLOSE_FILE");

	OPEN_FILE("Test25", &Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_FILE");

	// Read a all these blocks from the file
	for (Index = 0; Index < 250; Index++) {
		WriteBuffer[0] = 42;
		for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
			WriteBuffer[Index2] = Index;
		}
		READ_FILE(Inode, (long )Index, &ReadBuffer, &ErrorReturned);
		ErrorFound = FALSE;
		for (Index2 = 0; Index2 < PGSIZE ; Index2++) {
			if (ReadBuffer[Index2] != WriteBuffer[Index2])
				ErrorFound = TRUE;
		}
		if (ErrorFound == TRUE) {
			aprintf("There was an error in ReadFile - ");
			aprintf("we didn't read back the data successfully\n");
		} else {
			aprintf("Test25 - Successfully read logical block %d\n", Index);
		}
	}
	CLOSE_FILE(Inode, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CLOSE_FILE");

	// We're all done - record the structure of the disk
	CHECK_DISK(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHECK_DISK");

	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 25:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

}    // End of Test25

/**************************************************************************
 Test26 Open, Write, Read, Close
 Performs the following operations:
    GET_PROCESS_ID - find out who we are
	FORMAT   - Prepare a disk for our use
	OPEN_DIR - Open/Create the root directory
	CREATE_DIR  - Create five  directories under root
    CREATE_PROCESS - start up five processes running testZ and each one
                     is aimed at one of the five directories
 **************************************************************************/
#define         PRIORITY26              10
#define         TEST26_CHILDREN          5
void test26(void) {
	long OurProcessID;          // Used for our process
	long ErrorReturned;         // Used as return of error code.
	long TimeNow;               // Holds ending time of test
	long SleepTime = 1000;
	long DiskID = 1;
	long ProcessID1, ProcessID2, ProcessID3, ProcessID4, ProcessID5;
	int ProcSum = 0;
	int Index;
	char ChildProcessName[8];

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test26: Pid %ld\n", TEST_VERSION, OurProcessID);

	FORMAT(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "FORMAT");

	OPEN_DIR(DiskID, "root", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR of root");

	CREATE_DIR("Child0", &ErrorReturned);
	CREATE_DIR("Child1", &ErrorReturned);
	CREATE_DIR("Child2", &ErrorReturned);
	CREATE_DIR("Child3", &ErrorReturned);
	CREATE_DIR("Child4", &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_DIR of five subdirectories");

	CREATE_PROCESS("testZ-1", testZ, PRIORITY26, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("testZ-2", testZ, PRIORITY26, &ProcessID2, &ErrorReturned);
	CREATE_PROCESS("testZ-3", testZ, PRIORITY26, &ProcessID3, &ErrorReturned);
	CREATE_PROCESS("testZ-4", testZ, PRIORITY26, &ProcessID4, &ErrorReturned);
	CREATE_PROCESS("testZ-5", testZ, PRIORITY26, &ProcessID5, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS of five testZ processes");

	// For this to work successfully in testZ, the PIDs of these children need
	// to be (on mod 5 boundary) 0, 1, 2, 3, 4.  So the PIDs must be generated
	// monotonically increasing.
	// Here we check that behavior.
	ProcSum = (ProcessID1 % TEST26_CHILDREN) + (ProcessID2 % TEST26_CHILDREN)
			+ (ProcessID3 % TEST26_CHILDREN) + (ProcessID4 % TEST26_CHILDREN)
			+ (ProcessID5 % TEST26_CHILDREN);
	if ( ProcSum != 10)  {
		aprintf("ERROR in TEST 26 - the child pids are not correct.\n");
		// Something messed up - terminate the simulation
		TERMINATE_PROCESS(-2, &ErrorReturned)
	}

	// In these next cases, we will loop until EACH of the child processes
	// has terminated.  We know it terminated because for a while we get
	// success on the call GET_PROCESS_ID, and then we get failure when the
	// process no longer exists.
	// We do this for each process so we can make sure all file activity
	// has finished before we continue.

	for (Index = 1; Index <= 5; Index++) { // Look at all the processes we created
		ErrorReturned = ERR_SUCCESS;
		sprintf(ChildProcessName, "testZ-%d", Index);
		// If the call gives success, that means the process still exists
		while (ErrorReturned == ERR_SUCCESS) {
			SLEEP(SleepTime);
			GET_PROCESS_ID(ChildProcessName, &ProcessID1, &ErrorReturned);
		}
	}
	aprintf("Program TEST 26 sees termination of all child processes\n");

	// We're all done - record the structure of the disk
	CHECK_DISK(DiskID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHECK_DISK");

	GET_TIME_OF_DAY(&TimeNow);
	aprintf("TEST 26, PID %ld, Ends at Time %ld\n", OurProcessID, TimeNow);
	// End the simulation by terminating all processes

	TERMINATE_PROCESS(-2, &ErrorReturned);
}    // End of Test26

/**************************************************************************
 Test27 - Test6 in Multiprocessor mode
 This is just a placeholder.  To execute test27, run test10 with
 the argument 'm'
 **************************************************************************/
void test27(void) {
	INT32 ErrorReturned;
	aprintf(
			"This is just a placeholder.  To execute test27, run test10 with the argument M.\n");
	TERMINATE_PROCESS(-1, &ErrorReturned)
}   // End of test27

/**************************************************************************
 Test28 - Test13 in Multiprocessor mode
 This is just a placeholder.  To execute test28, run test13 with
 the argument 'm'
 **************************************************************************/
void test28(void) {
	INT32 ErrorReturned;
	aprintf(
			"This is just a placeholder.  To execute test28, run test13 with the argument M.\n");
	TERMINATE_PROCESS(-1, &ErrorReturned)
}   // End of test28

/**************************************************************************
 Test29 - Test26 in Multiprocessor mode
 This is just a placeholder.  To execute test29, run test26 with
 the argument 'm'
 **************************************************************************/
void test29(void) {
	INT32 ErrorReturned;
	aprintf(
			"This is just a placeholder.  To execute test29, run test26 with the argument M.\n");
	TERMINATE_PROCESS(-1, &ErrorReturned)
}   // End test29

/**************************************************************************
 testD
 Test causes usage of disks.  The test is designed to give
 you a chance to develop a mechanism for handling disk requests.
 This test picks a random disk (based on ProcessID, and then
 works off that target disk, performing many writes and reads.

 You will need a way to get the data read back from the disk
 into the buffer defined by the user process.  This can most
 easily be done after the process is rescheduled and about
 to return to user code.
 **************************************************************************/

#define         DISPLAY_GRANULARITY_D          10
#define         TESTD_LOOPS                    50

void testD(void) {
    long CurrentTime;
    long ErrorReturned;
    long OurProcessID;

    DISK_DATA *DataWritten;     // A pointer to structures holding disk info
    DISK_DATA *DataRead;
    long DiskID;
    INT32 CheckValue = 1234;
    long Sector;
    int Iterations;

    GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
    aprintf("Release %s: test D: Pid %ld\n", TEST_VERSION, OurProcessID);

    DataWritten = (DISK_DATA *) calloc(1, sizeof(DISK_DATA));
    DataRead = (DISK_DATA *) calloc(1, sizeof(DISK_DATA));
    if (DataRead == 0)
        aprintf("Something created an error allocating space in TEST D\n");

    for (Iterations = 0; Iterations < TESTD_LOOPS; Iterations++) {
        // Pick some disk to write to
	// DO NOT change this algorithm.  Test 9 depends on using this
	// same DiskID and knows the ProcessID from creating this testD
        DiskID = (OurProcessID / 2) % MAX_NUMBER_OF_DISKS;
        // Sector is chosen so that multiple processes each running testD
        //   will not write to the same sectors.
        Sector = OurProcessID + (Iterations * 17) % NUMBER_LOGICAL_SECTORS; // Bugfix 4.11
        DataWritten->int_data[0] = DiskID;
        DataWritten->int_data[1] = CheckValue;
        DataWritten->int_data[2] = Sector;
        DataWritten->int_data[3] = (int) OurProcessID;
        PHYSICAL_DISK_WRITE(DiskID, Sector, (char* )(DataWritten->char_data));

        // Now read back the same data.  Note that we assume the
        // DiskID and Sector have not been modified by the previous
        // call.
        PHYSICAL_DISK_READ(DiskID, Sector, (char* )(DataRead->char_data));

        if ((DataRead->int_data[0] != DataWritten->int_data[0])
                || (DataRead->int_data[1] != DataWritten->int_data[1])
                || (DataRead->int_data[2] != DataWritten->int_data[2])
                || (DataRead->int_data[3] != DataWritten->int_data[3])) {
            aprintf("ERROR in Test D, Part 1:  Process = %ld, Disk = %ld, Sector = %ld\n",
                    OurProcessID, DiskID, Sector);
            aprintf("Written:  %d,  %d,  %d,  %d\n", DataWritten->int_data[0],
                    DataWritten->int_data[1], DataWritten->int_data[2],
                    DataWritten->int_data[3]);
            aprintf("Read:     %d,  %d,  %d,  %d\n", DataRead->int_data[0],
                    DataRead->int_data[1], DataRead->int_data[2],
                    DataRead->int_data[3]);
        } else if ((Iterations % DISPLAY_GRANULARITY_D) == 0) {
            aprintf( "TEST D:  SUCCESS READING  Pid = %ld, DiskID =%ld, Sector = %ld\n",
                    OurProcessID, DiskID, Sector);
        }
    }   // End of for loop

// Now read back the data we've previously written and read

    aprintf("TEST D: Pid = %ld, Reading back data: \n", OurProcessID);

    for (Iterations = 0; Iterations < TESTD_LOOPS; Iterations++) {
        DiskID = (OurProcessID / 2) % MAX_NUMBER_OF_DISKS;
        Sector = OurProcessID + (Iterations * 17) % NUMBER_LOGICAL_SECTORS; // Bugfix 4.11
        DataWritten->int_data[0] = DiskID;
        DataWritten->int_data[1] = CheckValue;
        DataWritten->int_data[2] = Sector;
        DataWritten->int_data[3] = (int) OurProcessID;

        PHYSICAL_DISK_READ(DiskID, Sector, (char* )(DataRead->char_data));

        if ((DataRead->int_data[0] != DataWritten->int_data[0])
                || (DataRead->int_data[1] != DataWritten->int_data[1])
                || (DataRead->int_data[2] != DataWritten->int_data[2])
                || (DataRead->int_data[3] != DataWritten->int_data[3])) {
            aprintf("ERROR in Test D, Part 1:  Process = %ld, Disk = %ld, Sector = %ld\n",
                    OurProcessID, DiskID, Sector);
            aprintf("Written:  %d,  %d,  %d,  %d\n", DataWritten->int_data[0],
                    DataWritten->int_data[1], DataWritten->int_data[2],
                    DataWritten->int_data[3]);
            aprintf("Read:     %d,  %d,  %d,  %d\n", DataRead->int_data[0],
                    DataRead->int_data[1], DataRead->int_data[2],
                    DataRead->int_data[3]);
        } else if ((Iterations % DISPLAY_GRANULARITY_D) == 0) {
            aprintf(
                    "TEST D:  SUCCESS READING  Pid = %ld, DiskID =%ld, Sector = %ld\n",
                    OurProcessID, DiskID, Sector);
        }

    }   // End of for loop

    GET_TIME_OF_DAY(&CurrentTime);
    aprintf("TEST D: PID %2ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
    TERMINATE_PROCESS(-1, &ErrorReturned);
}           // End of testD
/**************************************************************************
 TestX
 This test is used as a target by the process creation programs.
 It has the virtue of causing lots of rescheduling activity in
 a relatively random way.

 **************************************************************************/
#define         NUMBER_OF_TESTX_ITERATIONS     10

void testX(void) {
    long OurProcessID;
    //long RandomSeed;
    long StartingTime;
    long EndingTime;
    long RandomSleep = 17;
    long ErrorReturned;
    int Iterations;
    GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
    aprintf("Release %s:TestX: Pid %ld\n", TEST_VERSION, OurProcessID);

    for (Iterations = 0; Iterations < NUMBER_OF_TESTX_ITERATIONS;
            Iterations++) {
        GET_TIME_OF_DAY(&StartingTime);
        RandomSleep = (RandomSleep * StartingTime) % 343;
        // Prevent sleep from being 0
        if (RandomSleep == 0)
            RandomSleep++;
        SLEEP(RandomSleep);
        GET_TIME_OF_DAY(&EndingTime);
        aprintf("TestX: Pid = %d, Time Now = %5d, Sleep Time = %ld, Latency Time = %d\n",
                (int) OurProcessID, (int)EndingTime, RandomSleep,
                (int) (EndingTime - StartingTime - RandomSleep));  // Time after interrupt
    }
    aprintf("TEST X, PID %2ld, Ends at Time %ld\n", OurProcessID, EndingTime);

    TERMINATE_PROCESS(-1, &ErrorReturned);
    aprintf("ERROR: TestX should be terminated but isn't.\n");

}            // End of testX

/**************************************************************************
 testZ
 File System calls (Open, Write, Read, Close)
 This is a child process of a test that has first formatted a disk,
 created multiple sub-directories and has then called multiple copies
 of testZ.
 test Z  does the following:
 1.  Open root directory in order to set Current Directory
 2.  Open a child directory of root based on this process PID which
 makes that child directory the Current Directory of this process.
 3.  Open two different files in the Current Directory and read/write
 to those files.
 **************************************************************************/
void testZ(void) {
	long OurProcessID;
	long EndingTime;
	long ErrorReturned;
	char Directoryname[8], Filename[8];
	char WriteBuffer[16], ReadBuffer[16];
	int Index, Index2;
	int WhichFile;
	long DiskID = 1;
	long Inode;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: TestZ: Pid %ld\n", TEST_VERSION, OurProcessID);

// Define the unique names used by this process
	sprintf(Directoryname, "Child%1d", (int) OurProcessID % TEST26_CHILDREN);

// Open the root directory and then the child directory
	OPEN_DIR((long )DiskID, "root", &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR");
	OPEN_DIR(-1, Directoryname, &ErrorReturned);
	SuccessExpected(ErrorReturned, "OPEN_DIR");

// Open the files and read/write to them.
// From two separate files
	for (WhichFile = 0; WhichFile <= 1; WhichFile++) {
		sprintf(Filename, "F%1d-%1d", (int) (OurProcessID % TEST26_CHILDREN),
				(int) WhichFile);

		// Open the existing file
		OPEN_FILE(Filename, &Inode, &ErrorReturned);
		SuccessExpected(ErrorReturned, "OPEN_FILE");
		aprintf("Completed system call OPEN_FILE\n");

		// Write a series of blocks to the file
		for (Index = 0; Index < 7; Index++) {
			WriteBuffer[0] = OurProcessID % TEST26_CHILDREN;
			for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
				WriteBuffer[Index2] = Index;
			}
			WRITE_FILE(Inode, (long )Index, &WriteBuffer, &ErrorReturned);
			SuccessExpected(ErrorReturned, "WRITE_FILE");
		}
		CLOSE_FILE(Inode, &ErrorReturned);
		SuccessExpected(ErrorReturned, "CLOSE_FILE");

		OPEN_FILE(Filename, &Inode, &ErrorReturned);
		// Read a series of blocks from the file
		for (Index = 0; Index < 7; Index++) {
			WriteBuffer[0] = OurProcessID % TEST26_CHILDREN;
			for (Index2 = 1; Index2 < PGSIZE ; Index2++) {
				WriteBuffer[Index2] = Index;
			}
			READ_FILE(Inode, (long )Index, &ReadBuffer, &ErrorReturned);
			SuccessExpected(ErrorReturned, "READ_FILE");
			ErrorReturned = FALSE;
			for (Index2 = 0; Index2 < PGSIZE ; Index2++) {
				if (ReadBuffer[Index2] != WriteBuffer[Index2])
					ErrorReturned = TRUE;
			}
			if (ErrorReturned == TRUE) {
				aprintf("There was an error in ReadFile - ");
				aprintf("we didn't read back the data successfully\n");
			} else {
				aprintf("TestZ - Successfully read logical block %d\n", Index);
			}
		}
		CLOSE_FILE(Inode, &ErrorReturned);
	}   // for WhichFile

	GET_TIME_OF_DAY(&EndingTime);
	aprintf("TEST Z, PID %ld, Ends at Time %ld\n", OurProcessID, EndingTime);

	TERMINATE_PROCESS(-1, &ErrorReturned); // Terminate ourselves

}            // End of testZ

/**************************************************************************
 TestM
 Touches a number of logical pages - but the number of pages touched
 will fit into the physical memory available.
 The addresses accessed are pseudo-random distributed in a non-uniform manner.
 **************************************************************************/

#define    STEP_SIZE               NUMBER_VIRTUAL_PAGES/(4 * NUMBER_PHYSICAL_PAGES )
#define    DISPLAY_GRANULARITY_M   32 * STEP_SIZE
#define    TOTAL_ITERATIONS        256
#define    SLEEP_GRANULARITY        16

void testM(void) {
	long OurProcessID;
	long MemoryAddress;
	long DataWritten = 0;
	long DataRead = 0;
	long CurrentTime;
	long ErrorReturned;

	int Iteration, MixItUp;
	int AddressesWritten[NUMBER_VIRTUAL_PAGES];

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: TestM: Pid %ld\n", TEST_VERSION, OurProcessID);

	for (Iteration = 0; Iteration < TOTAL_ITERATIONS; Iteration++)
		AddressesWritten[Iteration] = 0;
	for (Iteration = 0; Iteration < TOTAL_ITERATIONS; Iteration++) {
		GetSkewedRandomNumber(&MemoryAddress, OurProcessID, 90); // Generate Address    Bugfix 4.12
		MemoryAddress = 16 * MemoryAddress;      // Put address on page boundary
		AddressesWritten[Iteration] = MemoryAddress; // Keep record of location written
		DataWritten = (PGSIZE * MemoryAddress) + OurProcessID; // Generate Data    Bugfix 4.12
		MEM_WRITE(MemoryAddress, &DataWritten);       // Write the data
		// printf("Iteration = %d, Address = %d\n", Iteration, MemoryAddress);  // For debugging
		MEM_READ(MemoryAddress, &DataRead); // Read back data

		if (Iteration % DISPLAY_GRANULARITY_M == 0)
			aprintf("PID= %ld  address= %6ld   written= %6ld   read= %6ld\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten) {  // Written = Read?
			aprintf("AN ERROR HAS OCCURRED ON 1ST READBACK.\n");
			aprintf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		}

		// It makes life more fun!! to write the data again
		MEM_WRITE(MemoryAddress, &DataWritten); // Write the data

		// When we run testM, there are no physical faults, so the test
		// will run with no intervention.  When we want to run multiple
		// testM, the behavior is such that each runs by itself, terminates,
		// and then the next testM starts up.  But we want memory contention,
		// so we put a sleep here so that other testM processes will be
		// schedule.
		if ((Iteration % SLEEP_GRANULARITY) == 0)
			SLEEP(10);

	}    // End of for loop

// Now read back the data we've written and paged
// We try to jump around a bit in choosing addresses we read back
	aprintf("Reading back data: TEST M, PID %ld.\n", OurProcessID);

	for (Iteration = 0; Iteration < TOTAL_ITERATIONS; Iteration++) {
		if (!(Iteration % 2))      // Bugfix 4.11
			MixItUp = Iteration;
		else
			MixItUp = TOTAL_ITERATIONS - Iteration;
		MemoryAddress = AddressesWritten[MixItUp];  // Get location we wrote
		DataWritten = (PGSIZE * MemoryAddress) + OurProcessID; // Generate Data    Bugfix 4.12
		MEM_READ(MemoryAddress, &DataRead);      // Read back data

		if (Iteration % DISPLAY_GRANULARITY_M == 0)
			aprintf("PID= %ld  address= %6ld   written= %6ld   read= %6ld\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten) {  // Written = Read?
			aprintf("AN ERROR HAS OCCURRED ON 2ND READBACK.\n");
			aprintf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		}
	}    // End of for loop
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST M:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);      // Added 12/1/2013
}                                           // End of testM
/**************************************************************************

 TestE

 is used as a target by the message send/receive programs.
 All it does is send back the same message it received to the
 same sender.

 **************************************************************************/

typedef struct {
    long     target_pid;
    long     source_pid;
    long     actual_source_pid;
    long     send_length;
    long     receive_length;
    long     actual_senders_length;
    char     msg_buffer[LEGAL_MESSAGE_LENGTH ];
    char     msg_sent[LEGAL_MESSAGE_LENGTH ];
} TESTE_DATA;

long   OurProcessID;
long   ErrorReturned;

void testE(void) {
    TESTE_DATA *td;                          // Use as ptr to data

    // Here we maintain the data to be used by this process when running
    // on this routine.  This code should be re-entrant.

    td = (TESTE_DATA *) calloc(1, sizeof(TESTE_DATA));
    if (td == 0) {
        printf("Something screwed up allocating space in testE\n");
    }

    GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
    SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
    printf("Release %s:Test 1j_echo: Pid %ld\n", CURRENT_REL, OurProcessID);

    while (1) {       // Loop forever.
        td->source_pid = -1;
        td->receive_length = LEGAL_MESSAGE_LENGTH;
        RECEIVE_MESSAGE(td->source_pid, td->msg_buffer, td->receive_length,
                &(td->actual_senders_length), &(td->actual_source_pid),
                &ErrorReturned);
        SuccessExpected(ErrorReturned, "RECEIVE_MESSAGE");

        printf("Receive from PID = %ld: length = %ld: msg = %s:\n",
                td->actual_source_pid, td->actual_senders_length,
                td->msg_buffer);

        td->target_pid = td->actual_source_pid;
        strcpy(td->msg_sent, td->msg_buffer);
        td->send_length = td->actual_senders_length;
        SEND_MESSAGE(td->target_pid, td->msg_sent, td->send_length, &ErrorReturned);
        SuccessExpected(ErrorReturned, "SEND_MESSAGE");
    }     // End of while

}                                               // End of testE

/**************************************************************************
 ErrorExpected    and    SuccessExpected

 These routines simply handle the display of success/error data.

 **************************************************************************/

void ErrorExpected(INT32 ErrorCode, char sys_call[]) {
	if (ErrorCode == ERR_SUCCESS) {
		aprintf("Program ERRONEOUSLY returned success for call %s.\n", sys_call);
	} else
		aprintf("Program correctly returned an error: %d\n", ErrorCode);

}                      // End of ErrorExpected

void SuccessExpected(INT32 ErrorCode, char sys_call[]) {
	if (ErrorCode != ERR_SUCCESS) {
		aprintf("Program ERRONEOUSLY returned error %d for call %s.\n", ErrorCode, sys_call);
	} else
		aprintf("Program correctly returned success for %s.\n", sys_call);

}                      // End of SuccessExpected

/**************************************************************************
 Test41 exercises a simple memory write and read

 In global.h, there's a variable  DO_MEMORY_DEBUG.   Switching it to
 TRUE will allow you to see what the memory system thinks is happening.
 WARNING - it's verbose -- and I don't want to see such output - it's
 strictly for your debugging pleasure.
 **************************************************************************/
void test41(void) {
	long OurProcessID;
	long MemoryAddress;
	INT32 DataWritten;
	INT32 DataRead;
	long ErrorReturned;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);

	aprintf("Release %s: test41: Pid %ld\n", TEST_VERSION, OurProcessID);
	MemoryAddress = 412;
	DataWritten = MemoryAddress + OurProcessID;
	MEM_WRITE(MemoryAddress, &DataWritten);

	MEM_READ(MemoryAddress, &DataRead);

	aprintf("PID= %ld  address= %ld   written= %d   read= %d\n", OurProcessID,
			MemoryAddress, DataWritten, DataRead);
	if (DataRead != DataWritten)
		aprintf("AN ERROR HAS OCCURRED.\n");
	TERMINATE_PROCESS(-1, &ErrorReturned);
}                   // End of test41

/**************************************************************************
 Test42
 Exercises simple memory writes and reads.  Watch out, the addresses
 used are diabolical and are designed to show unusual features of your
 memory management system.

 We do sanity checks - after each read/write pair, we will
 read back the first set of data to make sure it's still there.
 **************************************************************************/
#define         TEST_DATA_SIZE          (INT16)7

void test42(void) {
    long OurProcessID;
    long FirstMemoryAddress;
    INT32 FirstDataWritten;
    INT32 FirstDataRead;
    long MemoryAddress;
    INT32 DataWritten;
    INT32 DataRead;
    long ErrorReturned;
    INT16 TestDataIndex = 0;

// This is an array containing the memory address we are accessing
    INT32 test_data[TEST_DATA_SIZE ] = { 0, PGSIZE/4, PGSIZE,
            PGSIZE + PGSIZE/4, 3 * PGSIZE,
            (NUMBER_VIRTUAL_PAGES - 1) * PGSIZE,
            NUMBER_VIRTUAL_PAGES * PGSIZE - 2 };

    GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
    aprintf("Release %s: test42: Pid %ld\n", TEST_VERSION, OurProcessID);

// Try a simple memory write
    FirstMemoryAddress = 5 * PGSIZE;
    FirstDataWritten = FirstMemoryAddress + OurProcessID + 7;
    MEM_WRITE(FirstMemoryAddress, &FirstDataWritten);

// Loop through all the memory addresses defined in the array
    while (TRUE ) {
        MemoryAddress = test_data[TestDataIndex];
        DataWritten = MemoryAddress + OurProcessID + 27;
        MEM_WRITE(MemoryAddress, &DataWritten);

        MEM_READ(MemoryAddress, &DataRead);

        aprintf("PID= %ld  address= %ld  written= %d   read= %d\n", OurProcessID,
                MemoryAddress, DataWritten, DataRead);
        if (DataRead != DataWritten)
            aprintf("AN ERROR HAS OCCURRED.\n");

        //      Go back and check earlier write
        MEM_READ(FirstMemoryAddress, &FirstDataRead);

        aprintf("PID= %ld  address= %ld   written= %d   read= %d\n",
                OurProcessID, FirstMemoryAddress, FirstDataWritten,
                FirstDataRead);
        if (FirstDataRead != FirstDataWritten)
            aprintf("AN ERROR HAS OCCURRED.\n");
        TestDataIndex++;
    }
}                            // End of test42

/**************************************************************************
 Test43
 Exercises a memory pattern.  It does this by calling a single
 instance of testM.  All of the touched pages SHOULD fit into physical memory.
 **************************************************************************/
void test43(void) {
	long CurrentTime;
	long ErrorReturned;
	long OurProcessID;
	long ProcessID;
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
        aprintf("Release %s: test43: Pid %ld\n", TEST_VERSION, OurProcessID);

	// Create a process to perform many actions
	CREATE_PROCESS("testM", testM, 10, &ProcessID, &ErrorReturned);

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("testM", &ProcessID, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 43:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-2, &ErrorReturned);
}           // End of test43

/**************************************************************************
 Test44
 Causes extensive page replacement, but reuses pages.
 It is touching more logical pages than there are physical pages.
 This program will terminate, but it might take a while.
 Test45 will require that you do a format and use the resulting swap area to
 hold the written out pages.  You MAY want to design your swap area here
 to work in that same way.
 **************************************************************************/

#define                 LOOP_COUNT                    4000
#define                 DISPLAY_GRANULARITY44          200
#define                 LOGICAL_PAGES_TO_TOUCH       6 * NUMBER_PHYSICAL_PAGES

// This structure keeps track of which addresses have been touched.
// We're accessing random sparse locations, so not all memory is accessed.
// Indeterminate results occur if we read memory that hasn't been written.
typedef struct {
	INT16 page_touched[LOOP_COUNT];
} MEMORY_TOUCHED_RECORD;

void test44(void) {
	long OurProcessID;
	long PageNumber;
	long MemoryAddress;
	int DataWritten = 0;
	int DataRead = 0;
	long ErrorReturned;
	long CurrentTime;
	int  DontPrintStatistics = 0;  // See the call to Test44_Statistics
	int  DoPrintStatistics   = 1;
	MEMORY_TOUCHED_RECORD *mtr;
	long Index, Loops;

	mtr = (MEMORY_TOUCHED_RECORD *) calloc(1, sizeof(MEMORY_TOUCHED_RECORD));

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
        aprintf("Release %s: test44: Pid %ld\n", TEST_VERSION, OurProcessID);

	for (Index = 0; Index < LOOP_COUNT; Index++) // Bugfix Rel 4.03  12/1/2013
		mtr->page_touched[Index] = 0;
	for (Loops = 0; Loops < LOOP_COUNT; Loops++) {
		// Get a random page number
		GetSkewedRandomNumber(&PageNumber, OurProcessID, LOGICAL_PAGES_TO_TOUCH);
		MemoryAddress = PGSIZE * PageNumber; // Convert page to address
		DataWritten = MemoryAddress + OurProcessID; // Generate data for page
		MEM_WRITE(MemoryAddress, &DataWritten);
		// Write it again, just as a test - a fault may or may not occur
		MEM_WRITE(MemoryAddress, &DataWritten);

		// Read it back and make sure it's the same
		MEM_READ(MemoryAddress, &DataRead);
		if (Loops % DISPLAY_GRANULARITY44 == 0)
			aprintf("PID= %ld  address= %4ld   written= %4d   read= %4d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten) {
			aprintf("AN ERROR HAS OCCURRED: READ NOT EQUAL WRITE.\n");
			aprintf("PID= %ld  address= %ld   written= %d   read= %d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		}
		// Record in our data-base that we've accessed this page
		mtr->page_touched[(short) Loops] = PageNumber;
		Test44_Statistics(OurProcessID, PageNumber, DontPrintStatistics);

	}   // End of for Loops

	for (Loops = 0; Loops < LOOP_COUNT; Loops++) {

		// We can only read back from pages we've previously
		// written to, so find out which pages those are.
		PageNumber = mtr->page_touched[(short) Loops];
		MemoryAddress = PGSIZE * PageNumber;        // Convert page to address
		DataWritten = MemoryAddress + OurProcessID; // Expected read
		MEM_READ(MemoryAddress, &DataRead);

		if (Loops % DISPLAY_GRANULARITY44 == 0)
			aprintf("PID= %ld  address= %4ld   written= %4d   read= %4d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten) {
			aprintf("ERROR HAS OCCURRED: READ NOT SAME AS WRITE.\n");
			aprintf("PID= %ld  address= %4ld   written= %4d   read= %4d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		}
	}   // End of for Loops

// We've completed reading back everything
	aprintf("TEST 44, PID %ld, HAS COMPLETED %ld Loops\n", OurProcessID, Loops);
	Test44_Statistics(OurProcessID, 0, DoPrintStatistics);
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST44:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

}                                 // End of test44

/**************************************************************************
 Test45
 Tests multiple copies of testM running simultaneously.
 Since each of these testM processes use up most of physical memory, the
 combination will cause extensive page faults.
 Test44 caused extensive page faults, but your code could put the
 pages that were written out, wherever you wanted.  In this test, there
 is a disk Format which means there is a defined swap area.  You are to
 use that swap area to contain the paged-out memory.
 **************************************************************************/

void test45(void) {
	long CurrentTime;
	long ProcessID;             // Contains a Process ID
	long ReturnedPID;           // Value of PID returned by System Call
	long OurProcessID;
	long ErrorReturned;
	long Iteration;
	long ChildPriority = 10;
	char ProcessName[16];       // Holds generated process name
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
        aprintf("Release %s: test45: Pid %ld\n", TEST_VERSION, OurProcessID);
	FORMAT(1, &ErrorReturned);
	CHECK_DISK(1, &ErrorReturned);
	for (Iteration = 0; Iteration < 5; Iteration++) {
		sprintf(ProcessName, "Test45_%ld", Iteration);
		aprintf("Creating process \"%s\"\n", ProcessName);
		CREATE_PROCESS(ProcessName, testM, ChildPriority, &ProcessID,
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	}

	// Now we sleep, see if one of the five processes has terminated, and
	// continue the cycle until all of them are gone.  This allows the testM
	// processes to exhibit scheduling.
	// We know that the process terminated when we do a GET_PROCESS_ID and
	// receive an error on the system call.

	ErrorReturned = ERR_SUCCESS;
	for (Iteration = 0; Iteration < 5; Iteration++) {
		ErrorReturned = ERR_SUCCESS;
		sprintf(ProcessName, "Test45_%ld", Iteration);
		while (ErrorReturned == ERR_SUCCESS) {
			GET_PROCESS_ID(ProcessName, &ReturnedPID, &ErrorReturned);
			if (ErrorReturned != ERR_SUCCESS)
				break;
			SLEEP(SleepTime);
		}
	}
	// When we get here, all child processes have terminated
	GET_TIME_OF_DAY(&CurrentTime);
	CHECK_DISK(1, &ErrorReturned);
	aprintf("TEST 45:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned); // Terminate all
}    // End test 45

/**************************************************************************
 Test46
 Tests multiple copies of testZ AND testM running simultaneously.
 This exercises both the disk file system and the disk usage for the
 paging mechanism.
 IF you have planned well, this test should just run with no modifications.
 **************************************************************************/

void test46(void) {
	long CurrentTime;
	long ProcessID;             // Contains a Process ID
	long ReturnedPID;           // Value of PID returned by System Call
	long DiskID = 1;            // Disk ID - MUST be the same as in test12
	long OurProcessID;
	long ErrorReturned;
	long ChildPriority = 10;
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
        aprintf("Release %s: test46: Pid %ld\n", TEST_VERSION, OurProcessID);

	CREATE_PROCESS("Test46-26", test26, ChildPriority, &ProcessID,
			&ErrorReturned);

	// We wait here until the newly created process has completed a Format.
	// When the format is completed, a root directory will have been created.
	// Only then will the OPEN_DIR call be successful

	ErrorReturned = -1;              // Initialize this value to failure
	while (ErrorReturned != ERR_SUCCESS) {
		SLEEP(1000);
		aprintf("TEST 46 looking for root directory\n");
		OPEN_DIR(DiskID, "root", &ErrorReturned);
	}

	// This gives the formatting process a chance to settle down.
	SLEEP(1000);

	// Now that we know the disk has been formatted, do memory tests that
	// will place information in the swap area.  It is expected that this
	// swap area will be on the same disk as we did the format.
	aprintf("TEST46 has opened the root directory created by TEST 26\n");
	CREATE_PROCESS("Test46-45", test45, ChildPriority, &ProcessID,
			&ErrorReturned);

	// Now we sleep, waiting until both of our child processes have terminated.
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		GET_PROCESS_ID("Test46-26", &ReturnedPID, &ErrorReturned);
		SLEEP(SleepTime);
	}
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		GET_PROCESS_ID("Test46-45", &ReturnedPID, &ErrorReturned);
		SLEEP(SleepTime);
	}

	// When we get here, both child processes have terminated
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST 46:   Ends at Time %ld\n", CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned); // Terminate all
}    // End test 46

/**************************************************************************
 Test47 - Test45 in Multiprocessor mode
 This is just a placeholder.  To execute test47, run test45 with
 the argument 'm'
 **************************************************************************/
void test47(void) {
	long ErrorReturned;
	aprintf(
			"This is just a placeholder.  To execute test47, run test45 with the argument M.\n");
	TERMINATE_PROCESS(-1, &ErrorReturned)
}   // End test47

/**************************************************************************
 Test48 starts up a number of processes who do tests of shared area.

 This process doesn't do much - the real action is in testS
 **************************************************************************/


#define           MOST_FAVORABLE_PRIORITY                       1
#define           NUMBER_TEST48_PROCESSES                       5
#define           SLEEP_TIME_48                             10000

void test48(void) {
	long OurProcessID;     // PID of test48
	long ErrorReturned;    // Used as part of system calls
	long ProcessID;
	long CurrentTime;
	long Iteration;
	long ReturnedPID;
	long ChildPriority = 10;
	char ProcessName[16];       // Holds generated process name

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: test48: Pid %ld\n", TEST_VERSION, OurProcessID);

	for (Iteration = 0; Iteration < NUMBER_TEST48_PROCESSES; Iteration++) {
		sprintf(ProcessName, "Test48_%ld", Iteration);
		aprintf("Creating process \"%s\"\n", ProcessName);
		CREATE_PROCESS(ProcessName, testS, ChildPriority, &ProcessID,
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	}

// Loop here until the "S" final process terminate.

	for (Iteration = 0; Iteration < NUMBER_TEST48_PROCESSES; Iteration++) {
		ErrorReturned = ERR_SUCCESS;
		sprintf(ProcessName, "Test48_%ld", Iteration);
		while (ErrorReturned == ERR_SUCCESS) {
			GET_PROCESS_ID(ProcessName, &ReturnedPID, &ErrorReturned);
			if (ErrorReturned != ERR_SUCCESS)
				break;
			SLEEP(SLEEP_TIME_48);
		}
	}
	// When we get here, all child processes have terminated
	GET_TIME_OF_DAY(&CurrentTime);
	aprintf("TEST48:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                  // End of test48

/**************************************************************************
 TestS - test shared memory usage.
 This test runs as multiple instances of processes; there are several
 processes who in turn manipulate shared memory.

 The algorithm used here flows as follows:

 o Get our PID and print it out.
 o Use our PID to determine the address at which to start shared
 area - every process will have a different starting address.
 o Define the shared area.
 o Fill in initial portions of the shared area by:
 + No locking required because we're modifying only our portion of
 the shared area.
 + Determine which location within shared area is ours by using the
 Shared ID returned by the DEFINE_SHARED_AREA.  The OS will return
 a unique ID for each caller.
 + Fill in portions of the shared area.
 o Sleep to let all testS PIDs start up.

 o If (shared_index == 0)   This is the MASTER Process

 o LOOP many times doing the following steps:
 + We DON'T need to lock the shared area since the SEND/RECEIVE
 act as synchronization.
 + Select a target process
 + Communicate back and forth with the target process.
 + SEND_MESSAGE( "next", ..... );
 END OF LOOP
 o Loop through each Slave process telling them to terminate

 o If (shared_index > 0)   This is a SLAVE Process
 o LOOP Many times until we get terminate message
 + RECEIVE_MESSAGE( "-1", ..... )
 + Read my mailbox and communicate with master
 + Print out lots of stuff
 + Do lots of sanity checks
 + If MASTER tells us to terminate, do so.
 END OF LOOP

 o If (shared_index == 0)   This is the MASTER Process
 o sleep                      Until everyone is done
 o print the whole shared structure

 o Terminate the process.

 **************************************************************************/

#define           NUMBER_MASTER_ITERATIONS    28
#define           PROC_INFO_STRUCT_TAG        1234
#define           SHARED_MEM_NAME             "almost_done!!\0"

// This is the per-process area for each of the processes participating
// in the shared memory.
typedef struct {
	INT32 structure_tag;      // Unique Tag so we know everything is right
	INT32 Pid;                // The PID of the slave owning this area
	INT32 TerminationRequest; // If non-zero, process should terminate.
	INT32 MailboxToMaster;    // Data sent from Slave to Master
	INT32 MailboxToSlave;     // Data sent from Master To Slave
	INT32 WriterOfMailbox;    // PID of process who last wrote in this area
} PROCESS_INFO;

// The following structure will be laid on shared memory by using
// the MEM_ADJUST   macro

typedef struct {
	PROCESS_INFO proc_info[NUMBER_TEST48_PROCESSES + 1];
} SHARED_DATA;

// We use this local structure to gather together the information we
// have for this process.
typedef struct {
	INT32 StartingAddressOfSharedArea;     // Local Virtual Address
	INT32 PagesInSharedArea;               // Size of shared area
	// The tag is how the OS knows to put all in same shared area
	char AreaTag[32];
	// Unique number supplied by OS. First must be 0 and the
	// return values must be monotonically increasing for each
	// process that's doing the Shared Memory request.
	INT32 OurSharedID;
	INT32 TargetShared;                  // Shared ID of slave we're sending to
	long TargetPid;                      // Pid of slave we're sending to
	INT32 ErrorReturned;
	long SourcePid;
	char ReceiveBuffer[20];
	long MessageReceiveLength;
	INT32 MessageSendLength;
	INT32 MessageSenderPid;
} LOCAL_DATA;

// This MEM_ADJUST macro allows us to overlay the SHARED_DATA structure
// onto the shared memory we've defined.  It generates an address
// appropriate for use by MEM_READ and MEM_WRITE.

#define         MEM_ADJUST( arg )                                   \
  (long)&(shared_ptr->arg) - (long)(shared_ptr)                     \
                      + (long)ld->StartingAddressOfSharedArea

#define         MEM_ADJUST2( shared, local, arg )                   \
  (long)&(shared->arg) - (long)(shared)                             \
                      + (long)local->StartingAddressOfSharedArea

// This allows us to print out the shared memory for debugging purposes
void PrintTestSMemory(SHARED_DATA *sp, LOCAL_DATA *ld) {
	int Index;
	INT32 Data1, Data2, Data3, Data4, Data5;

	aprintf("\nNumber of Masters + Slaves = %d\n",
			(int) NUMBER_TEST48_PROCESSES);

	for (Index = 0; Index < NUMBER_TEST48_PROCESSES; Index++) {
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].structure_tag),
				&Data1);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].Pid ), &Data2);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].MailboxToMaster ),
				&Data3);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].MailboxToSlave ),
				&Data4);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].WriterOfMailbox),
				&Data5);

		aprintf("Mailbox info for index %d:\n", Index);
		aprintf("\tIndex = %d, Struct Tag = %d,  ", Index, Data1);
		aprintf(" Pid =  %d,  Mail To Master = %d, ", Data2, Data3);
		aprintf(" Mail To Slave =  %d,  Writer Of Mailbox = %d\n", Data4, Data5);
	}          // END of for Index
}      // End of PrintTest2SMemory

void testS(void) {
	// The declaration of shared_ptr is only for use by MEM_ADJUST macro.
	// It points to an arbitrary location - but that's OK because we never
	// actually use the result of the pointer, only its offset.

	long OurProcessID;
	long ErrorReturned;
	LOCAL_DATA *ld;
	SHARED_DATA *shared_ptr = 0;
	int Index;
	INT32 ReadWriteData;    // Used to move to and from shared memory

	ld = (LOCAL_DATA *) calloc(1, sizeof(LOCAL_DATA));
	if (ld == 0) {
		aprintf("Unable to allocate memory in TEST S\n");
	}
	strcpy(ld->AreaTag, SHARED_MEM_NAME);

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	aprintf("Release %s: testS: Pid %ld\n", TEST_VERSION, OurProcessID);

	// As an interesting wrinkle, each process should start
	// its shared region at a somewhat different virtual address;
	// determine that here.
	ld->StartingAddressOfSharedArea = (OurProcessID % 17) * PGSIZE;

	// This is the number of pages required in the shared area.
	ld->PagesInSharedArea = sizeof(SHARED_DATA) / PGSIZE + 1;

	// Now ask the OS to map us into the shared area
	DEFINE_SHARED_AREA((long )ld->StartingAddressOfSharedArea, // Input - our virtual address
			(long)ld->PagesInSharedArea,// Input - pages to map
			(long)ld->AreaTag,// Input - ID for this shared area
			&ld->OurSharedID,// Output - Unique shared ID
			&ld->ErrorReturned);              // Output - any error
	SuccessExpected(ld->ErrorReturned, "DEFINE_SHARED_AREA");

	ReadWriteData = PROC_INFO_STRUCT_TAG; // Sanity data
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].structure_tag),
			&ReadWriteData);

	ReadWriteData = OurProcessID; // Store PID in our slot
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].Pid), &ReadWriteData);
	ReadWriteData = 0;         // Initialize this counter
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToMaster),
			&ReadWriteData);
	ReadWriteData = 0;         // Initialize this counter
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToSlave),
			&ReadWriteData);

	//  This is the code used ONLY by the MASTER Process
	if (ld->OurSharedID == 0) {  //   We are the MASTER Process
		// Loop here and send message to each slave
		for (Index = 0; Index < NUMBER_MASTER_ITERATIONS; Index++) {

			// Wait for all slaves to start up - we assume after the sleep
			// that the slaves are no longer modifying their shared areas.
			SLEEP(1000); // Wait for slaves to start

			// Get slave ID we're going to work with - be careful here - the
			// code further on depends on THIS algorithm
			ld->TargetShared = (Index % (NUMBER_TEST48_PROCESSES - 1)) + 1;

			// Read the memory of that slave to make sure it's OK
			MEM_READ(MEM_ADJUST(proc_info[ld->TargetShared].structure_tag),
					&ReadWriteData);
			if (ReadWriteData != PROC_INFO_STRUCT_TAG) {
				aprintf("We should see a structure tag, but did not\n");
				aprintf("This means that this memory is not mapped \n");
				aprintf("consistent with the memory used by the writer\n");
				aprintf(
						"of this structure.  It could be a page table problem.\n");
			}
			// Get the pid of the process we're working with
			MEM_READ(MEM_ADJUST(proc_info[ld->TargetShared].Pid),
					&ld->TargetPid);

			// We're sending data to the Slave
			MEM_WRITE(MEM_ADJUST(proc_info[ld->TargetShared].MailboxToSlave),
					&Index);
			MEM_WRITE(MEM_ADJUST(proc_info[ld->TargetShared].WriterOfMailbox),
					&OurProcessID);
			ReadWriteData = 0;   // Do NOT terminate
			MEM_WRITE(
					MEM_ADJUST(proc_info[ld->TargetShared].TerminationRequest),
					&ReadWriteData);
			aprintf("Sender %ld to Receiver %d passing data %d\n", OurProcessID,
					(int) ld->TargetPid, Index);

			// Check the iteration count of the slave.  If it tells us it has done a
			// certain number of iterations, then tell it to terminate itself.
			MEM_READ(MEM_ADJUST(proc_info[ld->TargetShared].MailboxToMaster),
					&ReadWriteData);
			if (ReadWriteData
					>= (NUMBER_MASTER_ITERATIONS / (NUMBER_TEST48_PROCESSES - 1))
							- 1) {
				ReadWriteData = 1;   // Do terminate
				MEM_WRITE(
						MEM_ADJUST(proc_info[ld->TargetShared].TerminationRequest),
						&ReadWriteData);
				aprintf("Master is sending termination message to PID %d\n",
						(int) ld->TargetPid);
			}

			// Now we are done with this slave - send it a message which will start it working.
			// The iterations may not be quite right - we may be sending a message to a
			// process that's already terminated, but that's OK.
			// Note that we're not sending data, merely an "alert"
			SEND_MESSAGE(ld->TargetPid, " ", 1, &ld->ErrorReturned); // Bugfix, July 2015
		}     // End of For Index
	}     // End of MASTER PROCESS

	// This is the start of the slave process work
	if (ld->OurSharedID != 0) {  //   We are a SLAVE Process
		// The slaves keep going forever until the master tells them to quit
		while (TRUE ) {

			ld->SourcePid = -1; // From anyone
			ld->MessageReceiveLength = 20;
			RECEIVE_MESSAGE(ld->SourcePid, ld->ReceiveBuffer,
					ld->MessageReceiveLength, &ld->MessageSendLength,
					&ld->MessageSenderPid, &ld->ErrorReturned);
			SuccessExpected(ld->ErrorReturned, "RECEIVE_MESSAGE");

			// Make sure we have our memory mapped correctly
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].structure_tag),
					&ReadWriteData);
			if (ReadWriteData != PROC_INFO_STRUCT_TAG) {
				aprintf("We should see a structure tag, but did not.\n");
				aprintf("This means that this memory is not mapped \n");
				aprintf("consistent with the memory used when WE wrote\n");
				aprintf("this structure.  It may be a page table problem.\n");
			}

			// Get the value placed in shared memory and compare it with the PID provided
			// by the messaging system.
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].WriterOfMailbox),
					&ReadWriteData);
			if (ReadWriteData != ld->MessageSenderPid) {
				aprintf("ERROR: ERROR: The sender PID, given by the \n");
				aprintf("RECEIVE_MESSAGE and by the mailbox, don't match\n");
			}

			// We're receiving data from the Master
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToSlave),
					&ReadWriteData);
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].WriterOfMailbox),
					&ld->MessageSenderPid);
			aprintf("Receiver %ld got message from %d passing data %d\n",
					OurProcessID, ld->MessageSenderPid, ReadWriteData);

			// See if we've been asked to terminate
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].TerminationRequest),
					&ReadWriteData);
			if (ReadWriteData > 0) {
				aprintf("Process %ld received termination message\n",
						OurProcessID);
				TERMINATE_PROCESS(-1, &ErrorReturned);
			}

			// Increment the number of iterations we've done.  This will ultimately lead
			// to the master telling us to terminate.
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToMaster),
					&ReadWriteData);
			ReadWriteData++;
			MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToMaster),
					&ReadWriteData);

		}  //End of while TRUE
	}      // End of SLAVE

	// The Master comes here and prints out the entire shared area

	if (ld->OurSharedID == 0) {  // The slaves should terminate before this.
		SLEEP(5000);                        // Wait for msgs to finish
		aprintf("Overview of shared area at completion of TEST S\n");
		PrintTestSMemory(shared_ptr, ld);
		TERMINATE_PROCESS(-1, &ErrorReturned);
	}              // END of if
	TERMINATE_PROCESS(-1, &ErrorReturned);

}                                // End of testS
/**************************************************************************

 test44_Statistics   This is designed to give an overview of how the
 paging algorithms are working.  It is especially useful when running
 test44 because it will enable us to better understand the global policy
 being used for allocating pages.

 **************************************************************************/

#define  MAX44_PID    20
#define  NUMBER_OF_PRINTS_PER_LINE  10
void Test44_Statistics(long Pid, long PageNumber, int Mode) {
    static short NotInitialized = TRUE;
    static int Counter[MAX44_PID];
    static int PagesTouched[MAX44_PID][NUMBER_VIRTUAL_PAGES];
    int i, j;
    int PrintsSoFar = 0;

    if (NotInitialized) {
        for (i = 0; i < MAX44_PID; i++)
            for (j = 0; j < NUMBER_VIRTUAL_PAGES; j++)
                PagesTouched[i][j] = 0;
        NotInitialized = FALSE;
    }
    if (Pid >= MAX44_PID) {
        aprintf("In Test44_Statistics - the pid entered, ");
        aprintf("%d, is larger than the maximum allowed", Pid);
        exit(0);
    }
    PagesTouched[Pid][PageNumber]++;  // Record the page used by this PID
    Counter[Pid]++;                   // Record we were here
    // It's time to print out a report
    if (Mode == 1) {
        aprintf("----- Report by TEST 44 - Pid %d -----\n", Pid);
        // Note we were called only for first loop in test44
        aprintf("Total number of memory requests = %d\n", 2 * Counter[Pid]);
        aprintf("Table shows VirtualPage[Number of Hits]\n");
        for (j = 0; j < NUMBER_VIRTUAL_PAGES; j++) {
            if (PagesTouched[Pid][j] > 0) {
                aprintf("%3d[%3d] ", j, PagesTouched[Pid][j]);
                PrintsSoFar++;
                if (PrintsSoFar >= NUMBER_OF_PRINTS_PER_LINE) {
                    aprintf("\n");
                    PrintsSoFar = 0;
                }
            }
        }
        aprintf("\n");
    }   // End if Mode == 1
}                 // End of Test44_Statistics

/**************************************************************************

 GetSkewedRandomNumber   Is a homegrown deterministic random
 number generator.  It produces  numbers that are NOT uniform across
 the allowed range.
 This is useful in picking page locations so that pages
 get reused and makes a LRU algorithm meaningful.
 This algorithm is VERY good for developing page replacement tests.
 July 2015 - optimized values so as to differentiate between random
 replacement and LRU replacement.  I saw a 10% improvement
 in fault rates when using LRU as compared to random replacement.

 This code was rewritten in Release 4.31
 **************************************************************************/
#define                 SKEWING_FACTOR          0.33
#define                 MY_RAND_MAX             32767
void GetSkewedRandomNumber(long *ReturnedValue, long Pid, long range) {
	static int FirstTime = TRUE;
	int RandomNumber;
	double Multiplier = range / pow( range, SKEWING_FACTOR );
	if (FirstTime) {
		srand(time(NULL));
		FirstTime = FALSE;
	}
	RandomNumber = (range * (rand() % 32767)) / 32767;
	RandomNumber = (int) (Multiplier * pow(RandomNumber, SKEWING_FACTOR));

	*ReturnedValue = RandomNumber;
} // End GetSkewedRandomNumber

/*****************************************************************
 testStartCode()
 A new thread (other than the initial thread) comes here the
 first time it's scheduled.
 *****************************************************************/
void testStartCode() {
	void (*routine)(void);
	routine = (void (*)(void)) Z502PrepareProcessForExecution();
	(*routine)();
// If we ever get here, it's because the thread ran to the end
// of a test program and wasn't terminated properly.
	aprintf("ERROR:  Simulation did not end correctly\n");
	exit(0);
}

/*****************************************************************
 main()
 This is the routine that will start running when the
 simulator is invoked.
 *****************************************************************/
int main(int argc, char *argv[]) {
	int i;
	for (i = 0; i < MAX_NUMBER_OF_USER_THREADS; i++) {
		Z502CreateUserThread(testStartCode);
	}

	osInit(argc, argv);
// We should NEVER return from this routine.  The result of
// osInit is to select a program to run, to start a process
// to execute that program, and NEVER RETURN!
	return (-1);
}    // End of main


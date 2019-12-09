/***************************************************************************
  QManager
  This manager looks after all queues - singly linked lists.

  WARNING:  You need to be using a lock for each of your Queues.
            Do NOT use this QManager without first locking the Q.

  The interfaces implemented here include:

  int  QCreate(char *QNameDescriptor);
      You must create a Queue before you can insert or remove items from the
      Queue.  You are allowed a maximum of MAX_QUEUES = 50 queues.
      Input: QNameDescriptor - a string containing the name you would
                 like to give the Q.  It's recommended you limit
		         this string to about 8 characters because you will
		         want to print it.  The max length is Q_MAX_NAME_LENGTH = 20
      Output: QID - The ID that describes this Q and is used in all
                 future references to this Q.  The Q Manager will report
                 an error by returning a value of -1.  You must check!

  int  QInsert(int QID, unsigned int QueueOrder, void *EnqueueingStructure);
      Enqueue an item on the designated Q.
      There is no limit to the number of items you can place on a queue.
      Input: QID - The ID that describes the target Q.
      Input: QueueOrder - The order in which the item will be enqueued.
                   Allowed values are from 0 to UINT_MAX = 	4294967295
                   Items are enqueued with the smallest value at the head.
                   If two or more items have the same QueueOrder, the
                        newest item will be enqueued AFTER the existing
                        items with that QueueOrder.
                   You may wish to use the routine QInsertOnTail() to
                        ensure you get the result you want.
      Input: EnqueueingStructure - Whatever you want to have enqueued.  It's
                   assumed this is the address of a structure.
      Output: The routine always returns 0.  But if there's an error
              the simulation ends.

   int  QInsertOnTail(int QID, void *EnqueueingStructure);
        Enqueue an item on the end of the designated Q.
        Input: QID - The ID that describes the target Q.
        Input: EnqueueingStructure - Whatever you want to have enqueued.  It's
                   assumed this is the address of a structure.
        Output: The routine always returns 0.  But if there's an error
              the simulation ends.

  void *QRemoveHead(int QID);
       Dequeue an item from the head of the designated Q.  The address
       of the item is returned to the caller and the item is removed
       from the Queue.
       Input: QID - The ID that describes the target Q.
       Output: The address of the structure that has been dequeued.
               If there is nothing on the Q, the return value = -1.

  void *QRemoveItem(int QID, void *EnqueueingStructure);
       Dequeue an item from the designated Q.  May or may not be the head.
       The address of the item is returned to the caller and the item
       is removed from the Queue.
       Input: QID - The ID that describes the target Q.
       Input: EnqueueingStructure - The address you are searching for.
       Output: The address of the first structure found that
               matches the EnqueueingStructure address and has been dequeued.
               If no matching item is found on the Q, the return value = -1.

  void *QNextItemInfo(int QID);
       Get information about the item on the head of the designated Q.
       The item is NOT removed from the Queue.
       Input: QID - The ID that describes the target Q.
       Output: The address of the structure that has been found on the head.
               If there is nothing on the Q, the return value = -1.

  void *QItemExists(int QID, void *EnqueueingStructure);
       Determine if the item is present on the Q.
       The item is NOT removed from the Queue.
       Input: QID - The ID that describes the target Q.
       Input: EnqueueingStructure - The address you are searching for.
       Output: The address of the structure that has been found.
               If the item is not found on the Q, the return value = -1.

  char *QGetName( int QID);
       Input: QID - The ID that describes the target Q.
       Output: A string with the QNameDescriptor you gave the Q
               when you created it.
  int GetNumberOfAllocatedQueues();
       Returns the number of Queues that have been allocated;
       Input:  Nothing
       Output: How many Queues have been allocated.
  void *QWalk(int QID, int QOrder);
       Returns the address of the "QOrderth" item on the Q.
       This is useful if you want to read what's on the Q.  Iterate
          as many times as necessary to get all items - keep going
          until you get a return value of -1.  Especially nice when
          you need to print the pids of your processes.
       The queue is unmodified by this routine.
       Input: QID - The ID that describes the target Q.
       Input: QOrder - the place of the item on the Q.  The first item
               on the Q is the 0th item.
       Output: The address of the structure that has been found.
               If the QOrder given is greater than the number of
               items on the Q, the return value = -1.

DEBUGGING YOUR USE OF THESE ROUTINES:
  This code has a constant Q_TRACE which is normally set to FALSE.
  If you set it to TRUE, you will get additional trace information.

  void  QPrint(int QID)
       Print out all details of the items on the designated Q.
       This should NOT be part of your completed code.
       The queue is unmodified by this routine.
       Input: QID - The ID that describes the target Q.
       Output:  Details are printed.  Here's an example:
Printing Q 0 with name Ready
Struct = 001D21E0, Order =          0, QdStruct = 40EA44, ... QNext = 001D21C8
Struct = 001D21C8, Order =          0, QdStruct = 40EA4C, ... QNext = 001D21B0
Struct = 001D21B0, Order =    1000000, QdStruct = 40EA54, ... QNext = 001D2198
Struct = 001D2198, Order =    1000000, QdStruct = 40EA5C, ... QNext = 001D2168
Struct = 001D2168, Order = 4294967295, QdStruct = 40EA64, ... QNext = 001D2180
Struct = 001D2180, Order = 4294967295, QdStruct = 40EA6C, ... QNext = FFFFFFFF
***************************************************************************/
/***************************************************************************
      Rel 4.50  Apr 2018  Initial release of the QueueManager
***************************************************************************/
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdarg.h>
#include    <limits.h>
#include    "global.h"
#include    "protos.h"
#include    "pcb.h"
#define    Q_TRACE                    FALSE
#define    Q_MAX_NAME_LENGTH          20
#define    Q_STRUCTURE_ID             57
#define    Q_HEAD_STRUCTURE_ID        53
#define    MAX_QUEUES                 50


//  These are the structures we use here to implement the Q's
typedef struct {
	void *queue;                         // Pointer to items on the queue
    int QID;                             // Which QID have we told the user this is
    char QName[Q_MAX_NAME_LENGTH];       // The name the user gave us for this Q
    int HeadStructID;
    //int ProcessAllocateID;
} Q_HEAD;

typedef struct {
    void *queue;                // Pointer to next QItem.
    unsigned int QueueOrder;    // For an ordered Q, the position in the Q
    void *QdStructure;          // What the caller gave us to hang to.
    int ItemStructID;
} Q_ITEM;

// Global Variables
Q_HEAD Queues[MAX_QUEUES];
int  NumberOfAllocatedQueues = 0;


// Internal Prototypes - used in this file only
void QProclaim(const char *format, ...);
void QCheckValidity( int QID, int QueueingOrder );
void QPanic(char *Text);

/**************************************************************************
***************************************************************************/
/**************************************************************************
  int  QCreate(char *QNameDescriptor);
      Input: QNameDescriptor - a string containing the name you would
                 like to give the Q.  It's recommended you limit
		 this string to about 8 characters because you will
		 want to print it.
      Output: QID - The ID that describes this Q and is used in all
                 future references to this Q.
		 If an error occurs, this value is -1.
***************************************************************************/
int  QCreate(char *QNameDescriptor)  {
    int ThisQ = NumberOfAllocatedQueues;
    // Check if too many Qs have been created.
    if (NumberOfAllocatedQueues >= MAX_QUEUES ) {
	    return -1;
    }
    // Check if name is too long
    if ( strlen( QNameDescriptor ) > Q_MAX_NAME_LENGTH )   {
	    return -1;
    }
    Queues[ThisQ].queue = (void *)-1;
    Queues[ThisQ].QID = NumberOfAllocatedQueues;

    strncpy(Queues[ThisQ].QName, QNameDescriptor, Q_MAX_NAME_LENGTH);
    Queues[ThisQ].HeadStructID = Q_HEAD_STRUCTURE_ID;
    //Queues[ThisQ].ProcessAllocateID = PROCESS_INITIAL_ID;
    NumberOfAllocatedQueues++;
    return( ThisQ );
}  // End of QCreate

/**************************************************************************
  QInsert(int QID, unsigned int QueueOrder, void *EnqueueingStructure);
      Enqueue an item on the designated Q.
      Input: QID - The ID that describes the target Q.
      Input: QueueOrder - The order in which the item will be enqueued.
                   Allowed values are from 0 to UINT_MAX = 	4294967295
                   Items are enqueued with the smallest value at the head.
                   If two or more items have the same QueueOrder, the
                        newest item will be enqueued AFTER the existing
                        items with that QueueOrder.
      Input: EnqueueingStructure - Whatever you want to have enqueued.  It's
                   assumed this is the address of a structure.
      Output: The function always returns 0.  But if there's an error
              the simulation ends.
***************************************************************************/
int  QInsert(int QID, unsigned int QueueOrder, void *EnqueueingStructure) {
    Q_ITEM *QItem;
    Q_ITEM *temp_ptr, *last_ptr;

    QProclaim("Entering QInsert:  QID = %d, QOrder = %d\n", QID, QueueOrder);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, QueueOrder );

    // Go to the special code that will place this item on the tail of the Q.
    if (QueueOrder == UINT_MAX)  {
    	QInsertOnTail( QID, EnqueueingStructure );
    	return 0;
    }
    QItem = (Q_ITEM *) malloc(sizeof(Q_ITEM));
    if (QItem == 0)
        QPanic("We didn't complete the malloc in QInsert.");

    QItem->queue       = (void *) -1;
    QItem->QueueOrder  = QueueOrder;
    QItem->QdStructure = EnqueueingStructure;
    QItem->ItemStructID= Q_STRUCTURE_ID;

    // Is there nothing on the Q?
    if ( Queues[QID].queue == (Q_ITEM *)-1) {
    	Queues[QID].queue = QItem;

    }  else {
    	last_ptr = (Q_ITEM *)(&Queues[QID]);
    	temp_ptr = (Q_ITEM *)(Queues[QID].queue); // First item on Q
    	while (1) {
    		// Is our new item "before" the item we're looking at
    		if (QueueOrder < temp_ptr->QueueOrder  ) { // Yes - enqueue
    			QItem->queue = last_ptr->queue;
    			last_ptr->queue = (void *) QItem;
    			break;
    		}
    		if (temp_ptr->queue == (void *)-1) {   // End of Q or empty
    			temp_ptr->queue = (INT32 *) QItem;
    			break;
    		}
    		last_ptr = temp_ptr;
    		temp_ptr = (Q_ITEM *) temp_ptr->queue;
    	} // End of while
    }  // End of else

    QProclaim("Exiting QInsert:  QID = %d, QOrder = %d\n", QID, QueueOrder);
    return 0;
} // End of QInsert

/**************************************************************************
int  QInsertOnTail(int QID, void *EnqueueingStructure);
     Enqueue an item on the end of the designated Q.
     Input: QID - The ID that describes the target Q.
     Input: EnqueueingStructure - Whatever you want to have enqueued.  It's
                assumed this is the address of a structure.
     Output: The function always returns 0.  But if there's an error
             the simulation ends.
***************************************************************************/
int  QInsertOnTail(int QID, void *EnqueueingStructure) {
    Q_ITEM *QItem;
    Q_ITEM *temp_ptr;
    Q_ITEM *last_ptr;
//    int Index;

    QProclaim("Entering QInsertOnTail:  QID = %d\n", QID);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, 42 );

    QItem = (Q_ITEM *) malloc(sizeof(Q_ITEM));
    if (QItem == 0)
        QPanic("We didn't complete the malloc in QInsert.");

    QItem->queue        = (void *) -1;
    QItem->QueueOrder   = UINT_MAX;
    QItem->QdStructure  = EnqueueingStructure;
    QItem->ItemStructID = Q_STRUCTURE_ID;

    // Is there nothing on the Q?
    if ( Queues[QID].queue == (Q_ITEM *)-1) {
    	Queues[QID].queue = QItem;
    }
    else {
    	// Walk to the end
    	last_ptr = (Q_ITEM *)(&Queues[QID]);
    	temp_ptr = (Q_ITEM *)(Queues[QID].queue); // First item on Q
    	while (1) {
    		if ( (long)temp_ptr == -1 )
    			break;
    		last_ptr = temp_ptr;
    		temp_ptr = (Q_ITEM *) temp_ptr->queue;
    	}
    	last_ptr->queue = QItem;
    }   // End of else
    return 0;
}       // End of QInsertOnTail

/**************************************************************************
  void *QRemoveHead(int QID);
       Dequeue an item from the head of the designated Q.
       Input: QID - The ID that describes the target Q.
       Output: The address of the structure that has been dequeued.
               If there is nothing on the Q, the return value = -1.
***************************************************************************/
void *QRemoveHead(int QID) {
    Q_ITEM *QItem;
    void *ReturnPointer;

    QProclaim("Entering QRemoveHead:  QID = %d\n", QID);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, 42 );

    // Check that the header points to something
    if (Queues[QID].queue == (void *)-1) {
        return ((void *)-1 );               // Q is empty
    }
    QItem = (Q_ITEM *) Queues[QID].queue;   // This is the head item

    Queues[QID].queue = QItem->queue;       // Remove the head item
    QItem->queue = 0;                       // Disable the item we removed

    if (QItem->ItemStructID != Q_STRUCTURE_ID) {
        QPanic("Bad structure ID in QRemoveHead");
    }

    QItem->ItemStructID = 0; // make sure this isn't mistaken
    ReturnPointer = QItem->QdStructure;
    free(QItem);
    QProclaim("Exiting QRemoveHead", QID);
    return (ReturnPointer );
}    // End of QRemoveHead

/**************************************************************************
void *QRemoveItem(int QID, void *EnqueueingStructure);
     Dequeue an item from the designated Q.  May not be the head.
     Input: QID - The ID that describes the target Q.
     Input: EnqueueingStructure - The address you are searching for.
     Output: The address of the first structure found that
             matches the EnqueueingStructure address and
	       has been dequeued.
             If no matching item is found on the Q, the return value = -1.
***************************************************************************/
void *QRemoveItem(int QID, void *EnqueueingStructure) {
    void *ReturnPointer;
    Q_ITEM *temp_ptr, *last_ptr;

    QProclaim("Entering QRemoveItem:  QID = %d\n", QID);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, 42 );

    // Check that the header points to something
    if (Queues[QID].queue == (void *)-1) {
        return ((void *)-1 );               // Q is empty
    }
	last_ptr = (Q_ITEM *)(&Queues[QID]);
	temp_ptr = (Q_ITEM *)(Queues[QID].queue); // First item on Q
	while (1) {
		// Is this the item we're looking at
		if (EnqueueingStructure == temp_ptr->QdStructure  ) { // Yes - dequeue
			last_ptr->queue = temp_ptr->queue;
			ReturnPointer = (void *) temp_ptr->QdStructure;
			break;
		}
		// Have we determined the item is not on Q
		if (temp_ptr->queue == (void *)-1) {   // End of Q or empty
			ReturnPointer = (void *)-1;
		    return (ReturnPointer );
		}
		last_ptr = temp_ptr;
		temp_ptr = (Q_ITEM *) temp_ptr->queue;
	} // End of while

    if (temp_ptr->ItemStructID != Q_STRUCTURE_ID) {
        QPanic("Bad structure ID in QRemoveItem");
    }

    temp_ptr->ItemStructID = 0; // make sure this isn't mistaken
    free(temp_ptr);
    QProclaim("Exiting QRemoveItem", QID);
    return (ReturnPointer );
}    // End of QRemoveItem

/**************************************************************************
void *QNextItemInfo(int QID);
     Get information about the item on the head of the designated Q.
     The item is NOT removed from the Queue.
     Input: QID - The ID that describes the target Q.
     Output: The address of the structure that has been found on the head.
             If there is nothing on the Q, the return value = -1.
***************************************************************************/
void *QNextItemInfo(int QID)  {
	   Q_ITEM *QItem;

	    QProclaim("Entering QNextItemInfo:  QID = %d\n", QID);
	    // Check the inputs are legal - if not legal, we QPanic
	    QCheckValidity( QID, 42 );

	    // Check that the header points to something
	    if (Queues[QID].queue == (void *)-1) {
	        return ((void *)-1 );               // Q is empty
	    }
	    QItem = (Q_ITEM *) Queues[QID].queue;   // This is the head item

	    if (QItem->ItemStructID != Q_STRUCTURE_ID) {
	        QPanic("Bad structure ID in QNextItemInfo");
	    }

	    QProclaim("Exiting QNextItemInfo", QID);
	    return (QItem->QdStructure );
}      // End of QNextItemInfo

/**************************************************************************
void *QItemExists(int QID, void *EnqueueingStructure);
     Determine if the item is present on the Q.
     The item is NOT removed from the Queue.
     Input: QID - The ID that describes the target Q.
     Input: EnqueueingStructure - The address you are searching for.
     Output: The address of the structure that has been found.
             If the item is not found on the Q, the return value = -1.
***************************************************************************/
void *QItemExists(int QID, void *EnqueueingStructure){
    void *ReturnPointer;
    Q_ITEM *temp_ptr;

    QProclaim("Entering QItemExists:  QID = %d\n", QID);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, 42 );

    // Check that the header points to something
    if (Queues[QID].queue == (void *)-1) {
        return ((void *)-1 );               // Q is empty
    }
	temp_ptr = (Q_ITEM *)(Queues[QID].queue); // First item on Q
	while (1) {
		// Is this the item we're looking at
		if (EnqueueingStructure == temp_ptr->QdStructure  ) { // Yes
			ReturnPointer = (void *) temp_ptr->QdStructure;
			break;
		}
		// Have we determined the item is not on Q
		if (temp_ptr->queue == (void *)-1) {   // End of Q or empty
			ReturnPointer = (void *)-1;
		    return (ReturnPointer );
		}
		temp_ptr = (Q_ITEM *) temp_ptr->queue;   // Next item
	} // End of while

    if (temp_ptr->ItemStructID != Q_STRUCTURE_ID) {
        QPanic("Bad structure ID in QItemExists");
    }

    QProclaim("Exiting QItemExists", QID);
    return (ReturnPointer );
}    // End of QItemExists

/**************************************************************************
  char *QGetName( int QID);
       Input: QID - The ID that describes the target Q.
       Output: A string with the QNameDescriptor you gave the Q
               when you created it.
***************************************************************************/
char *QGetName( int QID) {
	int  FillerNumber = 0;
    // Check the QID is legal
    QCheckValidity( QID, FillerNumber );
    return (void *)(Queues[QID].QName);
}    // End of QGetName

/**************************************************************************
void *QWalk(int QID, int QOrder);
     Returns the address of the "QOrderth" item on the Q.
     This is useful if you want to read what's on the Q.  Iterate
        as many times as necessary to get all items - keep going
        until you get a return value of -1.
     Input: QID - The ID that describes the target Q.
     Input: QOrder - the place of the item on the Q.  The first item
             on the Q is the 0th item.
     Output: The address of the structure that has been found.
             If the QOrder given is greater than the number of
             items on the Q, the return value = -1.
***************************************************************************/
void *QWalk(int QID, int QOrder)   {
	Q_ITEM *temp_ptr;
	int  whichItem = 0;
	int  FillerNumber = 0;

    // Check the QID is legal
    QCheckValidity( QID, FillerNumber );

	if ( QOrder < 0 )   {
		QProclaim("Error in QWalk - Order requested = %d\n", QOrder);
		return (void *)-1;
	}
	temp_ptr = (Q_ITEM *)(Queues[QID].queue); // First item on Q
	while( temp_ptr != (Q_ITEM *)-1 )  {
		if (whichItem == QOrder )  {
            //aprintf("ffff%d\n", ((PCB *)temp_ptr->QdStructure)->ProcessID);
            //aprintf("%sffffffffff\n", ((PCB *)temp_ptr->QdStructure)->ProcessName);
			return ((void *)(temp_ptr->QdStructure));
		}
		temp_ptr = temp_ptr->queue;
		whichItem++;
	}
	return (void *)-1;
}

/**************************************************************************
  GetNumberOfAllocatedQueues();
     Returns the number of Queues that have been allocated;
     Input:  Nothing
     Output: How many Queues have been allocated.
***************************************************************************/
int GetNumberOfAllocatedQueues() {
	return( NumberOfAllocatedQueues );
}

//void SetPID
/**************************************************************************
   QPrint()
   THIS IS A DEBUGGING ROUTINE FOR STUDENT USE
   Print out all details of the items on the designated Q
***************************************************************************/
void QPrint(int QID) {
    Q_ITEM *QItem;

    QProclaim("Entering QPrint:  QID = %d\n", QID);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, 42 );
    printf("Printing Q %d with name %s\n", QID, QGetName(QID));
    // Check that the header points to something
    if (Queues[QID].queue == (void *)-1) {
    	printf("Q is empty\n");
        return;               // Q is empty
    }
    QItem = (Q_ITEM *) Queues[QID].queue;   // This is the head entry

    while (QItem != (Q_ITEM *)-1) {
        printf("Struct Addr = %p, QueueOrder = %10u, QdStructure = %lX, StructID = %d.  QNext = %p\n",
        		(void *)QItem, QItem->QueueOrder, (unsigned long)QItem->QdStructure, QItem->ItemStructID, (void *)QItem->queue);
        printf("%d\n", ((PCB *)QItem->QdStructure)->ProcessID);
        printf("%szzzzzzz\n", ((PCB *)QItem->QdStructure)->ProcessName);
        if (QID == 0)  printf("%dzzzzzzz\n", ((PCB *)QItem->QdStructure)->WaitingTime);
        //printf("%d\n", ((PCB *)QItem->QdStructure)->WaitingTime);
        QItem = (Q_ITEM *) QItem->queue;
    }
}          // End of QPrint

/*void QPrint2(int QID) {
    Q_ITEM *QItem;

    QProclaim("Entering QPrint:  QID = %d\n", QID);
    // Check the inputs are legal - if not legal, we QPanic
    QCheckValidity( QID, 42 );
    printf("Printing Q %d with name %s\n", QID, QGetName(QID));
    // Check that the header points to something
    if (Queues[QID].queue == (void *)-1) {
    	printf("Q is empty\n");
        return;               // Q is empty
    }
    QItem = (Q_ITEM *) Queues[QID].queue;   // This is the head entry

    while (QItem != (Q_ITEM *)-1) {
        printf("Struct Addr = %p, QueueOrder = %10u, QdStructure = %lX, StructID = %d.  QNext = %p\n",
        		(void *)QItem, QItem->QueueOrder, (unsigned long)QItem->QdStructure, QItem->ItemStructID, (void *)QItem->queue);
        printf("%d\n", ((Message *)QItem->QdStructure)->sourcePID);
        printf("%szzzzzzz\n", ((Message *)QItem->QdStructure)->buffer);
        printf("%d\n", ((Message *)QItem->QdStructure)->targetPID);
        QItem = (Q_ITEM *) QItem->queue;
    }
}*/
/**************************************************************************
   QProclaim
   THIS IS AN INTERNAL ROUTINE USED TO SUPPORT THESE METHODS
   Simply print out text and the queue name if Q_TRACE is TRUE
   Note this routine can be called with a variable number of
   arguments.
***************************************************************************/
void QProclaim(const char *format, ...)   {
    va_list args;
    va_start(args, format);

    if ( Q_TRACE ) {
        vprintf(format, args);
    }
    va_end(args);
}   // End of QProclaim

/**************************************************************************
    QCheckValidity
    THIS IS AN INTERNAL ROUTINE USED TO SUPPORT THESE METHODS
    Make sure the QID is valid and points to a valid structure
    We terminate the simulation if something is invalid.
***************************************************************************/
void QCheckValidity( int QID, int QueueingOrder ) {

    if ( (QID < 0) || (QID >= NumberOfAllocatedQueues) ) {
        aprintf("%d\n", QID);
        QPanic("In QCheckValidity - Invalid QID\n");
    }

    if ( Queues[QID].HeadStructID != Q_HEAD_STRUCTURE_ID ){
    	printf("QID = %d, Head %d\n", QID, Queues[QID].HeadStructID);
	    QPanic("In QCheckValidity - Invalid QHeader\n");
    }
    if ( QueueingOrder < -1 ){
	    QPanic("In QCheckValidity - Invalid queueing order\n");
    }
}    // End of QCheckValidity

/**************************************************************************
   QPanic
   THIS IS AN INTERNAL ROUTINE USED TO SUPPORT THESE METHODS
   Simply print out text and exit.
   If the code gets here, it means something has gone wrong.
***************************************************************************/
void QPanic(char *Text) {

    printf("%s\n", Text);
    GoToExit(1);
}                     // End of QPanic

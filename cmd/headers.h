#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

typedef short bool;
#define true 1
#define false 0


#define msgQueue 10
#define SHKEY 300
#define SHKEY2 400
#define SHKEY3 500
#define SHKEY4 600

#define MAX_SIZE 1024
struct MemorySegment* memorySegments[MAX_SIZE];
int memorySegmentsCounter = 0;

int * shmaddr;                 



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}


//============================================ struct for a process ==========================================
/*each process has it's id , parents id ,it's state if it is  RUNNING or it is WAITING or it is FINISHED , running time , arrival time
, priority ,turnaround time ,remaining time ,waiting time , finish time, weighted turnaround time, state, pid and memsize */
struct Process
{
    int ID;
    int ParentID;
    int ArrivalTime;
    int FixedTime;
    int RunTime;
    int Priority;
    int TurnAroundTime;
    int RemainingTime;
    int WaitingTime;
    float WeightedTurnAround;
    int FinishTime;
    int State;
    // char state[20];
    int pid;
    int Memsize;
    int Start;
    int End;
};
//===============================================================================================================
//======================================== struct to be sended throught msg queue ===============================
struct msgbuff
{
    long mtype;
    struct Process Process;
};
//===============================================================================================================

//======================================== enum to store the scheduler algorithm to be perform ==================
enum SchedulerALgo
{
    hpf = 1, //uses priority queue and it's priority in enqueue is the priority of the process
    srtn = 2,//uses priority queue and it's priority in enqueue is the remaining time of the process
    rr = 3,  //uses a normal queue
    waitlist = 4,
};
//===============================================================================================================

//======================================= starting to implement our data structure ==============================

//======================================= struct to Node ========================================================
//first we need to have a node that store address of the process and has a pointer to the next node
struct Node
{
    struct Process* P;
    struct Node* next;
};

//===============================================================================================================



//======================================= function to create a new node =========================================
struct Node* createNode(int id , int arrivaltime, int fixedtime, int runtime, int remainingtime , int priority , int parentID, int state, int pid, int memsize, int start, int end)
{
   // printf("i entered the function createNode..\n");
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));//allocate memory dynamically
    //printf("i created new node using malloc\n");
    struct Process* newProcess = (struct Process*)malloc(sizeof(struct Process));
   // printf("i created new node using malloc\n");
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newProcess->ID=id;
    newProcess->ArrivalTime = arrivaltime;
    newProcess->FixedTime = fixedtime;
    newProcess->RunTime = runtime;
    newProcess->RemainingTime = remainingtime;
    newProcess->Priority = priority;
    newProcess->ParentID = parentID;
    newProcess->State = state;
    newProcess->pid = pid;
    // strcpy(newProcess->state,"WAITING");
    newProcess->WaitingTime = 0;
    newProcess->Memsize = memsize;
    newProcess->Start = start;
    newProcess->End = end;
    newNode->P=newProcess;
    newNode->next = NULL;

    return newNode;
}
//don't forget to use free(ptr_obj) , and the ptr_obj is the pointer to object that created dynamically to free the memory
//================================================================================================================

//======================= struct for the queue (modified work as Priority and normal queue) ======================
struct Queue
{
    struct Node *head;
    struct Node *tail;
    int count;
};
//================================================================================================================

//======================================= function to create a new queue =========================================
//this function creates a queue and set return it
struct Queue* createQueue()
{
    struct Queue* newQueue = (struct Queue*)malloc(sizeof(struct Queue));
    if (newQueue == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newQueue->head=NULL;
    newQueue->tail=NULL;
    newQueue->count=0;
    return newQueue;
}
//don't forget to use free(ptr_obj) , and the ptr_obj is the pointer to object that created dynamically to free the memory
//================================================================================================================

//=============================== function to check if the queue is empty =========================================
bool isEmpty(struct Queue** Q)
{
    if((*Q)->head == NULL)
        return true;
    else
        return false;
}
//================================================================================================================

//========================= function to Peek the first element in the queue ======================================
struct Process* PeekFront(struct Queue** Q)
{
    if(isEmpty(Q))
    {
        return NULL;
    }
    else
    {
        struct Process * P = (*Q)->head->P;//get from the queue the first node and from this node get the process
        return P;
    }
}
//================================================================================================================

//============== function dequeue delete the first node in the queue and return the process in that node ======================
struct Process * Dequeue(struct Queue** Q,enum SchedulerALgo i)
{
   
   // printf("i entered the Dequeue function..\n");
    //printf("%p\n",((*Q)->head)->next);
    //printf("%p\n",((*Q)->tail)->next);
    if(isEmpty(Q))
    {
        printf("Queue is empty...\n");
        return NULL;

    }
    //printf("i got the process at the head which is..\n");
    struct Process* P;
    P=((*Q)->head)->P;//i get the process in the first node in the queue and P is the return address
    //printProcess(P);


    //check if it is the only element in the queue
    struct Node* NodetobeDeleted=((*Q)->head);
    if((*Q)->count==1)//there is only 1 element in the queue then remove it and let the head equal null
    {
        //free(NodetobeDeleted);//i freed the node that the head points to

        ((*Q)->tail)=NULL;

        //free(NodetobeDeleted);//i freed the node that the head points to

        ((*Q)->head)=NULL;

        ((*Q)->count)=((*Q)->count)-1;
        

        return P;
    }
    else
    {
        //printf("there is more then one element in the queue..\n");
        ((*Q)->head)=((*Q)->head)->next;//update the head to be the next node
        if(i==3)
        {
            ((*Q)->tail)->next = ((*Q)->head);//update the tail to the new head
        }
        NodetobeDeleted->next=NULL;
        free(NodetobeDeleted);
        //printf("i freed the head and apply to the head the next node..\n");
        
        (*Q)->count--;
        return P;
    }

}
//================================================================================================================

//============== function enqueue to enqueue the node in the suitable place in the queue according to it's scheduler algorithm ======================
void Enqueue(struct Queue** Q,struct Process ProcessObj,enum SchedulerALgo i)
{
    //printf("i entered the function enqueue..\n");
    struct Node* CreatedNode = createNode(ProcessObj.ID,ProcessObj.ArrivalTime,ProcessObj.FixedTime,
    ProcessObj.RunTime,ProcessObj.RemainingTime,ProcessObj.Priority,ProcessObj.ParentID,
    ProcessObj.State,ProcessObj.pid,ProcessObj.Memsize, ProcessObj.Start, ProcessObj.End);

    //handle the case if the queue is empty
    //printf("i entered the function enqueue..\n");

    if(isEmpty(Q))//adding the node in a empty queue
    {
      //  printf("i entered the conditin if the queue empty..\n");
        if(i==3)
        {
            
            ((*Q)->head) = CreatedNode;
            ((*Q)->tail) = CreatedNode;
            CreatedNode->next=CreatedNode;//let the node point to itself
            (*Q)->count++;
        }
        else
        {
            //printf("i entered the to add an object to an empty queue using HPF or SRTN..\n");
            
            ((*Q)->head) = CreatedNode;
            ((*Q)->tail) = CreatedNode;
            (*Q)->count++;
        }
        return;
    }

    struct Node* Temp=((*Q)->head);
    switch(i)
    {
        case hpf:
        //printf("i entered to enqueue a process to HPF..\n");
            if (((*Q)->head)->P->Priority > ProcessObj.Priority)
            {//if the priority of the process entering the queue is higher than the priority of the first process in the queue then enqueue the new process at the head
                CreatedNode->next = ((*Q)->head);
                ((*Q)->head) = CreatedNode;
            }
            else
            {
                //move untill you found a node that has a process with the lower priority than the entering process and then added this node before it
                while (Temp->next != NULL && Temp->next->P->Priority <= ProcessObj.Priority)
                {
                    Temp = Temp->next;
                }
                CreatedNode->next = Temp->next;
                Temp->next = CreatedNode;
            }
            (*Q)->count=(*Q)->count+1;
            break;
        case srtn:
        //compare according to the remaining time of the entering process
        //printf("i entered to enqueue a process to SRTN..\n");
            if (((*Q)->head)->P->RemainingTime > ProcessObj.RemainingTime)
            {
                CreatedNode->next = ((*Q)->head);
                ((*Q)->head) = CreatedNode;
                //if the entering process has remaining time shorter than the process at the head so let the entering process be the head
            }
            else
            {
                while (Temp->next != NULL && Temp->next->P->RemainingTime <= ProcessObj.RemainingTime)
                {
                    Temp = Temp->next;
                    //move in the queue until finding a process that has a larger remaing time
                }
                CreatedNode->next = Temp->next;
                Temp->next = CreatedNode;
            }
            (*Q)->count=(*Q)->count+1;
            break;
        case rr://as a circular queue
        //printf("i entered to enqueue a process to RR..\n");
            CreatedNode->next=(*Q)->head;
            (*Q)->tail->next = CreatedNode;
            (*Q)->tail = CreatedNode;

            (*Q)->count = (*Q)->count + 1;
            break;
        case waitlist:
            // compare according to the Memsize of the entering process
            if (((*Q)->head)->P->Memsize > ProcessObj.Memsize)
            {
                CreatedNode->next = ((*Q)->head);
                ((*Q)->head) = CreatedNode;
                // if the entering process has remaining time shorter than the process at the head so let the entering process be the head
            }
            else
            {
                while (Temp->next != NULL && Temp->next->P->Memsize <= ProcessObj.Memsize)
                {
                    Temp = Temp->next;
                    // move in the queue until finding a process that has a larger Memsize time
                }
                CreatedNode->next = Temp->next;
                Temp->next = CreatedNode;
            }
            (*Q)->count = (*Q)->count + 1;
            break;
    }

}

//================================================================================================================


void printProcess(struct Process* p)
{
   printf("id is: %d\n", p->ID);
  // printf("pID is: %d\n", p->ParentID);
   printf("arrival times is: %d\n", p->ArrivalTime);
 //  printf("runTime is: %d\n", p->RunTime);
   printf("prioriry is: %d\n", p->Priority);
 //  printf("state is: %s\n", p->state);
   printf("remaining time is: %d\n", p->RemainingTime);
/*  printf("finish time is: %d\n", p->FinishTime);
   printf("Waiting time is: %d\n", p->WaitingTime);
   printf("TA is: %d\n", p->TurnAroundTime);
   printf("WTA is: %f\n", p->WeightedTurnAround);*/
}

void printList(struct Process Proccesses[], int inputProccessesCount)
{
    for(int i=0; i<inputProccessesCount; i++)
    {
        printf("\nProccess '%d, %d, %d, %d'\n", Proccesses[i].ID, Proccesses[i].ArrivalTime, Proccesses[i].RunTime, Proccesses[i].Priority);
    }
}

void printQueue(struct Queue **q)
{
   if (isEmpty(q))
    {
        printf("\nEmpty Queue\n");
        return;
    }
   printf("\nStart of the Q, Count: %d\n", (*q)->count);
   struct Node* start = ((*q)->head);
   printProcess(start->P);
   int counter=((*q)->count) ;
   //counter =counter-1;
   while (start->next != NULL)
   {
    counter--;
        if(counter == 0){
            break;
        }
      printProcess(start->next->P);
      start = start->next;
      
     // printf("%d\n",counter);
      //sleep(2);

   }
   printf("\nEND of the Q\n");
}

//======================================== MEMORY RELATED STRUCTURES AHEAD ======================================

bool AllocateProcess(struct Process *process);
struct MemoryNode* searchMemoryNodeByID(struct MemoryNode* root, int id);
struct MemoryNode* createMemoryNode(int segmentSize, int segmentLocation);
struct MemoryNode* createMemoryBSTHelper(int start, int end, struct MemoryNode* myParent);
struct MemoryNode* findNodeInShortestConsecutiveRange(int requiredSize);
struct MemoryNode* findParentNode(struct MemoryNode* node, int requiredSize);
struct MemoryNode* findAvailableSegment(struct MemoryNode* root, int requestedSize);
int occupySegment(int processID, struct MemoryNode* segment, int processSize);
void deallocateSegment(struct MemoryNode* segment);
void freeMemoryBST(struct MemoryNode* root);
void occupyChildren(struct MemoryNode* root);
void unoccupyChildren(struct MemoryNode* root);

//======================================== Memory Tree Node =====================================================

struct MemoryNode {
    int segmentSize;      // Size of the memory segment (power of 2)
    bool isOccupied;      // Flag indicating whether the segment is occupied
    int processID;       // ID of the process stored in the segment (if occupied)
    int storedSize;       // Size of the process stored in the segment (if occupied)
    int segmentLocation;  // Starting location of the memory segment
    int segmentLocationEnd;  // Ending location of the memory segment
    struct MemoryNode* parent;   // Pointer to the parent
    struct MemoryNode* left;   // Pointer to the left child node
    struct MemoryNode* right;  // Pointer to the right child node
};

bool AllocateProcess(struct Process *process)
{
    int memSize = process->Memsize;
    int ID = process->ID;
    struct MemoryNode *shortestLeaf = findNodeInShortestConsecutiveRange(memSize);

    if (shortestLeaf == NULL)
        return false;

    struct MemoryNode *smallestParent = findParentNode(shortestLeaf, memSize);

    if (smallestParent != NULL)
    {
        int endLocation = smallestParent->segmentLocationEnd;
        int startLocation = occupySegment(ID, smallestParent, memSize);
        process->Start = startLocation;
        process->End = endLocation;
        return true;
    }
    else
    {
        return false;
    }
}

void DeallocateProcess(int processID, struct MemoryNode* rootMemory) {
    struct MemoryNode* processSegment = searchMemoryNodeByID(rootMemory, processID);
    if (processSegment != NULL)
    deallocateSegment(processSegment);
}

struct MemoryNode* searchMemoryNodeByID(struct MemoryNode* root, int id) {
    // Base case: if the current node is NULL, return NULL
    if (root == NULL) {
        return NULL;
    }

    // Check if the current node's process ID matches the target ID
    if (root->processID == id) {
        return root; // If found, return the current node
    }

    // Recursively search in the left and right subtrees
    struct MemoryNode* leftResult = searchMemoryNodeByID(root->left, id);
    struct MemoryNode* rightResult = searchMemoryNodeByID(root->right, id);

    // Check if either left or right result is not NULL, return it
    if (leftResult != NULL) {
        return leftResult;
    }
    if (rightResult != NULL) {
        return rightResult;
    }

    // If both left and right results are NULL, return NULL
    return NULL;
}



struct MemoryNode* createMemoryNode(int segmentSize, int segmentLocation) {
    struct MemoryNode* newNode = (struct MemoryNode*)malloc(sizeof(struct MemoryNode));
    if (newNode != NULL) {
        newNode->segmentSize = segmentSize;
        newNode->isOccupied = false;
        newNode->storedSize = 0;
        newNode->processID = -1;
        newNode->segmentLocation = segmentLocation;
        newNode->segmentLocationEnd = (segmentLocation + segmentSize - 1)  ;
        newNode->left = NULL;
        newNode->right = NULL;
    }
    return newNode;
}

struct MemoryNode* createMemoryBSTHelper(int start, int end, struct MemoryNode* myParent) {
    // Base case: if start is greater than end, return NULL
    if (start > end) {
        return NULL;
    }

    // Calculate segment size
    int segmentSize = end - start + 1;
    int mid = (start + end) / 2;

    // Create a new memory node
    struct MemoryNode* newNode = createMemoryNode(segmentSize, start);
    newNode->parent = myParent;

    if (newNode != NULL) {
        // Prevent infinite recursions
        // The last element to execute this scope is the last 2 element
        // It will create two 1 childs first
        if (start != end) {
            newNode->left = createMemoryBSTHelper(start, mid, newNode);
            newNode->right = createMemoryBSTHelper(mid + 1, end, newNode);
            if (end - start == 1) // Only done by 2 nodes
            {
                // These are the leaves of the tree, they are used to
                // keep track of the tree in a more high-level way
                memorySegments[memorySegmentsCounter] = newNode->left;
                memorySegments[memorySegmentsCounter + 1] = newNode->right;
                memorySegmentsCounter++;
                memorySegmentsCounter++;
            }
        }
    }

    return newNode;
}

/* Fetches the high level interpretation of the tree to pick out a node from the range in which
   is empty AND has enough space for the process
   Any nodes in that range eventually lead back to the same parent that will be chosen */
struct MemoryNode* findNodeInShortestConsecutiveRange(int requiredSize) {
    
    struct MemoryNode* resultNode = NULL;
    int currentCount = 0;
    int shortestCount = MAX_SIZE;

    for (int i = 0; i < MAX_SIZE; ++i) {
        struct MemoryNode* currentNode = memorySegments[i];

        if (resultNode == NULL) {
                resultNode = currentNode;  // Update resultNode to the first unoccupied node
            }

        if (currentNode->isOccupied == false) {
            currentCount++;
        } 
        else
        {
            if (currentCount >= requiredSize && currentCount < shortestCount) {
                shortestCount = currentCount;
                resultNode = memorySegments[i - 1];  // If the streak was broken, pick the element before this happened
            }
            currentCount = 0;
        }
    }
    // Check if the last sequence is the shortest and if no interruptions happened above
    if (currentCount >= requiredSize && currentCount < shortestCount) {
        shortestCount = currentCount;
        resultNode = memorySegments[MAX_SIZE - currentCount];  // Update resultNode to the start of the shortest consecutive range
    }

    // Was that shortest range even enough for the space needed?
    return (shortestCount < requiredSize) ? NULL : resultNode;
}

// Starting from the leaf, go up until the 1st parent with enough size is reached
struct MemoryNode* findParentNode(struct MemoryNode* node, int requiredSize) {

    // Base case: if the current node is NULL
    if (node == NULL) {
        return NULL;
    }

    if (node->isOccupied || requiredSize > MAX_SIZE) {
        return NULL;
    }

    // Base case: if the current node is NULL or its segment size satisfies the requirement, return it
    if (node->segmentSize >= requiredSize) {
        return node;
    }

    // Recursive case: climb up to the parent node
    return findParentNode(node->parent, requiredSize);
}


// The "classic" way of navigating the tree
struct MemoryNode* findAvailableSegment(struct MemoryNode* root, int requestedSize) {
    // Base case: if the current node is NULL, return NULL
    if (root == NULL) {
        return NULL;
    }

    // Check if the segment is occupied; if so, return NULL
    if (root->isOccupied || requestedSize > MAX_SIZE) {
        return NULL;
    }

    // If both children are occupied, reject this one
    // for example, if two 256 are occupied, why chose the 512?
    if (root->right->isOccupied && root->left->isOccupied) {
        return NULL;
    }

    // Check if the requested size is less than or equal to half of the segment size
    if (requestedSize <= (root->segmentSize / 2)) {
        // If the requested size is less than or equal to half of the segment size, recursively search in the left subtree
        struct MemoryNode* leftSegment = findAvailableSegment(root->left, requestedSize);
        if (leftSegment != NULL) {
            return leftSegment;  // If found in the left subtree, return it
        }
        // If not found in the left subtree, continue searching in the right subtree
        return findAvailableSegment(root->right, requestedSize);
    }

    // If the requested size is greater than half of the segment size, return the current node
    return root;
}


// Mark a memory segment and all of its children as occupied
// Also returns the starting address of the segment
int occupySegment(int processID, struct MemoryNode* segment, int processSize) {
    if (segment != NULL) {
        segment->isOccupied = true;
        segment->processID = processID;
        segment->storedSize = processSize;
        occupyChildren(segment);
        return segment->segmentLocation;  // Return the starting position of the occupied segment
    }
    return -1;  // Return -1 if the segment is NULL
}

// Deallocate a memory segment and unoccupy it and its children
void deallocateSegment(struct MemoryNode* segment) {
    if (segment != NULL) {
        segment->isOccupied = false;
        segment->storedSize = 0;
        segment->processID = -1;
        unoccupyChildren(segment);
    }
}

// Free BST memory
void freeMemoryBST(struct MemoryNode* root) {
    if (root != NULL) {
        freeMemoryBST(root->left);
        freeMemoryBST(root->right);
        free(root);
    }
}

// Recursive marking of children as occupied (critical for the high level representation)
void occupyChildren(struct MemoryNode* root) {
    if (root != NULL) {
        root->isOccupied = true;
        occupyChildren(root->left);
        occupyChildren(root->right);
    }
}

// Recursively unoccupy memory segments of a given node and its children
void unoccupyChildren(struct MemoryNode* root) {
    if (root != NULL) {
        root->isOccupied = false;
        unoccupyChildren(root->left);
        unoccupyChildren(root->right);
    }
}

//===============================================================================================================
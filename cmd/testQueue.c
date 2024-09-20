#include "headers.h"

int main()
{
    enum SchedulerALgo insertionFactor = 2;
/*  1	2	14	10
    2	10	17	4
    3	16	14	9
    4	16	18	10
    5	24	2	6  */
    struct Process* p1;
    p1=(struct Process*)malloc(sizeof(struct Process));
    p1->ID = 1;
    p1->ArrivalTime = 2;
    p1->RunTime = 14;
    p1->Priority = 10;
    p1->RemainingTime = p1->RunTime;

    struct Process* p2;
    p2=(struct Process*)malloc(sizeof(struct Process));
    p2->ID = 2;
    p2->ArrivalTime = 10;
    p2->RunTime = 17;
    p2->Priority = 4;
    p2->RemainingTime = p2->RunTime;

    struct Process*  p3;
    p3=(struct Process*)malloc(sizeof(struct Process));
    p3->ID = 3;
    p3->ArrivalTime = 16;
    p3->RunTime = 14;
    p3->Priority = 9;
    p3->RemainingTime = p3->RunTime;

    struct Process*  p4;
    p4=(struct Process*)malloc(sizeof(struct Process));
    p4->ID = 4;
    p4->ArrivalTime = 16;
    p4->RunTime = 18;
    p4->Priority = 10;
    p4->RemainingTime = p4->RunTime;

    struct Process*  p5;
    p5=(struct Process*)malloc(sizeof(struct Process));
    p5->ID = 5;
    p5->ArrivalTime = 24;
    p5->RunTime = 2;
    p5->Priority = 6;
    p5->RemainingTime = p5->RunTime;

    struct Queue* CPQptr = createQueue();
    Enqueue(&CPQptr, *p2, insertionFactor);
    Enqueue(&CPQptr, *p3, insertionFactor);
    Enqueue(&CPQptr, *p1, insertionFactor);
    Enqueue(&CPQptr, *p5, insertionFactor);
    Enqueue(&CPQptr, *p4, insertionFactor);
    

    
    printQueue(&CPQptr);

    printProcess(Dequeue(&CPQptr,insertionFactor));
    printQueue(&CPQptr);

    printProcess(Dequeue(&CPQptr,insertionFactor));
    printQueue(&CPQptr);

    printProcess(Dequeue(&CPQptr,insertionFactor));
    printQueue(&CPQptr);

    printProcess(Dequeue(&CPQptr,insertionFactor));
    printQueue(&CPQptr);

    printProcess(Dequeue(&CPQptr,insertionFactor));
    printQueue(&CPQptr);

    
    return 0;
} 

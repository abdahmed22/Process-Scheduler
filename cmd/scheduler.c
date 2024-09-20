#include "headers.h"

FILE *outputFile1;
FILE *outputFile2;
FILE *outputFile3;

void handler(int);

void HPF( struct Queue** Ready_Processes, struct Queue** Waiting_Processes, struct MemoryNode* Memory_Root );
void SRTN( struct Queue** Ready_Processes, struct Queue** Waiting_Processes, struct MemoryNode* Memory_Root );
void RR( struct Queue** Ready_Processes, struct Queue** Waiting_Processes, struct MemoryNode* Memory_Root, int quantum );

void start(struct Process* process);
void finish(struct Process process, struct MemoryNode* Memory_Root);
void resume(struct Process process);
void Pause(struct Process process);

int processes_count=0;


bool stillSending = true;

double totalWTA = 0;
double totalWait = 0;
double totalRun = 0;
int totalSquareWTA = 0;
double* perProcessWTA;
int currentProcessIndex = 0;
int finished_process_count=0,shmid,msgid,RecievingQueue;
int Algo;
struct  msgbuff msgProcess;

int* allProcessCount;
char* ProcessPath;

int main(int argc, char * argv[])
{
    initClk();
    printf("Entered the scheduler::\n");
//========================================================================================================================
//Get the process path
    char processBuffer[500];
    getcwd(processBuffer, sizeof(processBuffer));
    ProcessPath = strcat(processBuffer, "/process.out");
//========================================================================================================================

//===================================== Readying command-line arguments  =================================================
    int type = atoi(argv[1]);                                               //1 for HPF , 2 for SRTN , 3 for RR
    Algo=type;
    printf("Algorithm type: %d\n",Algo);
    int quantum = atoi(argv[2]);                                            //RR quantum    -1 if HPF or SRTN
    processes_count = atoi(argv[4]);                                        //number of processes 
    printf("Number of processes: %d\n",processes_count);
//========================================================================================================================

//===================================== Initializing output files  =======================================================
    outputFile1 = fopen("scheduler.log", "w");
    outputFile2 = fopen("scheduler.perf", "w");
    outputFile3 = fopen("memory.log", "w");
    fprintf(outputFile1, "#At time x process y state arr w total z remain y wait k\n");
    fprintf(outputFile3, "#At time x allocated y bytes for process z from i to j\n");
//========================================================================================================================

//==================================================IPCs==================================================================

        key_t key=ftok("keyfile",msgQueue);
        RecievingQueue=msgget(key,0666| IPC_CREAT);//created the queue that will recieve the process through

        int id_rows= shmget(SHKEY4, 4, IPC_CREAT | 0644);
        allProcessCount = (int *) shmat(id_rows, (void *)0, 0);//get the number processes that the scheduler will deal with using shared memory

        struct Queue* Ready_Processes = createQueue();//queue to add the processes that thier arrival time has came 
        struct Queue* Waiting_Processes = createQueue();//queue to add the processes that are waiting for free memory
        struct MemoryNode* Memory_Root = createMemoryBSTHelper(0,1023, NULL); //1023=1024-1
        int temp = *allProcessCount;
        perProcessWTA = malloc(sizeof(int) * temp);
        signal(SIGUSR1, handler);
//========================================================================================================================

//=========================================== Choose algorithm type =======================================================
    if (type == 1) {
        printf("HPF Algorithm starts\n");
        HPF(&Ready_Processes, &Waiting_Processes, Memory_Root);
        printf("HPF Algorithm ends\n");
    } else if (type == 2) {
        while(getClk()<1){
            //sleep until clck equals 1          
        }
        printf("SRTN Algorithm starts\n");
        SRTN(&Ready_Processes, &Waiting_Processes, Memory_Root);
        printf("SRTN Algorithm ends\n");
    } else if (type == 3) {
        // while(getClk()<1){
        //     //sleep until clck equals 1
        // }
        printf("RR Algorithm starts\n");
        RR(&Ready_Processes, &Waiting_Processes, Memory_Root, quantum);
        printf("RR Algorithm ends\n");
    } else {
        printf("Invalid Algorithm type \n");
    }
//========================================================================================================================

//===================================== Outputfile.pref Calculations =====================================================
    float utilization = ((float)totalRun / getClk()) * 100.0;
    float avgWTA = totalWTA / finished_process_count;
    float avgWait = totalWait / finished_process_count;
    
    float totalStdWTA = 0;
    for (int i = 0; i < finished_process_count; i++)
    {
        totalStdWTA += (perProcessWTA[i] - avgWTA) * (perProcessWTA[i] - avgWTA);
    }
    
    float stdWTA = sqrt((float)(totalStdWTA / finished_process_count));

    fprintf(outputFile2, "CPU utilization = %.2f %% \n", utilization);
    fprintf(outputFile2, "Avg WTA = %.2f\n", avgWTA);
    fprintf(outputFile2, "Avg Waiting = %.2f\n", avgWait);
    fprintf(outputFile2, "Std WTA = %.2f\n", stdWTA);
//========================================================================================================================

//========================================== Closing output files  =======================================================
    fclose(outputFile1);
    fclose(outputFile2);
    fclose(outputFile3);
    kill(atoi(argv[3]),SIGINT);
    

    printf("scheduler main is about to end\n");
    destroyClk(true); //upon termination release the clock resources.
}
//========================================================================================================================

//========================================= Print Memory Function ========================================================
void printMeomry(struct Process* process) {
    int startTime = getClk();
    fprintf(outputFile3, "At time %d allocated %d bytes for process %d from %d to %d\n",
            startTime, process->Memsize, process->ID, process->Start, process->End);
    printf("At time %d allocated %d bytes for process %d from %d to %d\n",
            startTime, process->Memsize, process->ID, process->Start, process->End);
}
//========================================================================================================================

//============================================= Start Function ===========================================================
void start(struct Process* process) {
    int startTime = getClk();
    if(process->ID > processes_count || process->ID < 0) {
        return;
    }
    fprintf(outputFile1, "At time %d process %d started, arrival time %d total %d remain %d wait %d\n",
            startTime, process->ID, process->ArrivalTime, process->FixedTime,
            process->RemainingTime, startTime - process->ArrivalTime);
    printf("At time %d process with ID %d started, arrived at %d total %d remain %d wait %d\n",
            startTime, process->ID, process->ArrivalTime, process->FixedTime,
            process->RemainingTime, startTime - process->ArrivalTime);

    int Pid = fork();
    process->pid= Pid;
    if (process->pid == 0)
    { 
        //we need to send the remaining time to the generated process
        char Remaining_Time[20];
        sprintf(Remaining_Time,"%d",process->RemainingTime);        //convert the remaining time into string to be sended to the created process
                
        execl(ProcessPath,"process.out",Remaining_Time,NULL);  
    }
    else {
        return; 
    }
}
//========================================================================================================================

//============================================= Finish Function ==========================================================
void finish(struct Process process, struct MemoryNode* Memory_Root) {
    int finishTime = getClk();
    int TA = finishTime - process.ArrivalTime;
    double WTA =(double) TA / process.FixedTime;
    int wait = TA - process.FixedTime;
    if (process.End > 1024 || process.End < 0){
        process.End = 123;
        process.Start = 64;
    }

    fprintf(outputFile1, "At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process.ID, process.ArrivalTime, process.FixedTime, wait, TA, WTA);
    fprintf(outputFile3, "At time %d freed %d bytes from process %d from %d to %d\n",
            finishTime, process.Memsize, process.ID, process.Start, process.End);
    printf("At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process.ID, process.ArrivalTime, process.FixedTime, wait, TA, WTA);
    printf("At time %d freed %d bytes from process %d from %d to %d\n",
            finishTime, process.Memsize, process.ID, process.Start, process.End);


    DeallocateProcess(process.ID, Memory_Root),

    perProcessWTA[currentProcessIndex] = WTA;
    currentProcessIndex++;
    totalSquareWTA += WTA * WTA;
    totalWTA += WTA;
    totalWait += wait;
    totalRun += process.FixedTime;
    finished_process_count++;
}

//========================================================================================================================

//============================================= Resume Function ==========================================================
void resume(struct Process process)
{
    int currentTime = getClk();

    kill(process.pid, SIGCONT);

    fprintf(outputFile1, "At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
            currentTime, process.ID, process.ArrivalTime, process.FixedTime, process.RemainingTime,
            currentTime - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
    printf("At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
            currentTime, process.ID, process.ArrivalTime, process.FixedTime, process.RemainingTime,
            currentTime - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
}
//========================================================================================================================

//============================================= Pause Function ===========================================================
void Pause(struct Process process)
{
    kill(process.pid , SIGSTOP);
    fprintf(outputFile1, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
            getClk(), process.ID, process.ArrivalTime, process.FixedTime,process.RemainingTime,
            getClk() - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
    printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n",
            getClk(), process.ID, process.ArrivalTime, process.FixedTime,process.RemainingTime,
            getClk() - process.ArrivalTime - (process.FixedTime - process.RemainingTime));
}
//========================================================================================================================

//============================================= HPF algorithm ============================================================
void HPF( struct Queue** Ready_Processes, struct Queue** Waiting_Processes, struct MemoryNode* Memory_Root  ) {
    struct Process *process = NULL;
    int ID;
    int checkScheduler=-1;//if this variable is equal to -1 this means that the CPU is not processing any process so he can start a process at any instance
    int workingtime=0;
    int processes_counter=0;
    int starttime=-1;
    int currenttime = getClk();
    bool test;
    msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0,!IPC_NOWAIT);
            while(getClk()<1){
            //sleep until clck equals 1          
        }
    printf("message Recieved id =%d\n",msgProcess.Process.ID);

    struct MemoryNode* temp = findAvailableSegment(Memory_Root, msgProcess.Process.Memsize);

    if (temp == NULL) {
        printf("\nno space\n");
        Enqueue(Waiting_Processes,msgProcess.Process,waitlist);
    } else {
        printf("\nspace\n");
        test = AllocateProcess(&msgProcess.Process);
        printMeomry(&msgProcess.Process);
        Enqueue(Ready_Processes,msgProcess.Process,hpf);
    }

    while(stillSending || (processes_counter-processes_count)!=0 ||checkScheduler==-1)
    {
        while( msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0,IPC_NOWAIT) != -1)
        {   
            printf("message Recieved id =%d\n",msgProcess.Process.ID);
            if(msgProcess.Process.ID<=0 || msgProcess.Process.ID > processes_count+1)
                break;
            if(msgProcess.mtype==1) {    //the arrived message is garbage so check if there any process in the ready queue
                temp = findAvailableSegment(Memory_Root, msgProcess.Process.Memsize);
                if (temp == NULL) {
                    printf("\nno space\n");
                    Enqueue(Waiting_Processes,msgProcess.Process,waitlist);
                } else {
                    printf("\nspace\n");
                    test = AllocateProcess(&msgProcess.Process);
                    printMeomry(&msgProcess.Process);
                    Enqueue(Ready_Processes,msgProcess.Process,hpf);
                }
            }
            else
                break;
        }

        if(!isEmpty(Ready_Processes) )
        {
            if(checkScheduler==-1 && PeekFront(Ready_Processes)->ArrivalTime <= getClk())
            {
                printProcess(PeekFront(Ready_Processes));
                process=Dequeue(Ready_Processes,hpf); //get the process with the highest priority in the ready queue
                printProcess(process);
                ID = process->ID;
                if(process->ID==-1 || process == NULL)
                    return;
                process->FixedTime=process->RunTime;
                start(process);
                starttime=getClk();//send the process to start which fork a process child
                checkScheduler = 1;
            }
        }
        if(starttime + (process->RemainingTime) == getClk() && checkScheduler == 1)//this is the time the process will finish in
        {   
            processes_counter++;
            finish(*process,  Memory_Root);
            checkScheduler = -1; //the cpu is now free to accept ready process
            if(processes_count==processes_counter)
                break;
        }
    }
}
//========================================================================================================================

//============================================= SRTN algorithm ============================================================
void SRTN( struct Queue** Ready_Processes, struct Queue** Waiting_Processes, struct MemoryNode* Memory_Root ) {
    struct Process *process,*temp=NULL,*temp2=NULL;
    int ID;
    bool test;
    while(true) {
        if(!stillSending && isEmpty(Ready_Processes)) 
            return;
        if(!isEmpty(Ready_Processes) )
        {
            process=Dequeue(Ready_Processes,srtn);
            
            if(temp!=NULL)
            {
                if(temp != NULL && temp->ID != process->ID && temp->RemainingTime!=0 )
                    Pause(*temp);
            }

            if (process->State==0) {
                process->State=1;
                process->FixedTime=process->RunTime;
                start(process);
                sleep(1);
                process->RemainingTime=process->RemainingTime -1;
                temp=process;
            }
            else if(temp->ID != process->ID  ) {
                resume(*process);
                sleep(1);
                process->RemainingTime=process->RemainingTime -1;
                temp=process;
            }
            else {
                sleep(1);
                process->RemainingTime=process->RemainingTime -1;
            }
            if(process->RemainingTime==0)
            {
                finish(*process, Memory_Root);
                
                  temp2=PeekFront(Waiting_Processes);
                  if(temp2)
                   {
                    test = AllocateProcess(temp2);
                    if(test==true)
                    {
                    printMeomry(temp2);
                    temp2=Dequeue(Waiting_Processes,waitlist);
                    Enqueue(Ready_Processes,*temp2,srtn);
                    }
                   }

                temp=process;
            }
            else{
                Enqueue(Ready_Processes,*process,srtn);
            }
        }
        while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
                break;
            ID = msgProcess.Process.ID;
            printf("message Recieved id =%d\n",msgProcess.Process.ID);

            test = AllocateProcess(&msgProcess.Process);
            if(test==true){
                printMeomry(&msgProcess.Process);
                Enqueue(Ready_Processes,msgProcess.Process,srtn);
            }
            else{
                Enqueue(Waiting_Processes,msgProcess.Process,waitlist);
            }
             
        }
    }
}
//========================================================================================================================

//============================================= RR algorithm ============================================================
void RR(  struct Queue** Ready_Processes, struct Queue** Waiting_Processes, struct MemoryNode* Memory_Root, int quantum)
{
    bool test;
    struct Process *process;
    process->State =0;
    while(true) {
        if(!stillSending && isEmpty(Ready_Processes) ) 
            return;
        if(!isEmpty(Ready_Processes) )
        {
            process=Dequeue(Ready_Processes,rr);
            if( process->State==0 ) {
                if(process->RemainingTime >quantum) {
                    process->FixedTime=process->RunTime;
                    start(process);
                    process->State=1;
                    int past = getClk()+quantum;
                    int temp;
                    while(getClk() < past){
                        if(temp == 1) {       
                            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1){
                                break;
                            }
                            printf("message Recieved id =%d\n",msgProcess.Process.ID);
                            if (msgProcess.Process.ID != -1) {
                                Enqueue(Ready_Processes,msgProcess.Process,rr);
                                temp = 0;
                            }   
                        }
                        //get one process while sleeping and sleep until quantum is done
                    }
                    process->RemainingTime=process->RemainingTime-quantum;
                    Pause(*process);
                    Enqueue(Ready_Processes,*process,rr);

                }
                else if (process->RemainingTime == 0){
                    finish(*process, Memory_Root);
                } else {
                    process->FixedTime=process->RunTime;
                    start(process);
                    int past = getClk()+process->RemainingTime;
                    while(getClk() < past){
                        //sleep until quantum is done
                    }
                    finish(*process, Memory_Root);
                }
            }
            else {
                if(process->RemainingTime >quantum) {
                    resume(*process);
                    int past = getClk()+quantum;
                    while(getClk() < past){
                        //sleep until quantum is done
                    }
                    process->RemainingTime=process->RemainingTime-quantum;
                    Pause(*process);
                    Enqueue(Ready_Processes,*process,rr);
                }
                else if (process->RemainingTime == 0){
                    finish(*process, Memory_Root);
                } else {
                    resume(*process);
                    int past = getClk()+process->RemainingTime;
                    while(getClk() < past){
                        //sleep until quantum is done
                    }
                    finish(*process, Memory_Root);
                }
            }
        }
        while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1){
                break;
            }
            printf("message Recieved id =%d\n",msgProcess.Process.ID);
            if (msgProcess.Process.ID != -1) {
                Enqueue(Ready_Processes,msgProcess.Process,rr);
                test = AllocateProcess(&msgProcess.Process);
                printMeomry(&msgProcess.Process);
            }
        }
    }
}



void handler(int signum) // THe SIGUSER1 signal handler
{
    stillSending = false;
}

#include "headers.h"

void HPF( struct Queue** Ready_Processes);
void SRTN( struct Queue** Ready_Processes);
void RR( struct Queue** Ready_Processes,int quantum);

void start(struct Process* process);
void finish(struct Process process);
void resume(struct Process process);
void Pause(struct Process process);

FILE *outputFile1;
FILE *outputFile2;

double totalWTA = 0;
double totalWait = 0;
double totalRun = 0;
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
//========================================================================================================================
//Get the process path
    char processBuffer[500];
    getcwd(processBuffer, sizeof(processBuffer));
    ProcessPath = strcat(processBuffer, "/process.out");

//===================================== Readying command-line arguments  =================================================
    int type = atoi(argv[1]);                                               //1 for HPF , 2 for SRTN , 3 for RR
    Algo=type;
    int quantum = atoi(argv[2]);                                            //RR quantum    -1 if HPF or SRTN
//========================================================================================================================

//===================================== Initializing output files  =======================================================
    outputFile1 = fopen("scheduler.log", "w");
    outputFile2 = fopen("scheduler.perf", "w");
    fprintf(outputFile1, "#At time x process y state arr w total z remain y wait k\n");
//========================================================================================================================

        key_t key=ftok("keyfile",msgQueue);
        RecievingQueue=msgget(key,0666| IPC_CREAT);//created the queue that will recieve the process through
        int id_rows= shmget(SHKEY4, 4, IPC_CREAT | 0644);
        allProcessCount = (int *) shmat(id_rows, (void *)0, 0);



        struct Queue* Ready_Processes = createQueue();
        int temp = *allProcessCount;
        perProcessWTA = malloc(sizeof(int) * temp);

//===================================== Choose algorithm type ============================================================
    if (type == 1) {
        printf("HPF Algorithm starts\n");
        HPF(&Ready_Processes);
        printf("HPF Algorithm ends\n");
    } else if (type == 2) {
        printf("SRTN Algorithm starts\n");
        SRTN(&Ready_Processes);
        printf("SRTN Algorithm ends\n");
    } else if (type == 3) {
        printf("RR Algorithm starts\n");
        RR(&Ready_Processes,quantum);
        printf("RR Algorithm ends\n");
    } else {
        printf("Invalid Algorithm type \n");
    }

    float utilization = ((float)totalRun / getClk()) * 100.0;
    float avgWTA = totalWTA / finished_process_count;
    float avgWait = totalWait / finished_process_count;
    
    float totalStdWTA = 0;
    for (int i = 0; i < finished_process_count; i++)
    {
        totalStdWTA += (perProcessWTA[i] - avgWTA) * (perProcessWTA[i] - avgWTA);
    }
    
    float stdWTA = (totalStdWTA / finished_process_count);

    fprintf(outputFile2, "CPU utilization = %.2f %% \n", utilization);

    fprintf(outputFile2, "Avg WTA = %.2f\n", avgWTA);
    fprintf(outputFile2, "Avg Waiting = %.2f\n", avgWait);
    fprintf(outputFile2, "Std WTA = %.2f\n", stdWTA);


    fclose(outputFile1);
    fclose(outputFile2);

    destroyClk(true); //upon termination release the clock resources.
}
//========================================================================================================================

//============================================= Start Function ===========================================================
void start(struct Process* process) {
    if(process->ID == -1) {
        return;
    }
    process->pid = fork();
    if (process->pid == 0)
    { 
         printf("At time %d process %d started arr %d total %d remain %d wait %d\n",
            getClk(), process->ID, process->ArrivalTime, process->RunTime,
            process->RemainingTime, getClk() - process->ArrivalTime);
        //we need to send the remaining time to the generated process
        char Remaining_Time[20];
        sprintf(Remaining_Time,"%d",process->RemainingTime);//convert the remaining time into string to be sended to the created process

        execl(ProcessPath,"process.out",Remaining_Time,NULL);

        fprintf(outputFile1, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                getClk(), process->ID, process->ArrivalTime, process->RunTime,
                process->RemainingTime, getClk() - process->ArrivalTime);
    }

}
//========================================================================================================================

//============================================= Finish Function ==========================================================
void finish(struct Process process) {
    if(process.ID == -1) {
        return;
    }
    int finishTime = getClk();
    double TA = finishTime - process.ArrivalTime;
    double WTA = TA / process.RunTime;
    double wait = TA - process.RunTime;
    sleep(1);
    printf("At time %d process %d finished arr %d total %d remain 0 wait %.2f TA %.2f WTA %.2f\n",
            finishTime, process.ID, process.ArrivalTime, process.RunTime, wait, TA, WTA);
    fprintf(outputFile1, "At time %d process %d finished arr %d total %d remain 0 wait %.2f TA %.2f WTA %.2f\n",
            finishTime, process.ID, process.ArrivalTime, process.RunTime, wait, TA, WTA);

    perProcessWTA[currentProcessIndex] = WTA;
    currentProcessIndex++;
    totalWTA += WTA;
    totalWait += wait;
    totalRun += process.RunTime;
    finished_process_count++;
}

//========================================================================================================================

//============================================= Resume Function ==========================================================
void resume(struct Process process)
{
    int currentTime = getClk();
    if(process.ID == -1) {
        return;
    }
    kill(process.pid, SIGCONT);
    fprintf(outputFile1, "At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
    currentTime, process.ID, process.ArrivalTime, process.RunTime, process.RemainingTime,
    currentTime - process.ArrivalTime - (process.RunTime - process.RemainingTime));
    printf("At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
    currentTime, process.ID, process.ArrivalTime, process.RunTime, process.RemainingTime,
    currentTime - process.ArrivalTime - (process.RunTime - process.RemainingTime));
}
//========================================================================================================================

//============================================= Pause Function ===========================================================
void Pause(struct Process process)
{
    if(process.ID == -1) {
        return;
    }
    kill(process.pid , SIGSTOP);
    fprintf(outputFile1, "At time %d process %d stopped arr %d total %d remain %.2d wait %.2d\n",
    getClk(), process.ID, process.ArrivalTime, process.RunTime,process.RemainingTime,
    getClk() - process.ArrivalTime - (process.RunTime - process.RemainingTime));
    printf("At time %d process %d stopped arr %d total %d remain %.2d wait %.2d\n",
    getClk(), process.ID, process.ArrivalTime, process.RunTime,process.RemainingTime,
    getClk() - process.ArrivalTime - (process.RunTime - process.RemainingTime));
}
//========================================================================================================================

//============================================= HPF algorithm ============================================================
void HPF( struct Queue** Ready_Processes) {
    struct Process *process;
    int ID;
    while(true) {
        if(!isEmpty(Ready_Processes) )
        {
            process=Dequeue(Ready_Processes,1); 
            start(process);
            sleep(process->RemainingTime);
            finish(*process);
            if(ID ==-1 && isEmpty(Ready_Processes)) 
                return;
        }
        while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
                break;
            ID = msgProcess.Process.ID;
            printf("message Recieved id =%d\n",msgProcess.Process.ID);
            if (msgProcess.Process.ID != -1)
                Enqueue(Ready_Processes,msgProcess.Process,hpf);
        }
    }
}

// void SRTN(struct Queue** Ready_Processes) {
//     struct Process *process,*temp;
//     int ID;
//     while(true) {
//         if(!isEmpty(Ready_Processes) )
//         {
//             process=Dequeue(Ready_Processes,srtn);
//              if(ID ==-1 && isEmpty(Ready_Processes)) 
//                 return;
            
//             if (process->State==0) {
//                 start(process);
//                 process->State=1;
//                 sleep(1);
//                 process->RemainingTime=process->RemainingTime -1;

               
//             } else {
//                 resume(*process);
//                 sleep(1);
//                process->RemainingTime=process->RemainingTime -1;
//             }
//             if(process->RemainingTime==0)
//             {
//                 finish(*process);
//             }
//             temp=PeekFront(Ready_Processes);
//             if(!temp )
//             {
//                    if(temp->RemainingTime < process->RemainingTime)
//                    {
//                     pause(*process);
//                     Enqueue(Ready_Processes,msgProcess.Process,srtn);
//                    }
                
//             }
//             else{
//                 Enqueue(Ready_Processes,msgProcess.Process,srtn);
//             }

           
//         }
//         while(true) {
//             if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
//                 break;
//             ID = msgProcess.Process.ID;
//             printf("message Recieved id =%d\n",msgProcess.Process.ID);
//             if (msgProcess.Process.ID != -1)
//                 Enqueue(Ready_Processes,msgProcess.Process,srtn);







                
//         }
        
//     }
// }

//========================================================================================================================

//============================================= SRTN algorithm ============================================================
void SRTN(struct Queue** Ready_Processes) {
    struct Process *process;
    process->RemainingTime = 0;
    int remainingTime;
    int runTime;
    int ID;
    while(true) {
        if(!isEmpty(Ready_Processes) )
        {
            process=Dequeue(Ready_Processes,srtn);
            
            if (runTime == remainingTime) {
                start(process);
                sleep(process->RemainingTime);
                finish(*process);
            } else {
                resume(*process);
                sleep(process->RemainingTime);
                finish(*process);
            }

            if(ID ==-1 && isEmpty(Ready_Processes)) 
                return;
        }
        while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
                break;
            ID = msgProcess.Process.ID;
            printf("message Recieved id =%d\n",msgProcess.Process.ID);
            remainingTime = msgProcess.Process.RemainingTime;
            runTime = msgProcess.Process.RunTime;
            if (msgProcess.Process.ID != -1)
                if (msgProcess.Process.RemainingTime < process->RemainingTime){
                    Pause(*process);
                    Enqueue(Ready_Processes,*process,srtn);
                } else {
                    Enqueue(Ready_Processes,msgProcess.Process,srtn);
                }
        }
    }
}


//========================================================================================================================

//============================================= RR algorithm ============================================================
void RR( struct Queue** Ready_Processes,int quantum)
{
    int ID;
    struct Process *process;
    while(true) {
        if(!isEmpty(Ready_Processes) ){
        process=Dequeue(Ready_Processes,rr);
        if(ID ==-1 && isEmpty(Ready_Processes) ) 
            return;
        if(process->State==0) {
            start(process);
            if(process->RemainingTime >=quantum) {
                sleep(quantum);
                process->RemainingTime=process->RemainingTime-quantum;
                if( process->RemainingTime==0){
                    finish(*process);
                }
                else{
                Pause(*process);
                Enqueue(Ready_Processes,*process,rr);
                process->State=1;

                }
            }
        else {
            sleep(process->RemainingTime);
            
            finish(*process);
            }
        }
        else if(process->State==1) {
            resume( *process);
            if(process->RemainingTime >=quantum) {
                sleep(quantum);
                process->RemainingTime=process->RemainingTime-quantum;
                if( process->RemainingTime==0){
                    finish(*process);
                }
                else{
                Pause(*process);
                Enqueue(Ready_Processes,*process,rr);
                process->State=1;

                }
            }
        else {
            sleep(process->RemainingTime);
            
            finish(*process);
            }
            
        }

   }
      while(true) {
            if(msgrcv(RecievingQueue, &msgProcess, sizeof(msgProcess.Process),0, IPC_NOWAIT)==-1)
                break;
            ID = msgProcess.Process.ID;
            printf("message Recieved id =%d\n",msgProcess.Process.ID);
            if (msgProcess.Process.ID != -1)
                Enqueue(Ready_Processes,msgProcess.Process,rr);
        }

}
}
#include "headers.h"

int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    remainingtime = atoi(argv[1]);
    printf("the remaining time for the created process is %d ...\n",remainingtime);
    int temp = -1; 

    while (remainingtime > 0)
    {
        while (temp== getClk());
            if (temp != getClk()) {
                remainingtime--;
                temp = getClk();
            }
    }
    destroyClk(false);
    
    return 0;
}

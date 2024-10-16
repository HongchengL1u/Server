#include <unistd.h>
#include <sys/wait.h>
#include "SharedMemory/SharedMemory.h"

int main()
{

    pid_t ch_pid = fork();
    SharedMemory sm(1024,false);
    char* mem_ptr = sm.Get_Memptr();
    if(ch_pid == 0)
    {
        int* int_ptr = (int*)mem_ptr;
        int one = 0;
        int two = 1;
        int three; 
        for(int i=0;i<5;i++)
        {
            three = one+two;
            int_ptr[i] = three;
            one = two;
            two = three;
        }
        printf("child process exit!\n");
        exit(0);
    }
    else
    {
        waitpid(ch_pid, NULL, WCONTINUED);
        int* int_ptr = (int*)mem_ptr;
        for(int i=0;i<5;i++)
        {
            printf("index: %d, res: %d.\n",i,int_ptr[i]);
        }
    }
}
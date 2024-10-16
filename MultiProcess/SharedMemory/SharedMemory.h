#pragma once
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include "../util/util.h"
class SharedMemory
{
    public:
        SharedMemory(int size, bool flag):is_latest(flag)
        {
            CreateSharedMemwithKey(size);
        }
        ~SharedMemory() noexcept(false)
        {
            RestoreSharedMem();
        }
        char* Get_Memptr()
        {
            return mem_ptr;
        }
    private:
        void CreateSharedMemwithKey(int size)
        {
            // 这个语句也只是让生成的独特一点
            // key_t key = ftok("/Users/hongchengliu/Desktop/study/cpp/Server/MultiProcess/sm", 1234);
            key_t key = 1234;
            // IPC_CREAT 很重要
            shm_seg_id = shmget(key, size, S_IRUSR | S_IWUSR | IPC_CREAT);
            true_exit(shm_seg_id == -1,"shm_seg create error!");
            mem_ptr = (char*)shmat(shm_seg_id, NULL, 0);
            // return mem_ptr;
        }
        void RestoreSharedMem()
        {
            shmdt(mem_ptr);
            if(is_latest)
            {
                shmctl(shm_seg_id, IPC_RMID, NULL);
            }
        }

        int shm_seg_id = -1;
        char* mem_ptr = NULL;
        bool is_latest = false;
};
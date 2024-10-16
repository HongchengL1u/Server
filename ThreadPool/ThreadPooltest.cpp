#include "ThreadPool.h"
#include <unistd.h>
#include <iostream>
#include <string>
void taskFunc(void* arg){
    int num = *static_cast<int*>(arg);
    std::cout<<"thread"<<std::to_string(pthread_self())<<"working,number="<<num<<std::endl;
    sleep(0.1);
}

int main(){
    ThreadPool pool(10);
    for(int i = 0;i < 10000;++i){
        int* num = new int(i + 100);
        pool.add(taskFunc,num);
    }
    sleep(10);
    return 0;
}
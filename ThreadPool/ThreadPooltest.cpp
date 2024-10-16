#include"ThreadPool.h"
#include <unistd.h>
void taskFunc(void* arg){
    int num = *static_cast<int*>(arg);
    cout<<"thread"<<to_string(pthread_self())<<"working,number="<<num<<endl;
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
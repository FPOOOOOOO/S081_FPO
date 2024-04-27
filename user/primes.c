#include "kernel/types.h"
#include "user/user.h"
// #include <stddef.h>

void process(int p[]){
    int prime;
    close(p[1]);

    if(read(p[0], &prime, sizeof(prime)) > 0){
        printf("prime %d\n", prime);
        int p2[2];
        pipe(p2);
        if(fork() > 0){
            close(p2[0]);
            int i;
            while(read(p[0], &i, sizeof(i)) > 0){
                if(i % prime){
                    write(p2[1], &i, sizeof(i));
                }
            }
            close(p2[1]);
            wait(0);
        }else{
            close(p[0]);
            process(p2);
        }
        exit(0);
    }
}

int main(){
    int p[2];
    pipe(p);
    int pid = fork();

    if(pid > 0){
        close(p[0]);
        printf("prime 2\n");
        for(int i = 3; i <= 35; i++){
            if(i % 2){
                write(p[1], &i, sizeof(i));
            }
        }
        close(p[1]);
        wait(0);
    }else{
        process(p);
    }
    exit(0);
}
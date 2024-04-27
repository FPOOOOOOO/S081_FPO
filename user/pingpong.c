#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(){
    int p[2];
    pipe(p);
    int pid = fork();
    if(pid == 0){
        close(p[1]); // close child write end
        char buf[1];
        read(p[0], buf, 1);
        printf("%d: received ping\n", getpid());
        close(p[0]); // close child read end
        write(p[1], "1", 1);
        close(p[1]);
        exit(0);
    } else {
        close(p[0]); // close parent read end
        write(p[1], "1", 1);
        close(p[1]); // close parent write end
        wait(0); // wait for child to finish
        char buf[1];
        read(p[0], buf, 1);
        printf("%d: received pong\n", getpid());
        close(p[0]);
        exit(0);
    }
}
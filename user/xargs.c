#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(2, "xargs: no command given\n");
        exit(1);
    }

    char buf[512];
    char *xargv[MAXARG] = {0};
    int occupy = 0;
    int stdin_end = 0;

    for (int i = 1; i < argc; i++)
    {
        xargv[i - 1] = argv[i];
    }

    while (!(stdin_end && occupy == 0))
    {
        if (!stdin_end)
        {
            int remain_size = sizeof(buf) - occupy;
            int read_size = read(0, buf + occupy, remain_size);
            if (read_size < 0)
            {
                fprintf(2, "xargs: read error\n");
                exit(1);
            }
            if (read_size == 0)
            {
                close(0);
                stdin_end = 1;
            }
            occupy += read_size;
        }

        char *line_end = strchr(buf, '\n');
        while (line_end)
        {
            char xbuf[512] = {0};
            memcpy(xbuf, buf, line_end - buf);
            xargv[argc - 1] = xbuf;
            int pid = fork();
            if (pid == 0)
            {
                if (!stdin_end)
                {
                    close(0);
                }
                // printf("xargs: %s\n", xargv);
                // for (int i = 0; i < MAXARG && xargv[i]; i++)
                // {
                //     printf("xargv[%d] = \"%s\"\n", i, xargv[i]);
                // }
                if (exec(argv[1], xargv) < 0)// 这里不是很理解,就是要这样 exec("echo","echo 123") 不能 exec("echo","123");
                { 
                    fprintf(2, "xargs: exec error\n");
                    exit(1);
                }
            }
            else
            {
                memmove(buf, line_end + 1, occupy - (line_end - buf) - 1);
                occupy -= (line_end - buf) + 1;
                memset(buf + occupy, 0, sizeof(buf) - occupy);
                wait(0);
                line_end = strchr(buf, '\n');
            }
        }
    }

    exit(0);
}

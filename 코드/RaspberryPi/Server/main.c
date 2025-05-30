#include "../incs/main.h"

char threadName[5][20] = {"jetsonOutside", "jetsonInside", "raspberry", "arduino", "stm"};
void *(*threadFunc[5])(void *) = {jetsonOneThread, jetsonTwoThread, raspberryThread, arduinoThread, stmThread};

int main(void)
{
    int serverfd, max;
    fd_set fds, wfds, rfds;
    socklen_t addr_len;
    struct sockaddr_in addr;
    char bufRead[BUFSIZE];
    t_client clients[1024];
    int state = GET_RFID;
    bool wheelChair, people;
    int disabled = 0, notDisabled = 0;
    t_database rfidData;
    t_data **datas;

    serverfd = startSocket(&addr, &addr_len);
    datas = init();

    max = serverfd;
    printf("before while, max is:%d\n", max);
    FD_SET(serverfd, &fds);
    while (1) 
    {
        wfds = rfds = fds;

        if (max == 8)
        {
            mainThread(&state, &wheelChair, &people, &disabled, &notDisabled);
            endThings(datas);
        }

        if (select(max + 1, &wfds, &rfds, NULL, NULL) < 0)
            continue;

        for (int i = 0; i <= max; i++)
        {
            if (FD_ISSET(i, &wfds) && i == serverfd)
            {
                int clientSock = accept(serverfd, (struct sockaddr *)&addr, &addr_len);
                if (clientSock < 0)
                    continue;
                max = (clientSock > max) ? clientSock : max;
                printf("max is:%d\n", max);
                clients[clientSock].clientfd = clientSock;
                FD_SET(clientSock, &fds);
                break;
            }

            if (FD_ISSET(i, &wfds) && i != serverfd)
            {
                int res = read(i, bufRead, BUFSIZE);
                if (res <= 0)
                {
                    FD_CLR(i, &fds);
                    close(i);
                    break;
                }
                else
                {
                    for (int j = 0; j < 5; j++)
                    {
                        if (strncmp(threadName[j], bufRead, res) == 0)
                        {
                            datas[j]->state = &state;
                            datas[j]->clientfd = clients[i].clientfd;
                            datas[j]->wheelChair = &wheelChair;
                            datas[j]->people = &people;
                            datas[j]->rfidData = &rfidData;
                            datas[j]->disabled = &disabled;
                            datas[j]->notDisabled = &notDisabled;
                            pthread_create(&(datas[j]->pid), NULL, threadFunc[j], (void *)datas[j]);
                            FD_CLR(i, &fds);
                            if (max == 7)
                                max++;
                            if (j == RASPBERRY)
                                speakerOn(true, clients[i].clientfd, NULL);
                            break;
                        }
                    }
                }
            }
        }
    }
}



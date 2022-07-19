#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 200
#define NAME_SIZE 20

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *message);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];


int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thred_return;

    if (argc != 4)
    {
        printf("Usage : %s <IP> <port> <name> \n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    write(sock, argv[3], strlen(argv[3]));

    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
    pthread_join(snd_thread, &thred_return);
    pthread_join(rcv_thread, &thred_return);

    write(sock, argv[3], sizeof(argv[3]));
    write(sock, "님이 종료하였습니다.\n", strlen("님이 종료하셨습니다"));
    close(sock);

    return 0;
}

void *send_msg(void *arg) // send thread main
{
    int sock = *((int *)arg); //클라이언트의 파일 디스크립터
    char name_msg[NAME_SIZE + BUF_SIZE]; //크기를 이름과 버퍼사이즈로 넣어준다

    while (1)
    {           
        fgets(msg, BUF_SIZE, stdin);        
        sprintf(name_msg, "%s %s", name, msg); //client 이름과 msg를 합친다
        write(sock, name_msg, strlen(name_msg)); //서버로 문자열을 보냄
        if (!strcmp(msg, "나가기\n"))
        {
            close(sock); // 소켓 종료
            exit(1);
        }
    }
    return NULL;
}

void *recv_msg(void *arg) // read thread main
{
    int sock = *((int *)arg); // 클라이언트의 파일 디스크립터
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len;

    while (1)
    {
        memset(name_msg, 0, sizeof(name_msg));
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE-1);    
        //printf("길이 : %d\n", str_len);    
        name_msg[str_len]=0;

        if (strstr(name_msg, "displayClear")!=NULL)  
        {
            system("clear");              
            continue;
        }
        else if (str_len == -1) // read 실패시
        {
            printf("메세지를 읽어오지 못했습니다.\n");
            return (void *)-1;
        }       
        
        fputs(name_msg, stdout);   
    }
    return NULL;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

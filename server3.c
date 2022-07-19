#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 200
#define NAME_SIZE 20

void *handle_clnt(void *arg);
void show_room();
void send_msg(char *msg, int len);
void send_msg_clnt(char *msg, int len, int clnt_sock, int type);
void send_num(int num, int len);
void error_handling(char *msg);

int clnt_cnt = 0;
int type = 0;
int printout = 0;
pthread_mutex_t mutx;

typedef struct Node
{
    struct Node *next;
    char name[30];
    int clnt_sock;
    int type;
} Node;
Node *head;

void init_list() //연결리스트 헤드부분 정의
{
    head = malloc(sizeof(Node));
    head->next = NULL;
}

void Player_set(char *name, int clnt_sock) //사람이 들어올때마다 연결리스트 설정
{
    Node *cur = head;
    Node *new_name = malloc(sizeof(Node));

    if (cur->next == NULL) //첫번째 노드
    {
        strcpy(new_name->name, name);
        new_name->clnt_sock = clnt_sock;
        new_name->type = 0;
        new_name->next = NULL;
        cur->next = new_name;
    }
    else
    {
        while (cur->next != NULL)
        {
            cur = cur->next;
        }
        strcpy(new_name->name, name);
        new_name->clnt_sock = clnt_sock;
        new_name->type = 0;
        new_name->next = NULL;
        cur->next = new_name;
    }
}

void remove_name(int clnt_sock)
{
    Node *cur = head;
    Node *temp = cur;

    while (cur != NULL)
    {
        if (cur->clnt_sock == clnt_sock)
        {
            temp->next = cur->next;
            free(cur); // cur 동적할당 해제
            break;
        }
        temp = cur; // temp에 cur을 담는다
        cur = cur->next;
    }
}

void delete_list() //모든 연결리스트 해제
{
    Node *cur = head;
    Node *temp;

    while (cur != NULL) // cur가 NULL일때까지
    {
        temp = cur->next;
        free(cur); // cur 동적할당 해제
        cur = temp;
    }
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock, str_len;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    char name[NAME_SIZE];
    pthread_t t_id;

    init_list(); //노드 헤드 정의

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL); //뮤텍스 정의
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_cnt++;

        if (clnt_sock != -1) //연결성공
        {
            fflush(stdout);
            memset(name, 0, sizeof(name));
            read(clnt_sock, name, sizeof(name));
            Player_set(name, clnt_sock);
            show_room();
        }
        pthread_mutex_unlock(&mutx);

        printf("현재 연결된 인원수 : %d\n", clnt_cnt);
        printf("클라이언트 IP : %s\n", inet_ntoa(clnt_adr.sin_addr));
        printf("클라이언트 이름 : %s\n", name);
        printf("클라이언트 소켓 : %d번\n\n", clnt_sock);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock); //클라이언트에서 정보를 받는 쓰레드
        pthread_detach(t_id);
    }
    close(serv_sock);
    delete_list();

    return 0;
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0;
    srand(time(NULL));
    int ran = rand() % 1000 + 11;
    Node *cur = malloc(sizeof(Node));
    cur = head->next;
    Node *list = head->next;

    char *arr = NULL;
    char *cut;
    char temp[BUF_SIZE];
    char slt[BUF_SIZE];
    char name[BUF_SIZE];
    char msg[BUF_SIZE];

    while (cur != NULL)
    {
        if (cur->clnt_sock == clnt_sock)
            break;
        cur = cur->next;
    }
    while (1)
    {
        while (1)
        {
            memset(slt, 0, sizeof(slt));
            str_len = read(clnt_sock, slt, sizeof(slt));

            if (strstr(slt, "나가기") != NULL) // read로 읽은 데이터중 "나가기"가 있을 시
                break;

            if (cur->type != 0) //대기방이 아니면 바로 메세지 출력
            {
                send_msg_clnt(slt, strlen(slt), clnt_sock, cur->type);
                break;
            }

            if (strstr(slt, "방만들기") != NULL) // read로 읽은 데이터중 "방만들기"가 있을 시
            {
                send_msg_clnt("\n▶  초대할 사람의 이름을 적어주세요 (초대완료 : '그만하기' 입력)\n\n", strlen("\n▶  초대할 사람의 이름을 적어주세요 (초대완료 : '그만하기' 입력)\n\n"), clnt_sock, cur->type);
                type++; //방 번호 ++
                clnt_cnt--;
                cur->type = ran; //방 생성중 다른사람이 채팅을 못보게 랜덤난수로 지정
                break;
            }
            send_msg_clnt("잘못 입력하셨습니다.\n", strlen("잘못 입력하셨습니다.\n"), clnt_sock, cur->type);
        }
        if (strstr(slt, "나가기") != NULL) // read로 읽은 데이터중 "나가기"가 있을 시
            break;

        if (cur->type >= 11) //방을 생성중일 시
        {
            while (1)
            {
                list = head->next;
                memset(name, 0, sizeof(name));
                str_len = read(clnt_sock, name, sizeof(name));

                cut = strtok(name, "] "); //] 이후에 나오는 문자열을 분리(주소값 따로저장) srttok(문자열,구문자)
                while (cut != NULL)
                {
                    arr = cut;                //자른 주소를 arr[1]에 저장
                    cut = strtok(NULL, "] "); //구분자 뒤에서 다른 구분자 찾기
                }
                // printf("받은 메세지 : %s\n", arr);

                if (strstr(arr, "그만하기") != NULL) // strtok로 자른 부분부터 그만하기가 있다면
                {
                    write(cur->clnt_sock, "displayClear", NAME_SIZE+BUF_SIZE-1);
                    break;
                }

                while (list != NULL)
                {
                    if (!strncmp(arr, list->name, strlen(list->name)))
                    {
                        if (list->type == 0) //대상이 대기방일때
                        {
                            write(clnt_sock, "┌────────────────────────────────────┐\n", strlen("┌────────────────────────────────────┐\n"));
                            write(clnt_sock, "│", strlen("│"));
                            for (int i = 0; i < (37 - (strlen(list->name) + strlen("님을 초대했습니다")) * 2 / 3) / 2; i++) //임시값
                                write(clnt_sock, " ", strlen(" "));
                            write(clnt_sock, list->name, strlen(list->name));
                            write(clnt_sock, "님을 초대했습니다", strlen("님을 초대했습니다"));
                            for (int i = 0; i < (37 - (strlen(list->name) + strlen("님을 초대했습니다")) * 2 / 3) / 2 - 1; i++) //임시값
                                write(clnt_sock, " ", strlen(" "));
                            write(clnt_sock, "│\n", strlen("│\n"));
                            write(clnt_sock, "└────────────────────────────────────┘\n\n", strlen("└────────────────────────────────────┘\n\n"));

                            write(list->clnt_sock, "displayClear", NAME_SIZE+BUF_SIZE-1);
                            write(list->clnt_sock, "┌────────────────────────────────────┐\n", strlen("┌────────────────────────────────────┐\n"));
                            write(list->clnt_sock, "│", strlen("│"));
                            for (int i = 0; i < (37 - (strlen(cur->name) + strlen("님에게 초대받았습니다")) * 2 / 3) / 2; i++)
                                write(list->clnt_sock, " ", strlen(" "));
                            write(list->clnt_sock, cur->name, strlen(cur->name));
                            write(list->clnt_sock, "님에게 초대받았습니다", strlen("님에게 초대받았습니다"));
                            for (int i = 0; i < (37 - (strlen(cur->name) + strlen("님에게 초대받았습니다")) * 2 / 3) / 2 - 1; i++)
                                write(list->clnt_sock, " ", strlen(" "));
                            write(list->clnt_sock, "│\n", strlen("│\n"));
                            write(list->clnt_sock, "└────────────────────────────────────┘\n", strlen("└────────────────────────────────────┘\n"));
                            list->type = type;
                            clnt_cnt--;
                            write(list->clnt_sock, "┌─────────────────────────────┐\n", strlen("┌─────────────────────────────┐\n"));
                            write(list->clnt_sock, "│   채팅방에 입장하였습니다   │\n", strlen("│   채팅방에 입장하였습니다   │\n"));
                            write(list->clnt_sock, "└─────────────────────────────┘\n", strlen("└─────────────────────────────┘\n"));

                            break;
                        }
                    }
                    list = list->next;
                }      
                if(list ==NULL)          
                    write(clnt_sock, "\n대기방에 없는 친구입니다.\n\n", strlen("\n대기방에 없는 친구입니다.\n\n"));
            }
            cur->type = type; //사용자를 채팅방으로
            write(clnt_sock, "┌───────────────────────────┐\n", strlen("┌───────────────────────────┐\n"));
            write(clnt_sock, "│   채팅방을 만들었습니다   │\n", strlen("│   채팅방을 만들었습니다   │\n"));
            write(clnt_sock, "└───────────────────────────┘\n", strlen("└───────────────────────────┘\n"));
        }

        while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
        {
            arr = NULL;
            strcpy(temp, msg);

            cut = strtok(temp, "] "); //] 이후에 나오는 문자열을 분리(주소값 따로저장) srttok(문자열,구문자)
            while (cut != NULL)
            {
                arr = cut;                //자른 주소를 arr에 저장
                cut = strtok(NULL, "] "); //구분자 뒤에서 다른 구분자 찾기
            }
            // printf("받은 메세지 : %s\n", arr);

            if (strstr(arr, "방나가기") != NULL) // strtok로 자른 부분부터 나가기가 있다면
            {
                send_msg_clnt(cur->name, strlen(cur->name), clnt_sock, cur->type);
                send_msg_clnt("님이 채팅방을 나갔습니다.\n", strlen("님이 채팅방을 나갔습니다.\n"), clnt_sock, cur->type);
                cur->type = 0;
                clnt_cnt++;
                show_room();
                break;

                // return NULL;
            }
            send_msg_clnt(msg, str_len, clnt_sock, type);
            memset(msg, 0, sizeof(msg));
        }
    }
    printf("- 클라이언트 종료 요청 -\n\n");

    cur->type = 0;
    clnt_cnt--;
    if(clnt_cnt<0)
        clnt_cnt=0;
    remove_name(clnt_sock); //대상이 종료할 때
    show_room();
    close(clnt_sock);

    return NULL;
}

void show_room()
{
    Node *list = head->next;
    char buf[BUF_SIZE];

    while (list != NULL)
    {     
        if (list->type == 0)
        {
            write(list->clnt_sock, "displayClear", BUF_SIZE+NAME_SIZE-1);            
            usleep(10000);  
            write(list->clnt_sock, "┌──────────────────┬─────────────────┐\n", strlen("┌──────────────────┬─────────────────┐\n"));
            write(list->clnt_sock, "│   채팅방 만들기  │  채팅방 나가기  │\n", strlen("│   채팅방 만들기  │  채팅방 나가기  │\n"));
            write(list->clnt_sock, "├──────────────────┼─────────────────┤\n", strlen("├──────────────────┼─────────────────┤\n"));
            write(list->clnt_sock, "│   방만들기 입력  │   나가기 입력   │\n", strlen("│   방만들기 입력  │   나가기 입력   │\n"));
            write(list->clnt_sock, "├──────────────────┴─────────────────┤\n", strlen("├──────────────────┴─────────────────┤\n"));
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "│    현재 서버 접속자 인원 / %d명     │\n", clnt_cnt);
            write(list->clnt_sock, buf, strlen(buf));
            write(list->clnt_sock, "├────────────────────────────────────┤\n", strlen("├────────────────────────────────────┤\n"));
            Node *temp = head->next;
            while (temp != NULL)
            {
                if (temp->type == 0)
                {
                    write(list->clnt_sock, "│", strlen("│"));
                    for (int i = 0; i < (37 - strlen(temp->name) * 2 / 3) / 2; i++) //임시값
                        write(list->clnt_sock, " ", strlen(" "));
                    write(list->clnt_sock, temp->name, strlen(temp->name));
                    for (int i = 0; i < (37 - strlen(temp->name) * 2 / 3) / 2; i++) //임시값
                        write(list->clnt_sock, " ", strlen(" "));
                    write(list->clnt_sock, "│\n", strlen("│\n"));
                }
                temp = temp->next;
            }
            write(list->clnt_sock, "└────────────────────────────────────┘\n\n", strlen("└────────────────────────────────────┘\n\n"));
        }
        list = list->next;
    }
}

void send_msg_clnt(char *msg, int len, int clnt_sock, int type) // 특정 조건을 가진 클라이언트에게
{
    Node *cur = malloc(sizeof(Node));
    cur = head->next;

    pthread_mutex_lock(&mutx);
    while (cur != NULL)
    {
        if(cur->type == type)
        {
            if (cur->type > 10 && clnt_sock == cur->clnt_sock) //대상이 방을 만들고 있는 대상에게
                write(cur->clnt_sock, msg, len);
            else if (type != 0 && clnt_sock != cur->clnt_sock && cur->type == type) //대상이 대기방이 아니면 나를 제외한 같은 채팅방 사람들에게
                write(cur->clnt_sock, msg, len);
            else if (clnt_sock == cur->clnt_sock) //나에게만
                write(cur->clnt_sock, msg, len);
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
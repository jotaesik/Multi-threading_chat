#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 100	//msg사이즈 100제한
#define NAME_SIZE 20	//이름사이즈 20제한
	
void * send_msg(void * arg);	//미리선언
void * recv_msg(void * arg);	//미리선언
void error_handling(char * msg);	//미리선언
	
char name[NAME_SIZE]="[DEFAULT]";	//name문자배열에 디폴드값 저장
char msg[BUF_SIZE];	//100크기의 문자msg배열선언
	
int main(int argc, char *argv[])	//매개변수 받는 main함수
{
	int sock;	//sock변수 선언
	struct sockaddr_in serv_addr;	//주소,포트,ip갖춘 구조체 선언
	pthread_t snd_thread, rcv_thread;	//받는 쓰레드와 보내는 쓰레드 선언
	void * thread_return;	//pthread_join에 사용
	if(argc!=4) {	//인자가 4개가 아니라면
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);	//ex)./chat_clnt 127.0.0.1 9999 one 입력해주세요!
		exit(1);	//종료
	 }
	
	sprintf(name, "[%s]", argv[3]);	//name에 매개변수3번째것이 문자열로 저장
	sock=socket(PF_INET, SOCK_STREAM, 0);	//ipv4프로토콜 사용,tcp프로토콜사용,0을 매개변수로 한 socket()함수에 대한 디스크립러 반환
											//-1이 반환되면 소켓 생성실패.
	
	memset(&serv_addr, 0, sizeof(serv_addr));		//&serv_adr부터 serv_adr의 크기만큼 0으로 세팅
	serv_addr.sin_family=AF_INET;	////주소체계에 AF_INET저장
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);	//argv[1]로 받은 문자열을 빅엔디안32bit로 변환해서 sin_addr.s_addr에 저장
	serv_addr.sin_port=htons(atoi(argv[2]));	//argv[2]로 받은 문자열을 숫자로 변환해서 port에 저장
												//2byte데이터를 네트워크 byte order로 변경.
	  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)	//서버로 접속 요청 실패시 -1 반환
		error_handling("connect() error");	//에러
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);	//쓰레드 생성, 기본특성위해 NULL, 쓰레드 함수는 send_msg, 그리고 socket()에서 받은 &sock
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);	//쓰레드 생성, 기본특성위해 NULL, 쓰레드 함수는 recv_msg, 그리고 socket()에서 받은 &sock
	pthread_join(snd_thread, &thread_return);	//snd_thread종료 대기
	pthread_join(rcv_thread, &thread_return);	//rcv_thraed종료 대기
	close(sock);	//소켓종료
	return 0;	//리턴
}
	
void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);	//위의 pthread_create에서 (void*)&sock 으로 매개변수를 주었으므로 int sock에 넣기위해서 int형 변환
	char name_msg[NAME_SIZE+BUF_SIZE];	//NAME_SIZE와BUF_SIZE를 합한것크기의 name_msg문자배열선언
	while(1)	//반복
	{
		fgets(msg, BUF_SIZE, stdin);	//입력받은걸 BUF_SIZE만큼 msg저장
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n"))	//만약 q나	Q가 입력시
		{
			close(sock);	//소켓종료
			exit(0);	//종료
		}
		sprintf(name_msg,"%s %s", name, msg);	//name_msg에 name과 이어서 msg 저장
		write(sock, name_msg, strlen(name_msg));	//서버로 메시지 보내기
	}
	return NULL;	//return NULL
}
	
void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);	//위의 pthread_create에서 (void*)&sock 으로 매개변수를 주었으므로 int sock에 넣기위해서 int형 변환
	char name_msg[NAME_SIZE+BUF_SIZE];	//NAME_SIZE와BUF_SIZE를 합한것크기의 name_msg문자배열선언
	int str_len;	//문자열길이 선언
	while(1)	//반복
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);	//서버에서 보낸 name_msg를 NAME_SIZE+BUF_SIZE-1만큼 읽음
															//실패시 -1 반환
		if(str_len==-1) //read실패시
			return (void*)-1;	
		name_msg[str_len]=0;	//마지막 값에 NULL저장 종료알리기위해서
		fputs(name_msg, stdout);	//name_msg출력
	}
	return NULL;	//return nULL
}
	
void error_handling(char *msg)	//에러메시지 출력함수
{
	fputs(msg, stderr);	//msg입력
	fputc('\n', stderr);	//개행
	exit(1);	//종료
}

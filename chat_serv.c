#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100	//글자수 100제한
#define MAX_CLNT 256	//접속자수 256제한

void * handle_clnt(void * arg);	//미리선언
void send_msg(char * msg, int len);	//미리선언
void error_handling(char* msg);	//미리선언

int clnt_cnt=0;	//접속한사람 몇명인지 변수
int clnt_socks[MAX_CLNT];	//생성된 소켓의 파일디스크립터 저장 배열
pthread_mutex_t mutx;	//스레드를 위한 뮤텍스 선언

int main(int argc, char *argv[])	//메인함수
{
	int serv_sock, clnt_sock;	//선언
	struct sockaddr_in serv_adr, clnt_adr;	//주소,포트,ip갖춘 구조체 선언
	int clnt_adr_sz;	//클라이언트 주소 사이즈 선언
	pthread_t t_id;	//쓰레드 선언
	if(argc!=2) {	//만약 ./c 포트번호가 아닐경우
		printf("Usage : %s <port>\n", argv[0]);	//이렇게 입력해주세요!!
		exit(1);	//종료
	}
  
	pthread_mutex_init(&mutx, NULL);	//뮤텍스 초기화
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);	//ipv4프로토콜 사용,tcp프로토콜사용,0을 매개변수로 한 socket()함수에 대한 디스크립러 반환
												//-1이 반환되면 소켓 생성실패.

	memset(&serv_adr, 0, sizeof(serv_adr));	//&serv_adr부터 serv_adr의 크기만큼 0으로 세팅
	serv_adr.sin_family=AF_INET;	//주소체계에 AF_INET저장
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);	//sin_addr중 구조체인 s_adrr에 저장
												//4byte데이터를 네트워크 byte order로 변경.
												//이 컴퓨터에서 존재하는 랜카드중 사용가능한 랜카드ip주소 사용.
	serv_adr.sin_port=htons(atoi(argv[1]));	//argv[1]로 받은 문자열을 숫자로 변환해서 port에 저장
											//2byte데이터를 네트워크 byte order로 변경.
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)	//서버소켓과 &serv)adr구조체, 주소구조체의 길이로 bind()함수 호출 성공하면 0 실패하면 -1
		error_handling("bind() error");	//bind()에러 
	if(listen(serv_sock, 5)==-1)	//serv_sock과 연결요구 개수의 최대값 5로 지정 성공시 0반화, 실패시 -1 반환, 가볍게 말하면 놀이공원에 한번에 줄을 설 수 있는 인원이다
									//256으로도 변경가능
		error_handling("listen() error");	//listen()에러
	
	while(1)	//계속 반복
	{
		clnt_adr_sz=sizeof(clnt_adr);	//clnt_adr의 크기를 clnt_adr_sz에 저장
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);	//serv_sock에 clnt가 연결이 되는지 확인, 실패시 -1 반환
		
		pthread_mutex_lock(&mutx);	//임계영역에 진입하기 위해 잠금
		clnt_socks[clnt_cnt++]=clnt_sock;	//accpet()받은 clnt_sock를 clont_socks배열에 저장
		pthread_mutex_unlock(&mutx);	//잠금 해제
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);	//쓰레드 생성, 기본특성위해 NULL, 쓰레드 함수는 handle_clnt, 그리고 accpet()에서 받은 &clnt_sock
		pthread_detach(t_id);	//쓰레드 종료될때 모든 자원 해제
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));	//inet_nota를 이용하여 네트워크 바이트 순서의 32비트값을 변환
	}
	close(serv_sock);	//서버소켓 종료
	return 0;	//끝
}
	
void * handle_clnt(void * arg)	//핸들함수 매개변수 *arg
{
	int clnt_sock=*((int*)arg);	//위의 pthread_create에서 (void*)&clnt_sock으로 매개변수를 주었으므로 int clnt_sock에 넣기위해서 int형 변환
	int str_len=0, i;	//문자열 길이, 반복할때 사용할 i선언
	char msg[BUF_SIZE];	//BUF_SIZE만큼의 메시지문자배열 선언	
	
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)	//클라이언트에서 보낸 메시지를 메시지 크기만큼 읽음, 0이 될때까지(즉 Q나 q를 입력하면)
		send_msg(msg, str_len);	//send_msg함수 호출
		
	pthread_mutex_lock(&mutx);	//임계영역에 진입하기 위해 잠금
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(clnt_sock==clnt_socks[i])	//q로 소켓종료한 clnt_socks에서 찾기
		{
			while(i <clnt_cnt-1)	//0 1 2 3 4 에서 1이 나가면 0 2 3 4 가 되므로 뒤에 있는 소켓들 앞으로 옮기기 하나가  빠졌으므로
			{
				clnt_socks[i]=clnt_socks[i+1];	//옮기기
				  i++;	//다음 소켓

			}

			break;	//for문 종료
		}
	}
	clnt_cnt--;	//하나가 종료되었으므로 -1
	pthread_mutex_unlock(&mutx);	//잠금해제
	close(clnt_sock);	//사라진 소켓 종료
	return NULL;	//NULL반환
}
void send_msg(char * msg, int len)   // send to all
{
	int i;	//반복 변수
	pthread_mutex_lock(&mutx);	//임계영역에 진입하기 위해 잠금
	for(i=0; i<clnt_cnt; i++)	//반복시작
		write(clnt_socks[i], msg, len);	//모든 클라이언트에게 msg보내기
	pthread_mutex_unlock(&mutx);	//잠금 해제
}	
void error_handling(char * msg)	//에러메시지 출력함수
{
	fputs(msg, stderr);	//msg입력
	fputc('\n', stderr);	//개행
	exit(1);	//종료
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 100	//msg������ 100����
#define NAME_SIZE 20	//�̸������� 20����
	
void * send_msg(void * arg);	//�̸�����
void * recv_msg(void * arg);	//�̸�����
void error_handling(char * msg);	//�̸�����
	
char name[NAME_SIZE]="[DEFAULT]";	//name���ڹ迭�� �����尪 ����
char msg[BUF_SIZE];	//100ũ���� ����msg�迭����
	
int main(int argc, char *argv[])	//�Ű����� �޴� main�Լ�
{
	int sock;	//sock���� ����
	struct sockaddr_in serv_addr;	//�ּ�,��Ʈ,ip���� ����ü ����
	pthread_t snd_thread, rcv_thread;	//�޴� ������� ������ ������ ����
	void * thread_return;	//pthread_join�� ���
	if(argc!=4) {	//���ڰ� 4���� �ƴ϶��
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);	//ex)./chat_clnt 127.0.0.1 9999 one �Է����ּ���!
		exit(1);	//����
	 }
	
	sprintf(name, "[%s]", argv[3]);	//name�� �Ű�����3��°���� ���ڿ��� ����
	sock=socket(PF_INET, SOCK_STREAM, 0);	//ipv4�������� ���,tcp�������ݻ��,0�� �Ű������� �� socket()�Լ��� ���� ��ũ���� ��ȯ
											//-1�� ��ȯ�Ǹ� ���� ��������.
	
	memset(&serv_addr, 0, sizeof(serv_addr));		//&serv_adr���� serv_adr�� ũ�⸸ŭ 0���� ����
	serv_addr.sin_family=AF_INET;	////�ּ�ü�迡 AF_INET����
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);	//argv[1]�� ���� ���ڿ��� �򿣵��32bit�� ��ȯ�ؼ� sin_addr.s_addr�� ����
	serv_addr.sin_port=htons(atoi(argv[2]));	//argv[2]�� ���� ���ڿ��� ���ڷ� ��ȯ�ؼ� port�� ����
												//2byte�����͸� ��Ʈ��ũ byte order�� ����.
	  
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)	//������ ���� ��û ���н� -1 ��ȯ
		error_handling("connect() error");	//����
	
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);	//������ ����, �⺻Ư������ NULL, ������ �Լ��� send_msg, �׸��� socket()���� ���� &sock
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);	//������ ����, �⺻Ư������ NULL, ������ �Լ��� recv_msg, �׸��� socket()���� ���� &sock
	pthread_join(snd_thread, &thread_return);	//snd_thread���� ���
	pthread_join(rcv_thread, &thread_return);	//rcv_thraed���� ���
	close(sock);	//��������
	return 0;	//����
}
	
void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);	//���� pthread_create���� (void*)&sock ���� �Ű������� �־����Ƿ� int sock�� �ֱ����ؼ� int�� ��ȯ
	char name_msg[NAME_SIZE+BUF_SIZE];	//NAME_SIZE��BUF_SIZE�� ���Ѱ�ũ���� name_msg���ڹ迭����
	while(1)	//�ݺ�
	{
		fgets(msg, BUF_SIZE, stdin);	//�Է¹����� BUF_SIZE��ŭ msg����
		if(!strcmp(msg,"q\n")||!strcmp(msg,"Q\n"))	//���� q��	Q�� �Է½�
		{
			close(sock);	//��������
			exit(0);	//����
		}
		sprintf(name_msg,"%s %s", name, msg);	//name_msg�� name�� �̾ msg ����
		write(sock, name_msg, strlen(name_msg));	//������ �޽��� ������
	}
	return NULL;	//return NULL
}
	
void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);	//���� pthread_create���� (void*)&sock ���� �Ű������� �־����Ƿ� int sock�� �ֱ����ؼ� int�� ��ȯ
	char name_msg[NAME_SIZE+BUF_SIZE];	//NAME_SIZE��BUF_SIZE�� ���Ѱ�ũ���� name_msg���ڹ迭����
	int str_len;	//���ڿ����� ����
	while(1)	//�ݺ�
	{
		str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);	//�������� ���� name_msg�� NAME_SIZE+BUF_SIZE-1��ŭ ����
															//���н� -1 ��ȯ
		if(str_len==-1) //read���н�
			return (void*)-1;	
		name_msg[str_len]=0;	//������ ���� NULL���� ����˸������ؼ�
		fputs(name_msg, stdout);	//name_msg���
	}
	return NULL;	//return nULL
}
	
void error_handling(char *msg)	//�����޽��� ����Լ�
{
	fputs(msg, stderr);	//msg�Է�
	fputc('\n', stderr);	//����
	exit(1);	//����
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100	//���ڼ� 100����
#define MAX_CLNT 256	//�����ڼ� 256����

void * handle_clnt(void * arg);	//�̸�����
void send_msg(char * msg, int len);	//�̸�����
void error_handling(char* msg);	//�̸�����

int clnt_cnt=0;	//�����ѻ�� ������� ����
int clnt_socks[MAX_CLNT];	//������ ������ ���ϵ�ũ���� ���� �迭
pthread_mutex_t mutx;	//�����带 ���� ���ؽ� ����

int main(int argc, char *argv[])	//�����Լ�
{
	int serv_sock, clnt_sock;	//����
	struct sockaddr_in serv_adr, clnt_adr;	//�ּ�,��Ʈ,ip���� ����ü ����
	int clnt_adr_sz;	//Ŭ���̾�Ʈ �ּ� ������ ����
	pthread_t t_id;	//������ ����
	if(argc!=2) {	//���� ./c ��Ʈ��ȣ�� �ƴҰ��
		printf("Usage : %s <port>\n", argv[0]);	//�̷��� �Է����ּ���!!
		exit(1);	//����
	}
  
	pthread_mutex_init(&mutx, NULL);	//���ؽ� �ʱ�ȭ
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);	//ipv4�������� ���,tcp�������ݻ��,0�� �Ű������� �� socket()�Լ��� ���� ��ũ���� ��ȯ
												//-1�� ��ȯ�Ǹ� ���� ��������.

	memset(&serv_adr, 0, sizeof(serv_adr));	//&serv_adr���� serv_adr�� ũ�⸸ŭ 0���� ����
	serv_adr.sin_family=AF_INET;	//�ּ�ü�迡 AF_INET����
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);	//sin_addr�� ����ü�� s_adrr�� ����
												//4byte�����͸� ��Ʈ��ũ byte order�� ����.
												//�� ��ǻ�Ϳ��� �����ϴ� ��ī���� ��밡���� ��ī��ip�ּ� ���.
	serv_adr.sin_port=htons(atoi(argv[1]));	//argv[1]�� ���� ���ڿ��� ���ڷ� ��ȯ�ؼ� port�� ����
											//2byte�����͸� ��Ʈ��ũ byte order�� ����.
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)	//�������ϰ� &serv)adr����ü, �ּұ���ü�� ���̷� bind()�Լ� ȣ�� �����ϸ� 0 �����ϸ� -1
		error_handling("bind() error");	//bind()���� 
	if(listen(serv_sock, 5)==-1)	//serv_sock�� ����䱸 ������ �ִ밪 5�� ���� ������ 0��ȭ, ���н� -1 ��ȯ, ������ ���ϸ� ���̰����� �ѹ��� ���� �� �� �ִ� �ο��̴�
									//256���ε� ���氡��
		error_handling("listen() error");	//listen()����
	
	while(1)	//��� �ݺ�
	{
		clnt_adr_sz=sizeof(clnt_adr);	//clnt_adr�� ũ�⸦ clnt_adr_sz�� ����
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);	//serv_sock�� clnt�� ������ �Ǵ��� Ȯ��, ���н� -1 ��ȯ
		
		pthread_mutex_lock(&mutx);	//�Ӱ迵���� �����ϱ� ���� ���
		clnt_socks[clnt_cnt++]=clnt_sock;	//accpet()���� clnt_sock�� clont_socks�迭�� ����
		pthread_mutex_unlock(&mutx);	//��� ����
	
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);	//������ ����, �⺻Ư������ NULL, ������ �Լ��� handle_clnt, �׸��� accpet()���� ���� &clnt_sock
		pthread_detach(t_id);	//������ ����ɶ� ��� �ڿ� ����
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));	//inet_nota�� �̿��Ͽ� ��Ʈ��ũ ����Ʈ ������ 32��Ʈ���� ��ȯ
	}
	close(serv_sock);	//�������� ����
	return 0;	//��
}
	
void * handle_clnt(void * arg)	//�ڵ��Լ� �Ű����� *arg
{
	int clnt_sock=*((int*)arg);	//���� pthread_create���� (void*)&clnt_sock���� �Ű������� �־����Ƿ� int clnt_sock�� �ֱ����ؼ� int�� ��ȯ
	int str_len=0, i;	//���ڿ� ����, �ݺ��Ҷ� ����� i����
	char msg[BUF_SIZE];	//BUF_SIZE��ŭ�� �޽������ڹ迭 ����	
	
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)	//Ŭ���̾�Ʈ���� ���� �޽����� �޽��� ũ�⸸ŭ ����, 0�� �ɶ�����(�� Q�� q�� �Է��ϸ�)
		send_msg(msg, str_len);	//send_msg�Լ� ȣ��
		
	pthread_mutex_lock(&mutx);	//�Ӱ迵���� �����ϱ� ���� ���
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(clnt_sock==clnt_socks[i])	//q�� ���������� clnt_socks���� ã��
		{
			while(i <clnt_cnt-1)	//0 1 2 3 4 ���� 1�� ������ 0 2 3 4 �� �ǹǷ� �ڿ� �ִ� ���ϵ� ������ �ű�� �ϳ���  �������Ƿ�
			{
				clnt_socks[i]=clnt_socks[i+1];	//�ű��
				  i++;	//���� ����

			}

			break;	//for�� ����
		}
	}
	clnt_cnt--;	//�ϳ��� ����Ǿ����Ƿ� -1
	pthread_mutex_unlock(&mutx);	//�������
	close(clnt_sock);	//����� ���� ����
	return NULL;	//NULL��ȯ
}
void send_msg(char * msg, int len)   // send to all
{
	int i;	//�ݺ� ����
	pthread_mutex_lock(&mutx);	//�Ӱ迵���� �����ϱ� ���� ���
	for(i=0; i<clnt_cnt; i++)	//�ݺ�����
		write(clnt_socks[i], msg, len);	//��� Ŭ���̾�Ʈ���� msg������
	pthread_mutex_unlock(&mutx);	//��� ����
}	
void error_handling(char * msg)	//�����޽��� ����Լ�
{
	fputs(msg, stderr);	//msg�Է�
	fputc('\n', stderr);	//����
	exit(1);	//����
}
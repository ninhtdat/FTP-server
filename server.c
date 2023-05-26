#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include "status.h"
#include "lib.h"
#define LOGIN "LOGIN"
#define LOGOUT "LOGOUT"
#define REGISTER "REGISTER"
#define BACKLOG 2
#define MAXLISTPATH 2048
pthread_mutex_t lock1;
int request_Id = 1;
char file_Repository[100];
Client online_Client[1000];
//
void init_Array_Client() {
	int i;
	for(i = 0; i < 1000; i++) {

		online_Client[i].upload_Success = 0; 
	}
}
//
int number_Elements_In_Array(char** t) {
	int i;
	for (i = 0; *(t + i); i++)
    {
    }
    return i;
}
//
int find_Avaiable_Element_In_Array_Client() {
	int i;
	for (i = 0; i < 1000; i++) {
		if(online_Client[i].request_Id == 0) {
			return i;
		}
	}
	return -1;
}
//
int find_Client(int request_Id) {
	int i;
	for (i = 0; i < 1000; i++) {
		if(online_Client[i].request_Id == request_Id) {
			return i;
		}
	}
	return -1;
}
//
void set_Client(int j, int request_Id, char* name, int conn_Sock) {
	if(j >= 0) {
		online_Client[j].request_Id = request_Id;
		strcpy(online_Client[j].name, name);
		online_Client[j].conn_Sock = conn_Sock;	
	} else {
		printf("khong kha dung!\n");
	}
}
//
void increase_Request_Id() {
	pthread_mutex_lock(&lock1);
	request_Id++;
	pthread_mutex_unlock(&lock1);
}
//
void create_Folder(char* p) {
	char path1[100] = "./SERVER/";
	strcat(path1, p);
	struct stat st1 = {0};
	if (stat(path1, &st1) == -1) {
	    mkdir(path1, 0700);
	}
	strcat(path1,"/tt");
		if (stat(path1, &st1) == -1) {
	    mkdir(path1, 0700);
	}
}
//
void handle_Directory(Mess recv_Mess, int conn_Sock) {
	char list_Folder[MAXLISTPATH];
	char list_File[MAXLISTPATH];
	char p1[100];
	memset(p1,'\0',sizeof(p1));
	memset(list_Folder,'\0',sizeof(list_Folder));
	memset(list_File,'\0',sizeof(list_File));
	int i = find_Client(recv_Mess.request_Id);
	strcat(p1,"./SERVER/");
	strcat(p1,online_Client[i].name);
	get_List_File("./SERVER/public",list_Folder);
	get_List_File(p1,list_File);
	Mess msg1,msg2;
	strcpy(msg1.data,list_Folder);
	msg1.request_Id = recv_Mess.request_Id;
	msg1.len = strlen(msg1.data);
	send_Message(online_Client[i].conn_Sock, msg1);
	msg2.type = REQUEST_DIRECTORY;
	strcpy(msg2.data,list_File);
	msg2.request_Id = recv_Mess.request_Id;
	msg2.len = strlen(msg2.data);
	send_Message(online_Client[i].conn_Sock, msg2);
}
//
void handle_Request_Download(Mess recv_Msg, int conn_Sock) { 
	Mess send_Msg;
	FILE* fp1;
	char full_Path[50];
	strcpy(full_Path,recv_Msg.data);
	int i = find_Client(recv_Msg.request_Id);
	if ((fp1 = fopen(full_Path, "rb+")) == NULL){
        printf("Error: File not found\n");
        send_Msg.type = ERROR;
		send_Msg.len = 0;
		send_Message(online_Client[i].conn_Sock,send_Msg);
		return;
    }
    else {
		send_Msg.type = OK;
		send_Msg.len = 0;
		send_Message(online_Client[i].conn_Sock,send_Msg);
		Mess msg1;
		receive_Message(online_Client[i].conn_Sock,&msg1);
		if(msg1.type!= CANCEL){
			long file_len;
			fseek(fp1, 0, SEEK_END);     
			file_len = ftell(fp1);                   
			rewind(fp1);  
			int sum_Byte = 0;
			while(!feof(fp1)) {
				int number_Byte_Send = DATASIZE;
				if((sum_Byte + DATASIZE) > file_len) {
					number_Byte_Send = file_len - sum_Byte; 
				}
				char* buff1 = (char *) malloc((number_Byte_Send) * sizeof(char));
				fread(buff1, number_Byte_Send, 1, fp1); 
				memcpy(send_Msg.data, buff1, number_Byte_Send);
				send_Msg.len = number_Byte_Send;
				sum_Byte += number_Byte_Send; 
				if(send_Message(online_Client[i].conn_Sock, send_Msg) <= 0) { 
					printf("Connection closed!\n");
					break;
				}
				free(buff1);
				if(sum_Byte >= file_len) {
					break;
				}
			}
			send_Msg.len = 0;
			send_Message(online_Client[i].conn_Sock, send_Msg);
		}
    }

}
//
void remove_File(char* file_Name) {
    if (remove(file_Name) != 0)
    {
        perror("Following error occurred\n");
    }
}
//
void handle_Upload_File(Mess recv_Msg, int conn_Sock) {
	FILE *fptr1;
	char full_Path[100];
	Mess send_Msg;
	strcpy(full_Path,recv_Msg.data);
	int i = find_Client(recv_Msg.request_Id);
	if(fopen(full_Path,"r")!=NULL ) { // check if file exist
		send_Msg.type = ERROR;
		strcpy(send_Msg.data,"Warning: File name already exists");
		send_Msg.len = strlen(send_Msg.data);
		send_Msg.request_Id = recv_Msg.request_Id;
		send_Message(online_Client[i].conn_Sock,send_Msg);
		return;
	}
	else {
		send_Msg.type = OK;
		send_Msg.len = 0;
		send_Msg.request_Id = recv_Msg.request_Id;
		send_Message(online_Client[i].conn_Sock,send_Msg);
		fptr1 = fopen(full_Path,"w+");
		while(1) {
			receive_Message(conn_Sock, &recv_Msg);
			if(recv_Msg.type == ERROR) {
				fclose(fptr1);
				remove_File(full_Path);
			}
			if(recv_Msg.len > 0) { 
				fwrite(recv_Msg.data, recv_Msg.len, 1, fptr1);
			} else {
				break;
			}
		}
		fclose(fptr1);
		strcpy(send_Msg.data,"Upload Successful!");
		send_Msg.len = strlen(send_Msg.data);
		send_Message(online_Client[i].conn_Sock,send_Msg);
	}
}
//
void handle_Login(Mess mes, int conn_Sock) {
	char** temp1 = str_spli(mes.data, '\n');
	Status_Code login_Code;
	if(number_Elements_In_Array(temp1) == 3) {
		char** user_Str = str_spli(temp1[1], ' ');
		char** pass_Str = str_spli(temp1[2], ' '); 
		if((number_Elements_In_Array(user_Str) == 2) && (number_Elements_In_Array(pass_Str) == 2)) { 
			if(!(strcmp(user_Str[0], COMMAND_USER) || strcmp(pass_Str[0], COMMAND_PASS))) { 
				char name[30];
				char pass[20];
				strcpy(name, user_Str[1]);
				strcpy(pass, pass_Str[1]);
				if(validate_User_name(name) || validate_Password(pass)) { 
					login_Code = login_acc(name, pass); 
					if(login_Code != LOGINSUCCESS)
						mes.type = ERROR;
					else{
						if(mes.request_Id == 0) {
							mes.request_Id = request_Id;
							increase_Request_Id();
							int i = find_Avaiable_Element_In_Array_Client();
							set_Client(i, mes.request_Id, name, conn_Sock); 
							printf("Login ID:%d, rqID: %d, conn_sock: %d\n",i,mes.request_Id,conn_Sock);
							create_Folder(name);
						}
					}
				} else {
					mes.type = ERROR;
					login_Code = USERNAMEORPASSWORDINVALID; 
				}
			}
			else{
				login_Code = COMMANDINVALID;
				mes.type= ERROR;
			}
		}
		else{
			login_Code = COMMANDINVALID;
			mes.type= ERROR;
		}
	}
	else {
		mes.type= ERROR;
		login_Code = COMMANDINVALID;
		printf("Fails on handle Login!!");
	}
	send_With_Code(mes, login_Code, conn_Sock);
}
//
void handle_Register(Mess mess1, int conn_Sock){
	char** temp1 = str_spli(mess1.data, '\n');
	Status_Code register_Code;
	if(number_Elements_In_Array(temp1) == 3) {
		char** user_Str = str_spli(temp1[1], ' ');
		char** pass_Str = str_spli(temp1[2], ' ');
		if((number_Elements_In_Array(user_Str) == 2) && (number_Elements_In_Array(pass_Str) == 2)) {
			if(!(strcmp(user_Str[0], COMMAND_USER) || strcmp(pass_Str[0], COMMAND_PASS))) {
				char username1[30];
				char password1[20];
				strcpy(username1, user_Str[1]);
				strcpy(password1, pass_Str[1]);
				if(validate_User_name(username1) || validate_Password(password1)) {
					register_Code = register_User(username1, password1);
					if(register_Code != REGISTERSUCCESS)
						mess1.type= ERROR;
					else {
						if(mess1.request_Id == 0) {
							mess1.request_Id = request_Id;
							increase_Request_Id();
							int i = find_Avaiable_Element_In_Array_Client();
							set_Client(i, mess1.request_Id, username1, conn_Sock);
							create_Folder(username1);
						}
					}
				} else {
					mess1.type =  ERROR;
					register_Code = USERNAMEORPASSWORDINVALID;
				}
			}
			else{
				register_Code = COMMANDINVALID;
				mess1.type= ERROR;
			}
		}
		else{
			register_Code = COMMANDINVALID;
			mess1.type= ERROR;
		}
	}
	else {
		mess1.type= ERROR;
		register_Code = COMMANDINVALID;
		printf("Fails on handle Register!!");
	}
	send_With_Code(mess1, register_Code, conn_Sock);
}
//
void handle_Logout(Mess mess1, int conn_Sock){
	char** temp1 = str_spli(mess1.data, '\n');
	Status_Code logout_Code;
	if(number_Elements_In_Array(temp1) != 2) {
		mess1.type = ERROR;
		logout_Code = COMMANDINVALID;
		printf("Fails on handle logout!!");
	}
	else{
		logout_Code = logout_User(temp1[1]);
		if(logout_Code == LOGOUTSUCCESS) {
			int i = find_Client(mess1.request_Id);
			if(i >= 0) {
				online_Client[i].request_Id = 0;
				online_Client[i].name[0] = '\0';
			}
		}
	}
	send_With_Code(mess1, logout_Code, conn_Sock);
}
//
void handle_Authenticate_Request(Mess mess1, int conn_Sock) {
	char* payload_Header;
	char temp1[DATASIZE];
	strcpy(temp1, mess1.data);
	payload_Header = get_Header_Of_Payload(temp1);
	if(!strcmp(payload_Header, LOGIN)) {
		handle_Login(mess1, conn_Sock);
	} else if (!strcmp(payload_Header, REGISTER)) {
		handle_Register(mess1, conn_Sock);

	} else if(!strcmp(payload_Header, LOGOUT)) {
		handle_Logout(mess1, conn_Sock);
	}
}
//
void* c_handler(void* conn_sock) {
	int conn_Sock;
	conn_Sock = *((int *) conn_sock);
	Mess recv_Mess;
	pthread_detach(pthread_self());
	while(1) {
		if(receive_Message(conn_Sock, &recv_Mess) < 0) {
			if(recv_Mess.request_Id > 0) {
				int i = find_Client(recv_Mess.request_Id);
				if(i >= 0) {
					online_Client[i].request_Id = 0;
					logout_User(online_Client[i].name);
				}
			}
			break;
		}
		switch(recv_Mess.type) {
			case AUTHENTICATE: 
				handle_Authenticate_Request(recv_Mess, conn_Sock);
				break;
			case REQUEST_DIRECTORY:
				handle_Directory(recv_Mess, conn_Sock);
				break;
			case REQUEST_DOWNLOAD: 
				handle_Request_Download(recv_Mess, conn_Sock);
				break;
			case UPLOAD_FILE: 
				handle_Upload_File(recv_Mess, conn_Sock);
				break;
			default: break;
		}

	}

	return NULL;
}
//
int main(int argc, char **argv)
{
 	int p_num;
 	int listen_sock, conn_sock;
	struct sockaddr_in server;
	struct sockaddr_in client; 
	int sin_size;
	pthread_t tid;
		
 	if(argc != 2) {
 		perror(" Error Parameter! Please input only port number\n ");
 		exit(0);
 	}
 	if((p_num = atoi(argv[1])) == 0) {
 		perror(" Please input port number\n");
 		exit(0);
 	}
 	if(!valid_Port_Number(p_num)) {
 		perror("Invalid Port Number!\n");
 		exit(0);
 	}
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
		perror("\nError: ");
		return 0;
	}
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;         
	server.sin_port = htons(p_num);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(listen_sock, (struct sockaddr*)&server, sizeof(server)) == -1){
		perror("\nError: ");
		return 0;
	}     
	if(listen(listen_sock, BACKLOG) == -1){
		perror("\nError: ");
		return 0;
	}
	init_Array_Client();
	read_File();
	while(1) {
		sin_size = sizeof(struct sockaddr_in);
		if ((conn_sock = accept(listen_sock,( struct sockaddr *)&client, (unsigned int*)&sin_size)) == -1) 
			perror("\nError: ");
		if (pthread_mutex_init(&lock1, NULL) != 0) 
	    { 
	        printf("\n mutex init has failed\n"); 
	        return 1; 
	    } 
		pthread_create(&tid, NULL, &c_handler, &conn_sock);	
	}
	
	close(listen_sock);
	return 0;
}
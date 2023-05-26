#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <libgen.h>
#include <time.h>
#include "lib.h"
#define TIME_DELAY 500
char cur_user[255];
char r[100];
int request_Id;
int c_soc;
struct sockaddr_in s_addr;
char chose;
Mess *mes;
int is_Online = 0;
struct Stack1 *stack1;
char *list_Current_Direc[30];
char** list_Folder;
char** list_File;
//
void delay(int num)
{
	int seconds = num;
	clock_t start_time = clock();
	while (clock() < start_time + seconds);
}
//
void to_Name_Of_File(char *fileName1, char* name1 ) {
    char** token1 = str_spli(fileName1, '/');
    int i1;
    for (i1 = 0; *(token1 + i1); i1++){}
    strcpy(name1, *(token1 + i1 -1));
}
//
void remove_File(char* file_Name) {
    if (remove(file_Name) != 0)
    {
        perror("Following error occurred\n");
    }
}
//
void get_Directory(){
	Mess send_Msg, recv_Msg1, recv_Msg2;
	send_Msg.type = REQUEST_DIRECTORY;
	send_Msg.request_Id = request_Id;
	send_Msg.len = 0;
	send_Message(c_soc,send_Msg);
	receive_Message(c_soc,&recv_Msg1);
	receive_Message(c_soc,&recv_Msg2);
	if(recv_Msg1.len>0) list_Folder = str_spli(recv_Msg1.data, '\n');
	if(recv_Msg2.len>0) list_File = str_spli(recv_Msg2.data, '\n');
	
}
//
void save_File(char *str, char* data) {
	FILE *fp;
	if( (fp=fopen(str, "a")) == NULL ) {
    printf("loi mo file %s\n", str);
    exit(0);
	}
	fprintf(fp,"%s", data);
	fclose(fp);
}
//
void printFile(char *file_name) {
	FILE *fp;
	char ch;
	if( (fp=fopen(file_name, "r")) == NULL ) {
    printf("loi mo file %s\n", file_name);
    exit(0);
	}
	   while (ch != EOF){
        ch = fgetc(fp);
		if((ch<='z'&&ch>='a') || (ch<='Z'&&ch>='A') || ch=='\n' || ch==':' || ch=='.' || (ch<='9'&&ch>='0') || ch==' ' || ch=='\t')
        printf("%c", ch);
    }
	fclose(fp);
}
//
char * getLocalTime() {
	time_t raw_time;
  	struct tm * timeinfo;

  	time (&raw_time);
  	timeinfo = localtime (&raw_time);
	 return asctime(timeinfo);
}
//
void upload_File() {
	char file_Name[30];
	char full_Path[100] = "./CLIENT/";
	char tt[1000], data[1000], s[100];
	printf("Chon thu muc muon upload:\n");
	printf("1. %-15s\n2. public\n",cur_user);

	char chose[10];
	int op;
	while(1){
		printf("\nBan chon (0 to exit): ");
		scanf(" %s", chose);
		while(getchar() != '\n');
		op = atoi(chose);
		if((op >= 0) && (op <= 2)) {
			break;
		} else {
			printf("vui long nhap lai!\n");
		}
	}
	if(op == 0) return;
	printf("Nhap ten file muon upload: ");
	char str[100];
	scanf("%[^\n]s",str);
	strcat(full_Path, str);
	Mess msg1, send_Msg, recv_Msg;
	FILE* fptr1;
	if ((fptr1 = fopen(full_Path, "rb+")) == NULL){
        printf("Khong tim thay file!\n");
    }
    else {
		to_Name_Of_File(full_Path,file_Name);
		if(op == 1){
			sprintf(msg1.data, "./SERVER/%s/%s", cur_user,file_Name);
			sprintf(tt, "./SERVER/%s/tt/%s1", cur_user,file_Name);			
		}else {
			sprintf(msg1.data, "./SERVER/public/%s", file_Name);
			sprintf(tt, "./SERVER/public/tt/%s1", file_Name);
		}
		strcpy(data, str);
		msg1.type = UPLOAD_FILE;
		msg1.len = strlen(msg1.data);
		msg1.request_Id = request_Id;
		send_Message(c_soc,msg1);
		receive_Message(c_soc,&recv_Msg);
		if(recv_Msg.type == ERROR){
			printf("%s\n",recv_Msg.data);
			fclose(fptr1);
		}else{
			printf("Uploading...\n");
			int file_len;
			fseek(fptr1, 0, SEEK_END);     
			file_len = ftell(fptr1);            
			rewind(fptr1);  
			sprintf(s,"\t%dkb\n", file_len/1024);
			strcat(data, s);
			save_File(tt, data);
			int sum1 = file_len/10240;
			int sum2 = 0, k = 1;
			int sum_Byte = 0;
			while(!feof(fptr1)) {
				int number_Byte_Send = DATASIZE;
				if((sum_Byte + DATASIZE) > file_len) {
					number_Byte_Send = file_len - sum_Byte; 
				}
				char* buffer1 = (char *) malloc((number_Byte_Send) * sizeof(char));
				fread(buffer1, number_Byte_Send, 1, fptr1);
				memcpy(send_Msg.data, buffer1, number_Byte_Send);
				send_Msg.len = number_Byte_Send;
				sum_Byte += number_Byte_Send; 
				if(send_Message(c_soc, send_Msg) <= 0) { 
					printf("Connection closed!\n");
					break;
				}
				free(buffer1);
				if(sum_Byte >= file_len) {
					break;
				}
				delay(TIME_DELAY);
				sum2++;
				if((sum2-k*sum1) == 0) {
					printf("%d0%%\n", k);
					k++;
				}
			}
			sprintf(data, "Upload by:%s\t%sDownload by:\n", cur_user, getLocalTime());
			save_File(tt, data);
			send_Msg.len = 0;
        	send_Message(c_soc, send_Msg);
			receive_Message(c_soc, &recv_Msg);
			printf("%s\n",recv_Msg.data);
		}
    }
}
//
void handle_Search_File(char *file_Name, char *list_Result){
	int i, k = 0;
	if(list_File[0] != NULL){
	for(i=0;*(list_File+i);i++){
		char *temp1 = strdup(list_File[i]);
		if(strcmp(basename(temp1),file_Name)==0){
			strcat(list_Result,list_File[i]);
			k = 1;
		}
		free(temp1);
	}
	}
	if(list_Folder[0]!= NULL){
	if(k == 0) {
		for(i=0;*(list_Folder+i);i++){
		char *temp1 = strdup(list_Folder[i]);
		if(strcmp(basename(temp1),file_Name)==0){
			strcat(list_Result,list_Folder[i]);
		}
		free(temp1);
	}
	}
	}
}
//
int handle_Select_Download_File(char *select_Link) {
	char file_Name[100];
	char list_Result[1000];
	memset(list_Result,'\0',sizeof(list_Result));
	printf("Nhap ten file ban muon download: ");
	scanf("%[^\n]s", file_Name);
	handle_Search_File(file_Name,list_Result);
	if(strlen(list_Result)<=0) return -1;
	printf("tim thay: %s\n", list_Result);
	char chose[10];
    int op;
    while(1) {
	    printf("\nBan muon tai file?\n1.tai ve\n2.huy\nban chon:  ");
	    scanf(" %s", chose);
		while(getchar() != '\n');
		op = atoi(chose);
		if((op >= 1) && (op <= 2)) {
			break;
		} else {
			printf("vui long nhap lai!\n");
		}
	}
	
	if(op == 2) {
		return -1;
	}
	else {
		strcpy(select_Link, list_Result);
	}
	return 1;
}
//
int down(char *l){
	Mess send_Msg, recv_Msg;
	FILE* fptr1;

	fptr1 = fopen(l, "rb+");
	int file_len;
	fseek(fptr1, 0, SEEK_END);          // Jump to the end of the file
	file_len = ftell(fptr1); 
	fclose(fptr1);
	int sum1 = file_len/10240;
	int sum2 = 0, k = 1;
	
	char save_Folder[20] = "./CLIENT";
	char save_Path[50];
	char temp1[50];
	strcpy(temp1,l);
	send_Msg.type = REQUEST_DOWNLOAD;
	send_Msg.request_Id = request_Id;
	strcpy(send_Msg.data,l);
	send_Msg.len= strlen(send_Msg.data);
	send_Message(c_soc,send_Msg);
	receive_Message(c_soc,&recv_Msg);
	if(recv_Msg.type != ERROR){
	  printf("new name: ");
	  char name[30];
	  scanf("%s", name);
	  sprintf(save_Path,"%s/%s",save_Folder, name);
	  if(fopen(save_Path, "r+")!=NULL){
	    printf("File da ton tai!\n");
	    send_Msg.type = CANCEL;
	    send_Message(c_soc,send_Msg);
	    return -1;
	  }
	  send_Msg.type = OK;
	  send_Message(c_soc,send_Msg);
	  printf("Dowloading...\n");
	  fptr1 = fopen(save_Path,"w+");
	  while(1) {
	    receive_Message(c_soc, &recv_Msg);
	    if(recv_Msg.type == ERROR) {
	      fclose(fptr1);
	      remove_File(save_Path);
	      return -1;
	    }
	    if(recv_Msg.len > 0) { 
	      fwrite(recv_Msg.data, recv_Msg.len, 1, fptr1);
	      delay(TIME_DELAY);
	      sum2++;
	      if((sum2-k*sum1) == 0) {
			printf("%d0%%\n", k);
			k++;
	      }
		  } else {
		break;
	      }
	    }
	    fclose(fptr1);
	    return 1;
	  }
	  return -1;
	}
//
void download_File() {
	char select_Link[50];
	char data[1000];
	if(handle_Select_Download_File(select_Link)==1){
	char str[1000], str1[1000];
	strcpy(str, select_Link);
	char * token1 = strtok(str, "/");
	strcpy(str1,token1);
	strcat(str1,"/");
	token1 = strtok(NULL, "/");
	strcat(str1, token1);
	strcat(str1, "/");
	token1 = strtok(NULL, "/");
	strcat(str1, token1);
	strcat(str1, "/tt/");	
	token1 = strtok(NULL, "/");
	strcat(str1, token1);
	strcat(str1, "1");
	//printf("%s\n", str1);
	sprintf(data, "user: %s\t%s", cur_user, getLocalTime());
	save_File(str1, data);
		if(down(select_Link) == -1) {
			printf("ERROR!\n");
			return;
		}
		printf("Download Successful!");
	}else {
		printf("khong tim thay file!\n");
		return;
	}
}
//
void get_Login_Info(char *str){
	char name[255];
	char pass[255];
	printf("name: ");
	scanf("%[^\n]s", name);
	while(getchar() != '\n');
	printf("password: ");
	scanf("%[^\n]s", pass);
	while(getchar()!='\n');
	sprintf(mes->data, "LOGIN\nUSER %s\nPASS %s", name, pass);
	strcpy(str, name);
}
//
void login_Func(char *cur_user){
	char name1[255];
	mes->type = AUTHENTICATE;
	get_Login_Info(name1);
	mes->len = strlen(mes->data);
	send_Message(c_soc, *mes);
	receive_Message(c_soc, mes);
	if(mes->type != ERROR){
		is_Online = 1;
		strcpy(cur_user, name1);
		strcpy(r, "./");
		strcat(r,name1);
		request_Id = mes->request_Id;
		get_Directory();
		stack1 = create_Stack(30);
		printf("Hello %s!\n", name1);
	} else {
		printf("Dang nhap khong thanh cong!\n");
	}
}
//
int get_Register_Info(char *user){
	char name[255], pass[255];
	printf("Username: ");
	scanf("%[^\n]s",name);
	printf("Password: ");
	while(getchar()!='\n');
	scanf("%[^\n]s", pass);
	
		sprintf(mes->data, "REGISTER\nUSER %s\nPASS %s", name, pass);
		strcpy(user, name);
		return 1;
}
//
void register_Func(char *cur_user){
	char username1[255];
	if(get_Register_Info(username1)){
		mes->type = AUTHENTICATE;
		mes->len = strlen(mes->data);
		send_Message(c_soc, *mes);
		receive_Message(c_soc, mes);
		if(mes->type != ERROR){
			is_Online = 1;
			strcpy(cur_user, username1);
			strcpy(r, "./");
			strcat(r,username1);
			request_Id = mes->request_Id;
			get_Directory();
			stack1 = create_Stack(30);
			printf("dang ki thanh cong!\n");
		} else {
			printf("Dang ki khong thanh cong\n");
		}
	}
}
//
void logout_Func(char *cur_user){
	mes->type = AUTHENTICATE;
	sprintf(mes->data, "LOGOUT\n%s", cur_user);
	mes->len = strlen(mes->data);
	send_Message(c_soc, *mes);
	receive_Message(c_soc, mes);
	if(mes->type != ERROR){
		is_Online = 0;
		cur_user[0] = '\0';
		request_Id =0;
		printf("bye!\n");
	}
}
//
void menu_Authenticate() {
	printf("\nMENU:");
	printf("\n[1].Dang nhap");
	printf("\n[2].Dang ki");
	printf("\n[3].thoat");
	printf("\nBan chon: ");
}
//
void main_Menu() {
	printf("\nMENU:");
	printf("\n[1].Danh sach file\n[2].Thong tin file\n[3].Upload file\n[4].download file\n[5].Dang xuat\n");
	printf("\nBan chon: ");
}
//
void show_All_File() {
	int i;
	printf("\n---\nYOUR FODER:\n");
	for(i=0;*(list_File+i);i++){
		char *temp = strdup(list_File[i]);
		char **arr = str_spli(temp, '/');
		if(arr[4] == NULL){
			printf("%s\n", arr[3]);
		}
		free(temp);
	}
	printf("\n---\nPUBLIC:\n");
		for(i=0;*(list_Folder+i);i++){
		char *temp = strdup(list_Folder[i]);
		char **arr = str_spli(temp, '/');
		if(arr[4] == NULL){
			printf("%s\n", arr[3]);
		}
		free(temp);
	}
}
//
int search_File(char *select_Link) {
	char file_Name[100];
	char list_Result[1000];
	memset(list_Result,'\0',sizeof(list_Result));
	printf("Nhap ten file: ");
	scanf("%[^\n]s", file_Name);
	handle_Search_File(file_Name,list_Result);
	if(strlen(list_Result)<=0) return -1;
	printf("tim thay: %s\n", list_Result);
	char str[1000], str1[1000];
	strcpy(str, list_Result);
	char * token = strtok(str, "/");
	strcpy(str1,token);
	strcat(str1,"/");
	token = strtok(NULL, "/");
	strcat(str1, token);
	strcat(str1, "/");
	token = strtok(NULL, "/");
	strcat(str1, token);
	strcat(str1, "/tt/");	
	token = strtok(NULL, "/");
	strcat(str1, token);
	strcat(str1, "1");
	printf("\n-----\nTHONG TIN:\n");
	printFile(str1);
	printf("-----\n");
		strcpy(select_Link, list_Result);
	return 1;
}
//
void thongTin() {
	char select_Link[50];
	if(search_File(select_Link)==1){
	}else {
		printf("khong tim thay file!\n");
		return;
	}

}
//////////
int main(int argc, char const *argv[])
{
	if(argc!=3){
		printf("Error!\nInput error!\n");
		exit(0);
	}
	char *ser_Addr = malloc(sizeof(argv[1]) * strlen(argv[1]));
	strcpy(ser_Addr, argv[1]);
	int p = atoi(argv[2]);
	mes = (Mess*) malloc (sizeof(Mess));
	mes->request_Id = 0;
 	if(!valid_Port_Number(p)) {
 		perror("Invalid Port Number!\n");
 		exit(0);
 	}
	if(!check_IP(ser_Addr)) {
		printf("Invalid Ip Address!\n");
		exit(0);
	}
	strcpy(ser_Addr, argv[1]);
	if(!has_IP_Address(ser_Addr)) {
		printf("Not found information Of IP Address [%s]\n", ser_Addr);
		exit(0);
	}
	c_soc = socket(AF_INET, SOCK_STREAM, 0);
	if (c_soc == -1 ){
		perror("\nError: ");
		exit(0);
	}
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(p);
	s_addr.sin_addr.s_addr = inet_addr(ser_Addr);

	if(connect(c_soc, (struct sockaddr*) (&s_addr), sizeof(struct sockaddr)) < 0){
		printf("\nError!khong the ket noi server!\n");
		exit(0);
	
	}		
	while(1) {
		if(!is_Online) {
				menu_Authenticate();
	scanf(" %c", &chose);
	while(getchar() != '\n');
	switch (chose){
		case '1':
			login_Func(cur_user);
			break;
		case '2':
			register_Func(cur_user);
			break;
		case '3': 
			exit(0);
		default:
			printf("Vui long nhap lai!\n");
	}
		} else {
	main_Menu();
	scanf(" %c", &chose);
	while(getchar() != '\n');
	switch (chose) {
		case '1':
			show_All_File();
			break;
		case '2':
			thongTin();
			break;
		case '3':
			upload_File();
			get_Directory();
			break;
		case '4':
			download_File();
			break;
		case '5':
			logout_Func(cur_user);
			break;
		default:
			printf("Vui long nhap lai!\n");
	}
		}
	}

	close(c_soc);
	return 0;
}

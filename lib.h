#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include "status.h"
#define BLOCK 0
#define ACTIVE 1
#define MAX_LOGIN_FAILS 3
#define ACCOUNT_FILE "acc.txt"
#define OFFLINE 0
#define ONLINE 1
#define BUFFSIZE 2000
#define DATASIZE 1024
#define COMMAND_USER "USER"
#define COMMAND_PASS "PASS"

char **tokens1;

struct Stack1 { 
	int top1; 
	unsigned capacity1; 
	char **array1; 
};

typedef struct user1 {
	char name[32];
	char pass[32];
	int s;
	int is_login;
	int count_fails;
	struct user1 *next;
} User1;
extern User1 *h;
extern User1 *c;
User1 *h = NULL;
User1 *c = NULL;

typedef enum {
	AUTHENTICATE,
	REQUEST_FILE,
	REQUEST_DOWNLOAD,
	REQUEST_DIRECTORY,
	UPLOAD_FILE,
	ERROR,
	OK,
	CANCEL,
} Mess_Type;

typedef struct Mess{
	Mess_Type type;
	int request_Id;
	int len;
	char data[DATASIZE];
} Mess;

typedef struct Client {
	int request_Id;
	char name[30];
	int conn_Sock;
	int upload_Success;
} Client;

//
void get_List_File(char *basePath1, char *listfile1)
{
    char path1[1000];
    memset(path1,'\0',1000);
    struct dirent *dp1;
    DIR *dir1 = opendir(basePath1);

    if (!dir1) return;
    printf("%s\n",path1);
    while ((dp1 = readdir(dir1)) != NULL)
    {
        if (strcmp(dp1->d_name, ".") != 0 && strcmp(dp1->d_name, "..") != 0)
        {
            strcpy(path1, basePath1);
            strcat(path1, "/");
            strcat(path1, dp1->d_name);
            if(dp1->d_type == DT_REG) {
                strcat(listfile1, path1);
                strcat(listfile1, "\n");
            }
            get_List_File(path1,listfile1);
        }
    }
    closedir(dir1);
}
//
struct Stack1* create_Stack(unsigned capacity1) { 
	struct Stack1* stack1 = (struct Stack1*)malloc(sizeof(struct Stack1)); 
	stack1->capacity1 = capacity1; 
	stack1->top1 = -1; 
	stack1->array1 = (char**)malloc(stack1->capacity1 * sizeof(char*));
	return stack1; 
} 

//
int is_Has_Blank_Space(char* str1) {
	int i1 = 0;
	for(i1 = 0; i1 < strlen(str1); i1++) {
		if(str1[i1] == ' ') {
			return 1;
		}
	}
	return 0;
}
//
int in_Valid_Range(int num1, int min1, int max1) {
	return (num1 >= min1) && (num1 <= max1);
}
//
int validate_User_name(char* username1) {
	int len1 = strlen(username1);
	if(in_Valid_Range(len1, 6, 30) && !is_Has_Blank_Space(username1)) {
		return 1;
	}
	return 0;
}
//
int validate_Password(char* pass1) {
	int len1 = strlen(pass1);
	if(in_Valid_Range(len1, 3, 20) && !is_Has_Blank_Space(pass1)) {
		return 1;
	}
	return 0;
}
//
char** __str_split1(char* a_str1, const char a_delim1)
{
    char** result1    = 0;
    size_t count1     = 0;
    char* tmp1       = a_str1;
    char* last_comma1 = 0;
    char delim1[2];
    delim1[0] = a_delim1;
    delim1[1] = 0;

    while (*tmp1)
    {
        if (a_delim1 == *tmp1)
        {
            count1++;
            last_comma1 = tmp1;
        }
        tmp1++;
    }

    count1 += last_comma1 < (a_str1 + strlen(a_str1) - 1);

    count1++;

    result1 = malloc(sizeof(char*) * count1);

    if (result1)
    {
        size_t idx1  = 0;
        char* token1 = strtok(a_str1, delim1);

        while (token1)
        {
            assert(idx1 < count1);
            *(result1 + idx1++) = strdup(token1);
            token1 = strtok(0, delim1);
        }
        assert(idx1 == count1 - 1);
        *(result1 + idx1) = 0;
    }

    return result1;
}
//
int valid_Number(char *value1)
{
    if(!strcmp(value1, "0")) {
        return 1;
    }
    return (atoi(value1) > 0) && (atoi(value1) <= 255);
}
//
int check_Dots(char *str1)
{
    tokens1 = __str_split1(str1, '.');
    if (tokens1)
    {
        int i;
        for (i = 0; *(tokens1 + i); i++)
        {
            // count number elements in array
        }
        if((i-1) == 3) {
            return 1;
        }
    }
    return 0;
}
//
int check_IP(char *str1)
{
    if(check_Dots(str1)) {
        if (tokens1) {
            int i1;
            for (i1 = 0; *(tokens1 + i1); i1++)
            {
                if(!valid_Number(*(tokens1 + i1))) {
                    return 0;
                }
                free(*(tokens1 + i1));
            }
            free(tokens1);
            return 1;
        }
    }
    return 0;
}
//
int valid_Port_Number(int p) {
	return (p > 0) && (p <= 65535);
}
//
int has_IP_Address(char *ip1) {
    struct in_addr ipv4;
    inet_pton(AF_INET, ip1, &ipv4);
    struct hostent *host1 = gethostbyaddr(&ipv4, sizeof(ipv4), AF_INET);
    if (host1 != NULL)
    {
        return 1;
    }
    return 0;
}

//
User1 *create_New_User(char* name, char* pass, int s) {
	User1 *new_user = (User1*)malloc(sizeof(User1));
	strcpy(new_user->name, name);
	strcpy(new_user->pass, pass);
	new_user->s = s;
	new_user->is_login = OFFLINE;
	new_user->count_fails = 0;
	new_user->next = NULL;
	return new_user;
}
//
void exit_Program() {
	printf("File %s khong ton tai!\n", ACCOUNT_FILE);
	exit(0);
}
//
void update_File() {
	FILE* f1;
	f1 = fopen(ACCOUNT_FILE, "w");
	if(!f1) {
		printf("error!\n");
		exit_Program();
	}

	User1 *p1 = h;

    while (p1 != NULL) {
        fprintf(f1, "%s %s %d\n", p1->name, p1->pass, p1->s);    	
    	p1 = p1->next;
    }

    fclose(f1);
    return;
}
//
void appen(User1* new_user) {
	if(h == NULL) {
		h = new_user;
		c = new_user;
	}
 	else {
 		c->next = (User1*)malloc(sizeof(User1));
		c->next = new_user;
		c = new_user;
 	}
}

//
User1* search_User(char *name) {
	User1 *p1 = h;
    while (p1 != NULL) {
    	if(!strcmp(p1->name, name)) break;
        p1 = p1->next;
    }
    
	return p1;
}
//
int identify_PassWord(User1* user1, char* pass){
	return strcmp(user1->pass, pass);
}

int login_acc(char* name, char* pass){
	User1 *u = search_User(name);
	if(u == NULL) return USERNOTFOUND;
	else{
		if(u->is_login == ONLINE) return USERISONLINE;
		else if(u->s == ACTIVE){
			if(!identify_PassWord(u, pass)){
				u->is_login = ONLINE;
				u->count_fails = 0;
				return LOGINSUCCESS;
			}
			else{
				u->count_fails ++;
				if(u->count_fails == MAX_LOGIN_FAILS){
					u->s = BLOCK;
					update_File();
					return BLOCKEDUSER;
				}
				return PASSWORDINVALID;
			}
		}
		else 
			return USERISBLOCKED;
	}
}
//
void read_File() {
	char name[254];
	char pass[32];
	int s;
	char c;
	FILE* f;
	f = fopen(ACCOUNT_FILE, "r");
	if(!f) {
		printf("error!\n");
		exit_Program();
	}

	while(!feof(f)) {
		if(fscanf(f, "%s %s %d%c", name, pass, &s, &c) != EOF) {
			appen(create_New_User(name, pass, s));
		}
		if(feof(f)) break;
	}
	fclose(f);
	return;
}

//
int register_User(char* name, char* pass){
	User1 *u = search_User(name);
	if(u == NULL){
		u = create_New_User(name, pass, ACTIVE);
		u->is_login = ONLINE;
		appen(u);
		update_File();
		return REGISTERSUCCESS;
	}
	return ACCOUNTISEXIST;
}
//
int logout_User(char *name){
	User1 *u = search_User(name);
	if(u->is_login == ONLINE) {
		u->is_login = OFFLINE;
		return LOGOUTSUCCESS;
	} else {
		return COMMANDINVALID;
	}
	return SERVERERROR;
}

//
int copy_Mess(Mess* mess1, Mess temp1) {
  mess1->type = temp1.type;
  mess1->request_Id = temp1.request_Id;
  mess1->len = temp1.len;
  memcpy(mess1->data, temp1.data, temp1.len+1);
  return 1;
}

//
int send_Msg(int soc, Mess msg1){
  int dataLength1, nLeft1, idx1;
  nLeft1 = sizeof(Mess);
  idx1 = 0;
  while (nLeft1 > 0){
    dataLength1 = send(soc, &((char*)&msg1)[idx1], nLeft1, 0);
    if (dataLength1 < 0) return dataLength1;
    nLeft1 -= dataLength1;
    idx1 += dataLength1;
  }

  return sizeof(Mess);
}
//
int recvMsg(int soc, Mess *msg1){
  char recv_Buff[BUFFSIZE];
  int ret1, nLeft1, idx1, bytes_recv1;
  Mess recv_Message;
  ret1 = 0;
  idx1 = 0;
  nLeft1 = sizeof(Mess);
  while (nLeft1 > 0) {
    bytes_recv1 = nLeft1 > BUFFSIZE ? BUFFSIZE : nLeft1;
    ret1 = recv(soc, recv_Buff, bytes_recv1, 0);
    if (ret1 <= 0) return ret1;
    memcpy(&(((char*)&recv_Message)[idx1]), recv_Buff, ret1); 
    idx1 += ret1;
    nLeft1 -= ret1;
    
  }
  copy_Mess(&(*msg1), recv_Message);
  return sizeof(Mess);
}
//
int send_Message(int conn_soc, Mess msg1) {
  int bytes_sent1 = send_Msg(conn_soc, msg1);
  if(bytes_sent1 <= 0){
    printf("\nConnection closed!\n");
    close(conn_soc);
    return -1;
  }
  return 1;
}
//
int receive_Message(int conn_soc, Mess *msg1) {
  int bytes_rec;
  bytes_rec = recvMsg(conn_soc, msg1);
  if (bytes_rec <= 0){
    printf("\nConnection closed\n");
    return -1;
  }
  return 1;
}
//
char** str_spli(char* a_s, const char a_d)
{
    char** result1    = 0;
    size_t count1    = 0;
    char* tmp1        = a_s;
    char* last_comma1 = 0;
    char delim1[2];
    delim1[0] = a_d;
    delim1[1] = 0;
    while (*tmp1)
    {
        if (a_d == *tmp1)
        {
            count1++;
            last_comma1 = tmp1;
        }
        tmp1++;
    }
    count1 += last_comma1 < (a_s + strlen(a_s) - 1);
    count1++;

    result1 = malloc(sizeof(char*) * count1);

    if (result1)
    {
        size_t idx1  = 0;
        char* token1 = strtok(a_s, delim1);

        while (token1)
        {
            assert(idx1 < count1);
            *(result1 + idx1++) = strdup(token1);
            token1 = strtok(0, delim1);
        }
        if((count1 - 1)) {
          assert(idx1 == count1 - 1);
        }
        *(result1 + idx1) = 0;
    }

    return result1;
}
//
char* get_Header_Of_Payload(char* data) {
  if(strlen(data)) 
    return str_spli(data, '\n')[0];
  return NULL;
}
//
void send_With_Code(Mess m,Status_Code c, int s) {
    char msg_Code[200];
    Mess *new_Mess = (Mess*)malloc(sizeof(Mess));
    new_Mess->type = m.type;
    new_Mess->request_Id = m.request_Id;
    message_Code(c, msg_Code);
    sprintf(new_Mess->data,"%d : %s", c, msg_Code);
    new_Mess->len = strlen(new_Mess->data);
    send_Message(s, *new_Mess);
}


#include <string.h>
#include "status.h"

void message_Code(Status_Code code1, char *msg1) {
	switch (code1) {
        case USERNOTFOUND: strcpy(msg1, "User Not Found!!");
        break;
        case USERISBLOCKED: strcpy(msg1, "User is Blocked!!");
        break;
        case BLOCKEDUSER: strcpy(msg1, "User has been Blocked!!");
        break;
        case PASSWORDINVALID: strcpy(msg1, "Your input password invalid!!");
        break;
        case LOGINSUCCESS: strcpy(msg1, "Login Successfully!!");
        break;
        case USERISONLINE: strcpy(msg1, "The User Was In Online!!");
        break;
        case ACCOUNTISEXIST: strcpy(msg1, "The Account was existed!!");
        break;
        case REGISTERSUCCESS: strcpy(msg1, "Register Successfully!!");
        break;
        case LOGOUTSUCCESS: strcpy(msg1, "Logout SuccessFully!!");
        break;
        case FILENOTFOUND: strcpy(msg1, "User Not Found!!");
        break;
        case COMMANDINVALID: strcpy(msg1, "Command was invalid!!");
        break;
        case USERNAMEORPASSWORDINVALID: strcpy(msg1, "Username or Password invalid!!");
        break;
        case SERVERERROR: strcpy(msg1, "Something error!!");
        break;
        default: break;
      }
}
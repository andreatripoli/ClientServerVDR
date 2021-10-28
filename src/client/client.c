#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include <termios.h>  
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include "constant.h"
#include <binn.h>
#include "utils.h"

// Variables
FILE *file;
char line[RSIZ][LSIZ];
int i = 0;
int tot = 0;
char name[100];
char password[30];
char last_state[50];
char last_name_file[42];
char nameFile[17];
char namePath[30];
bool isRecord = false;
char *resultAuthentication;

// Socket
int client_sock_desc = 0;
bool isOnline = true;
bool serverIsOnline = true;
struct sockaddr_in client;
char *ipAddress = NULL;
char *portAddress =  NULL;

// Thread and mutex
pthread_t tid, tid2;
pthread_mutex_t lock;

// Functions
int isValidIpAddress(char *ipAddress);
int isNumber(char s[]);
void *message_sender();
void *message_receiver();
void *pingsF();
static char *rand_string(char *str, size_t size);
int duration (struct timeval *start,struct timeval *stop, struct timeval *delta);
void sendFile();
char* convertToSHA1(char* password);

// Signal Handler for SIGINT 
void sigintHandler(int sig_num)
{
    signal(SIGINT, sigintHandler);
    fflush(stdout);
    puts("\033[37m[INFO] Closing in progress...");
    sleep(2);
    exit(1);
}

// Signal Handler for SIGTSTP
void sighandler(int sig_num)
{
    signal(SIGTSTP, sighandler);
    fflush(stdout);
    puts("\033[37m[INFO] Closing in progress...");
    sleep(2);
    exit(1);
}

// Thread to record
void *myThread(void *vargp)
{

    strncpy(namePath, "audio/", sizeof("audio/"));
    strncat(namePath, rand_string(nameFile, 17), 18);

    char str[]="./arecord %s";
    char str1[40];
    sprintf(str1, str, namePath);
    strncpy(last_name_file, nameFile, sizeof(nameFile));
    system(str1);
    return NULL;
}


// Calculate the duration of a record
int duration (struct timeval *start,struct timeval *stop,struct timeval *delta)
{
    suseconds_t microstart, microstop, microdelta;
    
    microstart = (suseconds_t) (100000*(start->tv_sec))+ start->tv_usec;
    microstop = (suseconds_t) (100000*(stop->tv_sec))+ stop->tv_usec;
    microdelta = microstop - microstart;
    
    delta->tv_usec = microdelta%100000;
    delta->tv_sec = (time_t)(microdelta/100000);
    
    if((*delta).tv_sec < 0 || (*delta).tv_usec < 0)
        return -1;
    else
        return 0;
}

// Send a file
void sendFile() {
    struct timeval start, stop, delta;
    char buf[BUFFERT];
    off_t count=0, m,sz;
	long int n;
    int sfd,fd;
    int l=sizeof(struct sockaddr_in);
    struct stat buffer;
    

    // Unable to open file
    if ((fd = open(namePath,O_RDONLY))==-1){
        puts("\033[31m[INFO] Open fail...");
		return;
	}

    // Unable to parse the file
	if (stat(namePath,&buffer)==-1){
		puts("\033[31m[INFO] Stat fail...");
		return;
	}
	else
		sz=buffer.st_size;


    bzero(&buf,BUFFERT);

    gettimeofday(&start,NULL);
    n=read(fd,buf,BUFFERT);

    // Send the file to the server 
	while(n){
		if(n==-1){
			puts("\033[31m[INFO] Error. Try...");
			break;
		}

		m=sendto(client_sock_desc,buf,n,0,(struct sockaddr*)&client,l);
		if(m==-1){
			puts("\033[31m[INFO] Send fail...");
			break;
		}
		count+=m;
		//fprintf(stdout,"----\n%s\n----\n",buf);
		bzero(buf,BUFFERT);
        n=read(fd,buf,BUFFERT);

	}

    m=sendto(client_sock_desc,buf,0,0,(struct sockaddr *)&client,l); 
}


int main(int argc, char **argv) {

    // Initialize the signals 
    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, sighandler);

    // Initialize the mutex 
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        puts("\033[31m[INFO] Mutex init failed...");
        return 1;
    }

    puts("\033[37m[INFO] Client...");
    puts("");
    puts("\033[37m[INFO] Reading the configuration file...");

    // Extract the configuration data from the file 
    if((file = fopen("property.conf","r"))!=NULL){

            // File exists
            while(fgets(line[i], LSIZ, file)){
                line[i][strlen(line[i]) - 1] = '\0';
                i++;
            }

            tot = i;

            for(i = 0; i < tot; ++i){
                char *token = strtok(line[i], "=");
                while (token != NULL){   
                    if (((strcmp(token, "PORT")) != 0) && ((strcmp(token, "IP")) != 0) && ((strcmp(token, "VDR")) != 0)) {

                        if (isValidIpAddress(token) == 1) {
                            if (ipAddress == NULL) {
                                ipAddress = token;
                            }
                        }

                        if (isNumber(token) == 1) {
                            portAddress = token;
                        }       
                        token = strtok(NULL, "=");
                    }

                    else{
                        token = strtok(NULL, "=");
                    }
                }
            }
            fclose(file);
    } 
    // File not found
    else{
        puts("\033[31m[INFO] Configuration file not found...");
        return 1;
    }

    // Incorrect data 
    if ((ipAddress == NULL) || (portAddress == NULL)) {
        puts("\033[31m[INFO] Set the IP Address and port in the property file...");
        return 1;
    }

    // Print configuration file
    puts("");
    puts("\033[37mConfiguration");

    char confAddress[50] = "\033[36mIP Server: ";
    char confPort[20] = "\033[36mPort: ";

    strncat(confAddress, ipAddress, sizeof(ipAddress));
    strncat(confPort, portAddress, sizeof(portAddress));

    puts(confAddress);
    puts(confPort);

    puts("");

    client_sock_desc = socket(AF_INET, SOCK_STREAM, 0);

    // Unable to create socket
    if (client_sock_desc == -1) {
        puts("\033[31m[INFO] Could not create socket...");
        return 1;
    }

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr(ipAddress);
    client.sin_port = htons(atoi(portAddress)); 

    // If the process is not busy, try again 
    do {
        puts("\033[31m[INFO] Busy process. New attempt in 5 seconds...");
        sleep(5);
    } while (connect(client_sock_desc, (struct sockaddr *)&client, sizeof(client))<0);
    

    puts("\033[37m[INFO] Process started...\n");

    send(client_sock_desc,"",strlen("")+1,0);
    
    // Threads
    pthread_t send,receive, pings;
    pthread_create(&pings,NULL,pingsF,NULL);
    pthread_create(&send,NULL,message_sender,NULL);
    pthread_create(&receive,NULL,message_receiver,NULL);
    pthread_join(pings,NULL);
    pthread_join(send,NULL);
    pthread_join(receive,NULL);

    close(client_sock_desc);
}

// Ping the server to check if it shuts down 
void *pingsF(){
    int nread;
    binn *obj  = binn_object();
    while(true){
        if (!isOnline){
            break;
        }

        // When the server is not responding, the client cannot be any operation 
        if (serverIsOnline) {
            binn_object_set_str(obj, "request", "-ping"); 
            nread = send(client_sock_desc,binn_ptr(obj), binn_size(obj),0);
            if(nread<0){
                puts("\033[31m[INFO] Server is down...");
                serverIsOnline = false;
            } 
        }
        else {
            close(client_sock_desc);

            client_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
            if (client_sock_desc == -1) {

            }
            else{
                client.sin_family = AF_INET;
                client.sin_addr.s_addr = inet_addr(ipAddress);
                client.sin_port = htons(atoi(portAddress)); 

                // The server comes back online
                if (connect(client_sock_desc, (struct sockaddr *)&client, sizeof(client))<0){
                    continue;
                }
                serverIsOnline = true;
                puts("\033[32m[INFO] Server is up...");
            }
            
        }  
        sleep(5);
    }
}

// The client can type on the terminal 
void *message_sender(){
    while(true){
        char input[500];     
        char inputCopy[500];  
        char* result = NULL;   

        // If the server is online
        if (!serverIsOnline){
            continue;
        }
        else
        {
            result = fgets(input,480, stdin);
            
            if (input[strlen(input) - 1] != '\n') {
                puts("\033[31m[INFO] Too many characters...\033[37m");
                continue;
            }

            pthread_mutex_lock(&lock);
                
            strncpy(inputCopy,input, sizeof(input));

            // Function to exit the program
            if(strcmp(input,"-exit\n")==0){  

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                binn_object_set_str(obj, "request", "-exit");        

                //Send server to exit command
                if(send(client_sock_desc,binn_ptr(obj), binn_size(obj),0)<0){
                    puts("\033[31m[INFO] Send failed...\n");
                }
                isOnline = false;
                sleep(2);
                puts("\033[33m[COMMAND] Closing in progress...");
                sleep(2);
                binn_free(obj);
                fflush(stdout);
                exit(1);                        
            }

            // Function to view the commands
            else if(strcmp(input,"-help\n")==0){

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                binn_object_set_str(obj, "request", "-help");
                send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                binn_free(obj);
            }

            // Function to view the info user
            else if(strcmp(input,"-whoami\n")==0){

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                binn_object_set_str(obj, "request", "-whoami");
                send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                binn_free(obj);
            }

            // Login function
            else if(strcmp(input,"-login\n")==0){

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                
                char *p;
                char passwd[30] = {0};
                char name[30] = {0};  
                char *result;
                char *encrypt;
                int size = 61; // One extra for ‘\0’
                result = (char *)malloc(sizeof(char)*size);

                puts("\033[37mAuthentication"); 

                while (1) 
                {
                    if (name[0] == 0) 
                    {
                        printf("\033[36mEnter your username: \033[37m");  
                        fgets(name,28, stdin);

                        if (name[strlen(name) - 1] != '\n') {
                            strncpy(name, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        }
                        else {
                            int size = strlen(name); //Total size of string
                            name[size-1] = '\0';
                        }

                        if ((strcmp(name, " ") == 0) || (strcmp(name,"-exit")== 0) || (strcmp(name, "\n") == 0) || (strcmp(name, "") == 0)) {
                            break;
                        }
                    }

                    if (passwd[0] == 0) 
                    {
                            printf("\033[36mEnter your password: ");  

                            // Hidden
                            set_disp_mode(STDIN_FILENO,0);  

                            fgets(passwd,28, stdin);

                            if (passwd[strlen(passwd) - 1] != '\n') {
                                set_disp_mode(STDIN_FILENO,1);  
                                strncpy(passwd, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }
                            else {
                                int size = strlen(passwd); //Total size of string
                                passwd[size-1] = '\0';
                            }
                            set_disp_mode(STDIN_FILENO,1);  
                                                        
                            if ((strcmp(passwd, " ") == 0) || (strcmp(passwd,"-exit")== 0) || (strcmp(passwd, "\n") == 0) || (strcmp(passwd, "") == 0)) {
                                break;
                            }
                            else
                            {
                                break;
                            }
                    }

                }

                if ((name[0] == 0) && (passwd[0] == 0))  
                {
                    printf("\033[31m[INFO] Data wrong...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                // SHA function(in clear -> coded )
                char *sha1 = convertToSHA1(passwd); 
                binn_object_set_str(obj, "request", "-login");
                binn_object_set_str(obj, "name", name);
                binn_object_set_str(obj, "password", sha1);
                send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                binn_free(obj);
                pthread_mutex_unlock(&lock);
                puts("\033[0m");
            }

            // Function to register 
            else if(strcmp(input,"-register\n")==0){

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                
                char *p;
                char passwd[30] = {0};
                char name[30] = {0};  
                char surname[30] = {0};  
                char username[30] = {0};  
                char *result;
                char *encrypt;
                int size = 61; // One extra for ‘\0’
                result = (char *)malloc(sizeof(char)*size);

                puts("\033[37mRegistration"); 

                while (1) 
                {
                    if (name[0] == 0) 
                    {
                        printf("\033[36mEnter your name: \033[37m");  

                        fgets(name,28, stdin);

                        if (name[strlen(name) - 1] != '\n') {
                            strncpy(name, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        } 
                        else {
                                int size = strlen(name); //Total size of string
                                name[size-1] = '\0';
                            }


                        if ((strcmp(name, " ") == 0) || (strcmp(name,"-exit")== 0) || (strcmp(name, "\n") == 0) || (strcmp(name, "") == 0)) {
                            break;
                        }
                    }


                    if (surname[0] == 0) 
                    {
                        printf("\033[36mEnter your surname: \033[37m");  

                        fgets(surname,28, stdin);

                        if (surname[strlen(surname) - 1] != '\n') {
                            strncpy(surname, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        }
                        else {
                                int size = strlen(surname); //Total size of string
                                surname[size-1] = '\0';
                            } 


                        if ((strcmp(surname, " ") == 0) || (strcmp(surname,"-exit")== 0) || (strcmp(surname, "\n") == 0) || (strcmp(surname, "") == 0)) {
                            break;
                        }
                    }


                    if (username[0] == 0) 
                    {
                        printf("\033[36mEnter your username: \033[37m");  
                        fgets(username,28, stdin);

                        if (username[strlen(username) - 1] != '\n') {
                            strncpy(username, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        }
                        else {
                                int size = strlen(username); //Total size of string
                                username[size-1] = '\0';
                            } 

                        if ((strcmp(username, " ") == 0) || (strcmp(username,"-exit")== 0) || (strcmp(username, "\n") == 0) || (strcmp(username, "") == 0)) {
                            break;
                        }
                    }

                    if (passwd[0] == 0) 
                    {
                            printf("\033[36mEnter your password: ");  

                            set_disp_mode(STDIN_FILENO,0);  
                            fgets(passwd,28, stdin);

                            if (passwd[strlen(passwd) - 1] != '\n') {
                                set_disp_mode(STDIN_FILENO,1);  
                                strncpy(passwd, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }
                            else {
                                int size = strlen(passwd); //Total size of string
                                passwd[size-1] = '\0';
                            } 
                            set_disp_mode(STDIN_FILENO,1);  
                                                        
                            if ((strcmp(passwd, " ") == 0) || (strcmp(passwd,"-exit")== 0) || (strcmp(passwd, "\n") == 0) || (strcmp(passwd, "") == 0)) {
                                break;
                            }
                            else
                            {
                                break;
                            }
                    }

                }

                if ((name[0] == 0) && (surname[0] == 0) && (username[0] == 0)&& (passwd[0] == 0) )  
                {
                    printf("\033[31m[INFO] Data wrong...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                char *sha1 = convertToSHA1(passwd); 
                binn_object_set_str(obj, "request", "-register");
                binn_object_set_str(obj, "name", name);
                binn_object_set_str(obj, "surname", surname);
                binn_object_set_str(obj, "username", username);
                binn_object_set_str(obj, "password", sha1);
                send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                binn_free(obj);

                puts("\033[0m");

                memset(&passwd[0], 0, sizeof(passwd));
                memset(&name[0], 0, sizeof(name));
                memset(&surname[0], 0, sizeof(surname));
                memset(&username[0], 0, sizeof(username));
            }


            // Function to manage your own address book 
            else if(strcmp(input,"-book\n")==0){

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                
                char *p;
                char subRequest[30] = {0};
                char person[30] = {0};  

                puts("\033[37mAddress Book"); 

                while (1) 
                {
                    if (subRequest[0] == 0) 
                    {
                        printf("\033[36mChoose an operation\n");  
                        printf("\033[34madd (To add a person to your address book)\n");  
                        printf("\033[34mdelete (To delete a person from your address book)\n");  
                        printf("\033[34mshow (To show your address book) \033[37m\n"); 

                        fgets(subRequest,28, stdin);

                        if (subRequest[strlen(subRequest) - 1] != '\n') {
                            strncpy(subRequest, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        }  

                        if ((strcmp(subRequest, " ") == 0) || (strcmp(subRequest,"-exit")== 0) || (strcmp(subRequest, "\n") == 0) || (strcmp(subRequest, "") == 0)) {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }

                if ((subRequest[0] == 0))  
                {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                // Add a person to your address book 
                if (strcmp(subRequest, "add\n") == 0) {

                    while (1) 
                    {
                        if (person[0] == 0) 
                        {
                            printf("\033[36mChoose the username of the person to add: \033[37m");

                            fgets(person,28, stdin);

                            if (person[strlen(person) - 1] != '\n') {
                                strncpy(person, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person); //Total size of string
                                person[size-1] = '\0';
                            }


                            if ((strcmp(person, " ") == 0) || (strcmp(person,"-exit")== 0) || (strcmp(person, "\n") == 0) || (strcmp(person, "") == 0)) {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }

                    if ((person[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not chosen a person...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    binn_object_set_str(obj, "request", "-book");
                    binn_object_set_str(obj, "subrequest", "-add");
                    binn_object_set_str(obj, "person", person);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                    binn_free(obj);

                    puts("\033[0m");


                }

                // Delete a person from your address book 
                else if  (strcmp(subRequest, "delete\n") == 0) {

                    while (1) 
                    {
                        if (person[0] == 0) 
                        {
                            printf("\033[36mChoose the username of the person to add: \033[37m");   
                            fgets(person,28, stdin);

                            if (person[strlen(person) - 1] != '\n') {
                                strncpy(person, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person); //Total size of string
                                person[size-1] = '\0';
                            }

                            if ((strcmp(person, " ") == 0) || (strcmp(person,"-exit")== 0) || (strcmp(person, "\n") == 0) || (strcmp(person, "") == 0)) {
                                break;
                            }
                        } 
                        else {
                            break;
                        }
                    }

                    if ((person[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not chosen a person...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    binn_object_set_str(obj, "request", "-book");
                    binn_object_set_str(obj, "subrequest", "-delete");
                    binn_object_set_str(obj, "person", person);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                    binn_free(obj);

                    puts("\033[0m");

                }
                // Show your address book 
                else if  (strcmp(subRequest, "show\n") == 0) {

                    binn_object_set_str(obj, "request", "-book");
                    binn_object_set_str(obj, "subrequest", "-show");

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                    binn_free(obj);

                    puts("\033[0m");

                } 
                else {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                memset(&subRequest[0], 0, sizeof(subRequest));
                memset(&person[0], 0, sizeof(person));
            }


            // Function to send either a text or a record 
            else if(strcmp(input,"-send\n")==0){

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }
                
                binn *obj  = binn_object();
                
                char *p;
                char subRequest[30] = {0};
                char person[30] = {0};   
                char text[100] = {0};

                puts("\033[36mSend a message"); 

                while (1) 
                {
                    if (subRequest[0] == 0) 
                    {
                        printf("\033[36mChoose an operation\n");  
                        printf("\033[34mmessage (To send some text to a person)\n");  
                        printf("\033[34mrecord (To send an audior record to a person)\033[37m\n");  

                        fgets(subRequest,28, stdin);

                        if (subRequest[strlen(subRequest) - 1] != '\n') {
                            strncpy(subRequest, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        } 

                        if ((strcmp(subRequest, " ") == 0) || (strcmp(subRequest,"-exit")== 0) || (strcmp(subRequest, "\n") == 0) || (strcmp(subRequest, "") == 0)) {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }

                if ((subRequest[0] == 0))  
                {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }


                // Send a message
                if (strcmp(subRequest, "message\n") == 0) {

                    while (1) 
                    {
                        if (person[0] == 0) 
                        {
                            printf("\033[36mChoose a person: \033[37m");  
                            

                            fgets(person,28, stdin);

                            if (person[strlen(person) - 1] != '\n') {
                                strncpy(person, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person); //Total size of string
                                person[size-1] = '\0';
                            }

                            if ((strcmp(person, " ") == 0) || (strcmp(person,"-exit")== 0) || (strcmp(person, "\n") == 0) || (strcmp(person, "") == 0)) {
                                break;
                            }
                        }

                        
                        if (text[0] == 0) 
                        {
                                printf("\033[36mWrite your message: \033[37m");  

                                fgets(text,100, stdin);

                                if (text[strlen(text) - 1] != '\n') {
                                    strncpy(text, "0", sizeof("0"));
                                    puts("\033[31m[INFO] Too many characters...\033[37m");
                                    break;
                                } 
                                else {
                                    int size = strlen(text); //Total size of string
                                    text[size-1] = '\0';
                                }
    
                                                            
                                if ((strcmp(text, "") == 0) || (strcmp(text,"-exit")== 0) || (strcmp(text, "\n") == 0) || (strcmp(text, "") == 0)) {
                                    break;
                                }
                                
                        }
                        else {
                            break;
                        }

                    }

                    
                    if ((person[0] == 0) || (text[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not entered the data correctly...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }


                    binn_object_set_str(obj, "request", "-send");
                    binn_object_set_str(obj, "subrequest", "-message");
                    binn_object_set_str(obj, "person", person);
                    binn_object_set_str(obj, "text", text);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);
                    binn_free(obj);

                    puts("\033[0m");


                }

                // Send a record
                else if (strcmp(subRequest, "record\n") == 0) {
                    if (isRecord) {
                        puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!\n");
                        pthread_mutex_unlock(&lock);
                        continue;
                    } 

                    isRecord = true;
                    pthread_create(&tid, NULL, myThread, NULL);


                }
                else {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                memset(&subRequest[0], 0, sizeof(subRequest));
                memset(&person[0], 0, sizeof(person));
                memset(&text[0], 0, sizeof(text));

            }


            // Stop recording the record
            else if(strcmp(input,"-stop\n")==0) {

                if (!isRecord) {
                    puts("\033[33m[COMMAND] You are not registering... Type -send!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                char person[30] = {0};  

                while (1) 
                {
                    if (person[0] == 0) 
                    {
                        printf("\033[36mChoose a person: \033[37m");  

                        fgets(person,28, stdin);

                        if (person[strlen(person) - 1] != '\n') {
                            strncpy(person, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        } 
                        else {
                                int size = strlen(person); //Total size of string
                                person[size-1] = '\0';
                            }

                        if ((strcmp(person, " ") == 0) || (strcmp(person,"-exit")== 0) || (strcmp(person, "\n") == 0) || (strcmp(person, "") == 0)) {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }

                if ((person[0] == 0))  
                {
                    printf("\033[31m[INFO] You have not chosen a person...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn_object_set_str(obj, "request", "-send");
                binn_object_set_str(obj, "subrequest", "-record");
                binn_object_set_str(obj, "file", last_name_file);
                binn_object_set_str(obj, "person", person);

                send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                pthread_cancel(tid);
                sleep(1);
                puts("\033[33m[COMMAND] Registration completed...\n");   
                sleep(1);
                sendFile();
                sleep(2);

                send(client_sock_desc, "q", strlen("q"), 0);
                sleep(1);

                remove(namePath);
                memset(&last_name_file[0], 0, sizeof(last_name_file));
                memset(&namePath[0], 0, sizeof(namePath));
                binn_free(obj);

                isRecord = false;
            }

            // Manage messages
            else if(strcmp(input,"-message\n")==0) {

                if (isRecord) {
                    puts("\033[33m[COMMAND] Registration in progress... Type -stop to interrupt!");
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                binn *obj  = binn_object();
                char subRequest[30] = {0};
                char person[30] = {0};  
                char idText[10] = {0};  

                puts("\033[37mDo you want to menage messages?"); 

                while (1) 
                {
                    if (subRequest[0] == 0) 
                    {
                        printf("\033[36mChoose an operation\n");  
                        printf("\033[34mnew (To check new messages)\n");  
                        printf("\033[34mall (To see all messages)\n"); 
                        printf("\033[34mdelete (To delete a message) \n");  
                        printf("\033[34monly (To see only message)\n");  
                        printf("\033[34mshow (To see only chat)\n");  
                        printf("\033[34mforward (To forward a message)\033[37m\n");  


                        fgets(subRequest,28, stdin);

                        if (subRequest[strlen(subRequest) - 1] != '\n') {
                            strncpy(subRequest, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        }  

                        if ((strcmp(subRequest, " ") == 0) || (strcmp(subRequest,"-exit")== 0) || (strcmp(subRequest, "\n") == 0) || (strcmp(subRequest, "") == 0)) {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }


                if ((subRequest[0] == 0))  
                {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }

                // Check new messages
                if (strcmp(subRequest, "new\n") == 0) {

                    binn_object_set_str(obj, "request", "-message");
                    binn_object_set_str(obj, "subrequest", "-new");

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                    puts("\033[0m");

                }

                // View all messages
                else if (strcmp(subRequest, "all\n") == 0)  {

                    binn_object_set_str(obj, "request", "-message");
                    binn_object_set_str(obj, "subrequest", "-all");

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                    puts("\033[0m");

                }

                // Delete a message
                else if (strcmp(subRequest, "delete\n") == 0)  {

                    while (1) 
                    {
                        if (idText[0] == 0) 
                        {
                            printf("\033[36mInsert a id message: \033[37m");  
                            
                            fgets(idText,8, stdin);

                            if (idText[strlen(idText) - 1] != '\n') {
                                strncpy(idText, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }  
                            else {
                                int size = strlen(idText); //Total size of string
                                idText[size-1] = '\0';
                            }

                            if ((strcmp(idText, " ") == 0) || (strcmp(idText,"-exit")== 0) || (strcmp(idText, "\n") == 0) || (strcmp(idText, "") == 0)) {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }


                    if ((idText[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not chosen a message...\n");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    binn_object_set_str(obj, "request", "-message");
                    binn_object_set_str(obj, "subrequest", "-delete");
                    binn_object_set_str(obj, "idMessage", idText);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                    puts("\033[0m");
                }


                // Forward a message
                else if (strcmp(subRequest, "forward\n") == 0)  {

                    while (1) 
                    {
                        if (person[0] == 0) 
                        {
                            printf("\033[36mChoose a person: \033[37m");  
                            

                            fgets(person,28, stdin);

                            if (person[strlen(person) - 1] != '\n') {
                                strncpy(person, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }
                            else {
                                int size = strlen(person); //Total size of string
                                person[size-1] = '\0';
                            }

                            if ((strcmp(person, " ") == 0) || (strcmp(person,"-exit")== 0) || (strcmp(person, "\n") == 0) || (strcmp(person, "") == 0)) {
                                break;
                            }
                        }

                        
                        if (idText[0] == 0) 
                        {
                                printf("\033[36mInsert a id message: \033[37m");  

                                fgets(idText,8, stdin);

                                if (idText[strlen(idText) - 1] != '\n') {
                                    strncpy(idText, "0", sizeof("0"));
                                    puts("\033[31m[INFO] Too many characters...\033[37m");
                                    break;
                                }     
                                else {
                                    int size = strlen(idText); //Total size of string
                                    idText[size-1] = '\0';
                                }
                                                            
                                if ((strcmp(idText, "") == 0) || (strcmp(idText,"-exit")== 0) || (strcmp(idText, "\n") == 0) || (strcmp(idText, "") == 0)) {
                                    break;
                                }
                                
                        }
                        else {
                            break;
                        }

                    }

                    
                    if ((person[0] == 0) || (idText[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not entered the data correctly...\n");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }



                    binn_object_set_str(obj, "request", "-message");
                    binn_object_set_str(obj, "subrequest", "-forward");
                    binn_object_set_str(obj, "person", person);
                    binn_object_set_str(obj, "idMessage", idText);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                    puts("\033[0m");


                }

                // Show a chat
                else if (strcmp(subRequest, "show\n") == 0)  {

                    while (1) 
                    {
                        if (person[0] == 0) 
                        {
                            printf("\033[36mChoose a person: \033[37m");  
                            
                            fgets(person,28, stdin);

                            if (idText[strlen(person) - 1] != '\n') {
                                strncpy(person, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }  
                            else {
                                int size = strlen(person); //Total size of string
                                person[size-1] = '\0';
                            }

                            if ((strcmp(person, " ") == 0) || (strcmp(person,"-exit")== 0) || (strcmp(person, "\n") == 0) || (strcmp(person, "") == 0)) {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }


                    if ((person[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not chosen a person...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    binn_object_set_str(obj, "request", "-message");
                    binn_object_set_str(obj, "subrequest", "-show");
                    binn_object_set_str(obj, "person", person);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                    puts("\033[0m");
                }



                // Show only message
                else if (strcmp(subRequest, "only\n") == 0) {

                    while (1) 
                    {
                        if (idText[0] == 0) 
                        {
                            printf("\033[36mInsert a id message: \033[37m");  
                            
                            fgets(idText,8, stdin);

                            if (idText[strlen(idText) - 1] != '\n') {
                                strncpy(idText, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }  
                            else {
                                int size = strlen(idText); //Total size of string
                                idText[size-1] = '\0';
                            }

                            if ((strcmp(idText, " ") == 0) || (strcmp(idText,"-exit")== 0) || (strcmp(idText, "\n") == 0) || (strcmp(idText, "") == 0)) {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }


                    if ((idText[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not chosen a message...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    binn_object_set_str(obj, "request", "-message");
                    binn_object_set_str(obj, "subrequest", "-only");
                    binn_object_set_str(obj, "idMessage", idText);

                    send(client_sock_desc, binn_ptr(obj), binn_size(obj), 0);

                    puts("\033[0m");

                }

                

                else {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }


                memset(&subRequest[0], 0, sizeof(subRequest));
                memset(&person[0], 0, sizeof(person));
                memset(&idText[0], 0, sizeof(idText));

                binn_free(obj);
            }


            else{  
                    puts("\033[0m");
            }

            pthread_mutex_unlock(&lock);
        }
      
    }

}

// Thread to receive messages from the server 
void *message_receiver(){
    int nread;
    while(true){
        char received_msg[1000];

        if (!serverIsOnline){
            continue;
        }
        else
        {
            nread = recv(client_sock_desc,received_msg,1000,0);

            if(nread<=0) {
                puts("\033[31m[INFO] Server is down...");
                serverIsOnline = false;
            }
            else{
                // If the server sends a file to play
                if (strcmp(received_msg, "-play")==0) {
                    
                    int sfd,fd;
                    unsigned int length=sizeof(struct sockaddr_in);
                    long int n, m,count=0;
                    unsigned int nsid;
                    ushort clt_port;
                    char buffer[BUFFERT],filename[256];
                    char dst[INET_ADDRSTRLEN];

                    // It is saved as a tmp and then deleted
                    if ((fd=open("audio/tmp",O_CREAT|O_WRONLY,0600))==-1)
                    {
                        puts("\033[31m[INFO] Open fail...\n");
                        continue;
                    }

                    bzero(buffer,BUFFERT);
                    while ((n = recv(client_sock_desc, buffer, BUFFERT, 0)) > 0) { 
                        if (strcmp(buffer, "-stop")==0) {
                            break;
                        }

                        if((m=write(fd,buffer,n))==-1){
                            puts("\033[31m[INFO] Write fail...\n");
                            break;
                        }
                        count=count+m;
                        bzero(buffer,BUFFERT);
                    }

                    close(fd);

                    puts("\033[33m[COMMAND] Listening in progress...\n");
                    char str[]="./aplay audio/tmp";
                    system(str);
                    remove("audio/tmp");    
                    puts("\033[33m[COMMAND] Listening completed...\033[0m\n");       
 
                }
                // To ping the server 
                else if (strcmp(received_msg, "")==0) {

                }
                // Otherwise, print the message
                else {
                    puts(received_msg);
                    puts("\033[0m");
                }
                
            }   
        }
    }
}


#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <mysql.h>
#include <semaphore.h>
#include <openssl/sha.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include "constant.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdbool.h>
#include <time.h>
#include <binn.h>
#include "utils.h"

// Variables
FILE *file;
char *ipAddress = NULL;
char *portAddress =  NULL;
char *ipVDR1 =  NULL;
char *ipVDR2 =  NULL;
char *portVDR1 = NULL;
char *portVDR2 = NULL;
char *portDB = NULL;
char *dbFlags =  NULL;
char *dbHost =  NULL;
char *dbUser =  NULL;
char *dbName =  NULL;
char *dbPasswd =  NULL;
bool serverIsOnline = true;
bool connectionVDR1 = true;

// Semaphone
static sem_t sem;

// FIle
char line[RSIZ][LSIZ];
int i = 0;
int tot = 0;

// Socket
int client_counter = 0; 
struct sockaddr_in sock_serv,sock_clt;

// Initialization for vdr
int vdr_sock_desc = 0;
clientStruct * listClient;

// Initialize server, socket variables
int socket_desc, new_socket, c, *new_sock;
struct sockaddr_in server, client;
struct sockaddr_in clientVDR;

// Mutex
pthread_mutex_t lock, lock2, lock3;


// Initialize Database
MYSQL *con;
MYSQL_ROW row;      
MYSQL_RES* result;
MYSQL_FIELD* field; 
MYSQL_ROW row1;      
MYSQL_RES* result1;
MYSQL_FIELD* field1; 
MYSQL_ROW row2;      
MYSQL_RES* result2;
MYSQL_FIELD* field2; 
MYSQL_ROW row3;      
MYSQL_RES* result3;
MYSQL_FIELD* field3; 

// Query
char query[300];
char query1[300];
char query2[300];
char selectBook[]= "SELECT idAddressBook FROM user, addressBook WHERE idCreator = idUser AND idUser = %d\n";
char checkUserExist[]= "SELECT * FROM user WHERE usernameUser = '%s'\n";
char insertUser[]="INSERT INTO user(nameUser, surnameUser, usernameUser, passwordUser) VALUES ('%s','%s','%s', '%s')\n";
char checkUserLogin[]= "SELECT * FROM user WHERE usernameUser = '%s' AND passwordUser = '%s'\n";
char insertAddressBook[]="INSERT INTO addressBook(idCreator) VALUES (%d)\n";
char checkBookExist[]="SELECT * FROM detailsBook WHERE idBook = %d AND idPerson = %d \n";
char addDetailsBook[]="INSERT INTO detailsBook(idBook, idPerson) VALUES (%d, %d)\n";
char deleteDetailsBook[]="DELETE FROM detailsBook WHERE idBook = %d AND idPerson = %d\n";
char showBook[]="SELECT surnameUser FROM detailsBook, addressBook, user WHERE idBook = idAddressBook AND idPerson = idUser AND idBook = %d\n";
char selectChat[]="SELECT DISTINCT(idDetailsBook) FROM detailsBook, addressBook, user WHERE idBook = idAddressBook AND idAddressBook = %d AND idPerson = %d\n";
char insertMessage[]="INSERT INTO detailsMessage(idDetails, textMessage, checkSum) VALUES (%d, '%s', '%s')\n";
char insertRecord[]="INSERT INTO detailsMessage(idDetails, nameFile, checkSum) VALUES (%d, '%s', '%s')\n";
char checkNewMessage[]="SELECT DISTINCT(idMessage), textMessage, nameFile, usernameUser, creationMessage FROM detailsMessage, detailsBook, addressBook, user WHERE viewMessage = 0 AND idDetails = idDetailsBook AND idPerson = %d AND idBook = idAddressBook AND idCreator = idUser\n";
char updateViewMessage[]= "UPDATE detailsMessage SET viewMessage = 1 WHERE idMessage = %d\n";
char findReceiver[]="SELECT idUser, usernameUser FROM detailsMessage, detailsBook, user WHERE idDetails = idDetailsBook AND idPerson = idUser AND idMessage = %d\n";
char findSender[]="SELECT idUser,usernameUser, textMessage, nameFile, creationMessage, viewMessage FROM detailsMessage, detailsBook, addressBook, user WHERE idDetails = idDetailsBook AND idBook = idAddressBook AND idCreator = idUser AND idMessage = %d\n";
char showAll[]="SELECT DISTINCT(idMessage), textMessage, nameFile, viewMessage, creationMessage FROM detailsMessage, detailsBook, addressBook, user WHERE  idDetails = idDetailsBook AND (idPerson = %d OR idBook = idAddressBook AND idCreator = idUser AND idUser = %d) ORDER BY idMessage ASC\n";
char deleteMessage[]="DELETE FROM detailsMessage WHERE idMessage = %d\n";

char checkUserVDR[]="SELECT * FROM user WHERE idUser = %d\n LIMIT 1";
char insertUserVDR[]="INSERT INTO user(idUser, nameUser, surnameUser, usernameUser, passwordUser, registrationUser ) VALUES (%d, '%s','%s','%s', '%s', '%s')\n";
char updateUserVDR[]="UPDATE user SET nameUser = '%s', surnameUser = '%s', usernameUser = '%s', passwordUser = '%s' WHERE idUser = %d\n";

char checkAddressBookVDR[]="SELECT * FROM addressBook WHERE idAddressBook = %d\n LIMIT 1";
char insertAddressBookVDR[]="INSERT INTO addressBook(idAddressBook, idCreator, creationBook) VALUES (%d, %d, '%s')\n";
char updateAddressBookVDR[]="UPDATE addressBook SET idCreator = %d WHERE idAddressBook = %d\n";

char checkDetailsVDR[]="SELECT * FROM detailsBook WHERE idDetailsBook = %d\n LIMIT 1";
char insertDetailsVDR[]="INSERT INTO detailsBook(idDetailsBook, idBook, idPerson, creationDetails) VALUES (%d, %d, %d, '%s')\n";
char updateDetailsVDR[]="UPDATE detailsBook SET idBook = %d, idPerson = %d WHERE idDetailsBook = %d\n";

char checkMessageVDR[]="SELECT * FROM detailsMessage WHERE idMessage = %d\n LIMIT 1";
char insertMessageVDR[]="INSERT INTO detailsMessage(idMessage, idDetails, textMessage, nameFile, viewMessage, checkSum, creationMessage) VALUES (%d, %d, '%s', '%s', %d, '%s', '%s')\n";
char updateMessageVDR[]="UPDATE detailsMessage SET idDetails = %d, textMessage = '%s', nameFile = '%s', viewMessage = %d, checkSum = '%s'WHERE idMessage = %d\n";

char deleteUser[]="DELETE FROM user WHERE usernameUser = '%s'\n";
char reset1[]="DELETE FROM user\n";
char reset2[]= "ALTER TABLE user AUTO_INCREMENT=1\n";
char reset3[]= "ALTER TABLE addressBook AUTO_INCREMENT=1\n";
char reset4[]= "ALTER TABLE detailsBook AUTO_INCREMENT=1\n";
char reset5[]= "ALTER TABLE detailsMessage AUTO_INCREMENT=1\n";


// Functions
int isValidIpAddress(char *ipAddress);
void *connection_handler(void *socket_desc);
int isNumber(char s[]);
void *message_sender();
void *message_receiver();
void *pingvdr();
int remove_by_value(clientStruct ** head, int val);
void push(clientStruct * head, int id, int sock, int stamp, char* user);
void delete_list(clientStruct *head);
void print_list(clientStruct * head);
int duration (struct timeval *start,struct timeval *stop, struct timeval *delta);
int create_server_socket (int port);
char* fileToSHA1(char* FILE);
static char *rand_string(char *str, size_t size);
int duplicate(char *src, char *dst);
void *checkSocket();
void close_list_socket(clientStruct * head, sem_t sem);
void sendFile(char *namePath, int client_sock_desc, struct sockaddr_in client);


/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    fflush(stdout);
    delete_list(listClient);
    mysql_close(con);   
    puts("\033[37m\n[INFO] Closing in progress...");
    sleep(2);
    exit(1);
}

// Signal Handler for SIGTSTP
void sighandler(int sig_num)
{
    fflush(stdout);
    delete_list(listClient);
    mysql_close(con);   
    puts("\033[37m\n[INFO] Closing in progress...");
    sleep(2);
    exit(1);
}



int main(int argc, char *argv[]) {

    // Initialize the signals 
    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, sighandler);

    // Initialize the semaphore 
    sem_init(&sem, 0, 100);

    // Initialize the mutex 
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        puts("\033[31m[INFO] Mutex init failed...");
        return 1;
    }

    if (pthread_mutex_init(&lock2, NULL) != 0)
    {
        puts("\033[31m[INFO] Mutex init failed...");
        return 1;
    }

    if (pthread_mutex_init(&lock3, NULL) != 0)
    {
        puts("\033[31m[INFO] Mutex init failed...");
        return 1;
    }

    // Initialize list
    listClient= (clientStruct *) malloc(sizeof(clientStruct));


    puts("\033[37m[INFO] Server");
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
                    if (((strcmp(token, "PORT")) != 0) && ((strcmp(token, "IP")) != 0) && ((strcmp(token, "VDR")) != 0) 
                        && ((strcmp(token, "HOST")) != 0)&& ((strcmp(token, "USER")) != 0)&& ((strcmp(token, "PASSWD")) != 0)&& ((strcmp(token, "DB")) != 0)
                        && ((strcmp(token, "DBPORT")) != 0)&& ((strcmp(token, "FLAGS")) != 0) && ((strcmp(token, "VDR1PORT")) != 0) && ((strcmp(token, "VDR2PORT")) != 0)    ) {
                        

                        if (isValidIpAddress(token) == 1) {
                            if (ipAddress == NULL) {
                                ipAddress = token;
                                token = strtok(NULL, "=");
                                continue;
                            }

                            if (ipVDR1 == NULL) {
                                ipVDR1 = token;
                                token = strtok(NULL, "=");
                                continue;
                            }

                            if (ipVDR2 == NULL) {
                                ipVDR2 = token;
                                token = strtok(NULL, "=");
                                continue;
                            }

                        }
                        else if(isNumber(token) == 1) {
                 
                            if(portAddress == NULL){
                                portAddress = token;
                                token = strtok(NULL, "=");
                                continue;  
                            }

                            if(portVDR1 == NULL){
                                portVDR1 = token;
                                token = strtok(NULL, "=");
                                continue;  
                            }

                            if(portVDR2 == NULL){
                                portVDR2 = token;
                                token = strtok(NULL, "=");
                                continue;  
                            }

                            if (portDB == NULL) {
                                portDB = token;
                                token = strtok(NULL, "=");
                                continue;
                            }

                            
                            if (dbFlags == NULL) {
                                dbFlags = token;
                                break;
                                
                            }
                        }      
                        else{
                            if (dbHost == NULL) {
                                    dbHost = token;
                                    token = strtok(NULL, "=");
                                    continue;
                            }  


                            if (dbUser == NULL) {
                                    dbUser = token;
                                    token = strtok(NULL, "=");
                                    continue;
                            } 

                            if (dbPasswd == NULL) {
                                    dbPasswd = token;
                                    token = strtok(NULL, "=");
                                    continue;
                            } 

                            if (dbName == NULL) {
                                    dbName = token;
                                    token = strtok(NULL, "=");
                                    continue;
                            } 

                        }                     
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
    if ((ipAddress == NULL) || (portAddress == NULL) || (ipVDR1 == NULL)|| (ipVDR2 == NULL)|| (dbHost == NULL)|| (dbUser == NULL)
            || (dbPasswd == NULL)|| (dbName == NULL)|| (portDB == NULL)|| (dbFlags == NULL) || (portVDR1 == NULL) || (portVDR2 == NULL)   ) {
        puts("\033[31m[INFO] Set the configuration in the property file...");
        return 1;
    }


    // Print configuration file
    puts("");
    puts("\033[37mServer Configuration");

    printf("\033[36mIP Server: %s\n", ipAddress);
    printf("\033[36mPort: %s\n", portAddress);
    printf("\033[36mIP VDR1: %s\n", ipVDR1);
    printf("\033[36mVDR1 Port: %s\n", portVDR1);
    printf("\033[36mIP VDR1: %s\n", ipVDR2);
    printf("\033[36mVDR2 Port: %s\n\n", portVDR2);

    printf("\033[37mDatabase Configuration\n");

    printf("\033[36mHost: %s\n", dbHost);
    printf("\033[36mUser: %s\n", dbUser);
    printf("\033[36mPassword: %s\n", dbPasswd);
    printf("\033[36mDatabase: %s\n", dbName);
    printf("\033[36mPort: %s\n", portDB);
    printf("\033[36mFlags: %s\n\n", dbFlags);

    // Database
    con = mysql_init(NULL);
    if (!mysql_real_connect(con, dbHost, dbUser, dbPasswd, dbName, atoi(portDB), NULL, atoi(dbFlags))) {
		puts("\033[31m[INFO] Unable to connect to the database...");
		return 1;
	}

    puts("\033[0m[INFO] Successful connection to database...");

    // VDR
    vdr_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (vdr_sock_desc == -1) {
        puts("\033[31m[INFO] Could not create VDR socket...");
        return 1;
    }
    else{
        clientVDR.sin_family = AF_INET;
        clientVDR.sin_addr.s_addr = inet_addr(ipVDR1);
        clientVDR.sin_port = htons(atoi(portVDR1)); 

        if (connect(vdr_sock_desc, (struct sockaddr *)&clientVDR, sizeof(clientVDR))<0){
            puts("\033[31m[INFO] Connection error to VDR...");
            connectionVDR1 = false;
        }
        else{
            puts("\033[0m[INFO] Successful connection to VDR...");
        }

    }

    // Threads
    pthread_t send,receive, pings, checkSock;
    pthread_create(&pings,NULL,pingvdr,NULL);
    pthread_create(&checkSock,NULL,checkSocket,NULL);
    pthread_create(&send,NULL,message_sender,NULL);
    pthread_create(&receive,NULL,message_receiver,NULL);

    // Server
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        puts("\033[31m[INFO] Could not create socket...");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipAddress);
    server.sin_port = htons(atoi(portAddress)); 

    // If the process is not busy, try again 
    do {
        puts("\033[31m[INFO] Busy process. New attempt in 5 seconds...");
        sleep(5);
    } while (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0);

    puts("\033[0m[INFO] Process started...\n");

    listen(socket_desc, 100);

    puts("\033[37m[+] Waiting for incoming connection...");
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))){

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) new_sock) < 0) {
            puts("\033[31m[INFO] Could not create thread...");
            return 1;
        }
    }

    mysql_close(con);   
    delete_list(listClient);
    close(socket_desc);
    close(vdr_sock_desc);
    return 0;

}


// Manages connections with clients
void *connection_handler(void *socket_desc)
{
    char client_input[500];  //received input
    char client_info[200]; //sended output
    char client_output[10000]; //sended output
    // Get the socket descriptor
    int sock = *((int *)socket_desc);

    char client_name[100]={0};
    char client_idUser[10]={0};
    char client_nameReal[100]={0};
    char client_surnameReal[100]={0};
    char client_password[100]={0};

    
    int timestamp;

    // First time the client connects
    if (recv(sock, client_input, sizeof(client_input), 0) > 0)
    {
        strcpy(client_output, "\033[33m[SERVER] Welcome! To log in type -login [username] [password]...");
        write(sock, client_output, strlen(client_output) + 1);

    }
    else
    {
        strcpy(client_output, "\033[31m[SERVER] Server joinning error...");
        write(sock, client_output, strlen(client_output) + 1);
    }

    // The client sends the commands
    while (recv(sock, client_input, sizeof(client_input), 0) > 0){


        if ((!serverIsOnline) || (!connectionVDR1)) {
            continue;
        }

        timestamp = (int)time(NULL);

        // User logout from server 
        if (strcmp(binn_object_str(client_input, "request"), "-exit") == 0){   

            // Logged in
            if (client_name[0] != 0) {
                strcpy(client_info, "\033[33m[COMMAND] User "); 
                strcat(client_info, client_name);
                strcat(client_info, " has left the server...");
                puts(client_info);
            
                strcpy(client_output, "\033[33m[SERVER] You just left the server...");
                send(sock, client_output, strlen(client_output) + 1,0);

                remove_by_value(&listClient, atoi(client_idUser));

                sem_post(&sem);

                sleep(5);
                break;
            }
            else
            {
                strcpy(client_output, "\033[31m[SERVER] You are not logged in...  ");
                send(sock, client_output, strlen(client_output) * sizeof(char) + 1,0);
            }
        }


        else if (strcmp(binn_object_str(client_input, "request"), "-ping") == 0){   

        }


        // Help information 
        else if (strcmp(binn_object_str(client_input, "request"), "-help") == 0){

            // Logged in
            if (client_name[0] != 0) {
                strcpy(client_info, "\033[33m[COMMAND] User "); 
                strcat(client_info, client_name);
                strcat(client_info, " just used the 'help' command...");
                puts(client_info);

                strcpy(client_output, "\033[33m[SERVER] Response\n");
                strcat(client_output, "The available commands are:\n");
                strcat(client_output, "-whoami\n");
                strcat(client_output, "-message\n");
                strcat(client_output, "-send\n");
                strcat(client_output, "-help\n");
                strcat(client_output, "-exit");


                send(sock, client_output, strlen(client_output) * sizeof(char) + 1,0);
            }
            else {
                strcpy(client_info, "\033[33m[COMMAND] An unregistered user just used the 'help' command..."); 
                puts(client_info);

                strcpy(client_output, "\033[33m[SERVER] Response\n");
                strcat(client_output, "The available commands are:\n");
                strcat(client_output, "-login [username] [password]\n");
                strcat(client_output, "-register [name] [surname] [username] [password]\n");
                strcat(client_output, "-exit");


                send(sock, client_output, strlen(client_output) * sizeof(char) + 1,0);
            }
        }



        // User information 
        else if (strcmp(binn_object_str(client_input, "request"), "-whoami") == 0){

            if (client_name[0] != 0) {
                strcpy(client_info, "\033[33m[COMMAND] User "); 
                strcat(client_info, client_name);
                strcat(client_info, " just used the 'whoami' command...");
                puts(client_info);

                strcpy(client_output, "\033[33m[SERVER] Response\nName: ");
                strcat(client_output, client_nameReal);

                strcat(client_output, "\nSurname: ");
                strcat(client_output, client_surnameReal);

                strcat(client_output, "\nUsername: ");
                strcat(client_output, client_name);

                strcat(client_output, "\nPassword: ");
                strcat(client_output, client_password);
                
                strcat(client_output, "\nId: ");
                strcat(client_output, client_idUser);

                send(sock, client_output, strlen(client_output) * sizeof(char) + 1,0);
            }
            else
            {
                strcpy(client_output, "\033[31m[SERVER] You must be logged in to use whoami command...");
                send(sock, client_output, strlen(client_output) * sizeof(char) + 1,0);
            }
        }

        // Login Command
        else if (strcmp(binn_object_str(client_input, "request"), "-login") == 0) {

            if (client_name[0] != 0) {
                        strcpy(client_output, "\033[31m[SERVER] You are already logged in...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
            }

            strcpy(client_name, binn_object_str(client_input, "name"));
            strcpy(client_password, binn_object_str(client_input, "password"));

            
            if ((client_name[0] == 0) || (client_password[0] == 0)) {
                        strcpy(client_output, "\033[31m[SERVER] You did insert wrong inputs. Try again...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
            }

            sprintf(query, checkUserLogin, client_name, client_password);
            if (mysql_query(con, query)){
                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                send(sock, client_output, strlen(client_output) + 1,0);
                memset(&client_name[0], 0, sizeof(client_name));
                continue;
            }

            result = mysql_store_result (con);
            int n_rows = mysql_num_rows (result);

            if (n_rows == 0) {
                mysql_free_result (result);
                strcpy(client_output, "\033[31m[SERVER] The credentials are incorrect...");
                send(sock, client_output, strlen(client_output) + 1,0);
                memset(&client_name[0], 0, sizeof(client_name));
                memset(&client_password[0], 0, sizeof(client_password));
                continue;
            }
            else{
                int i = 0;
                field = mysql_fetch_field (result);
                row = mysql_fetch_row (result); 
                strcpy(client_idUser, row[0]);
                strcpy(client_nameReal, row[1]);
                strcpy(client_surnameReal, row[2]);
                strcpy(client_name, row[3]);
                
            }

            mysql_free_result (result);

            push(listClient, atoi(client_idUser), sock, timestamp, client_name);
            
            strcpy(client_info, "\033[33m[COMMAND] User ");
            strcat(client_info, client_name);
            strcat(client_info, " has joined the server...");

            puts(client_info);
            
            strcpy(client_output, "\033[33m[SERVER] You just joined the server...");
            send(sock, client_output, strlen(client_output) + 1,0);

            sem_wait(&sem);


        }

        // Register command
        else if (strcmp(binn_object_str(client_input, "request"), "-register") == 0) {
            
            
            if (client_name[0] != 0) {
                        strcpy(client_output, "\033[31m[SERVER] You are already logged in...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
            }

            strcpy(client_nameReal, binn_object_str(client_input, "name"));
            strcpy(client_surnameReal, binn_object_str(client_input, "surname"));
            strcpy(client_name, binn_object_str(client_input, "username"));
            strcpy(client_password, binn_object_str(client_input, "password"));

            

            if ((client_nameReal[0] == 0) || (client_surnameReal[0] == 0) || (client_name[0] == 0) || (client_password[0] == 0)) {
                        strcpy(client_output, "\033[31m[SERVER] You did insert wrong inputs. Try again...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
            }

            sprintf(query, checkUserExist, client_name);
            if (mysql_query(con, query)){
                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                send(sock, client_output, strlen(client_output) + 1,0);
                memset(&client_name[0], 0, sizeof(client_name));
                continue;
            }

            result = mysql_store_result (con);
            int n_rows = mysql_num_rows (result);

            if (n_rows == 0) {
                sprintf(query, insertUser, client_nameReal, client_surnameReal, client_name, client_password);
                if (mysql_query(con, query))                   
                {          
                    mysql_free_result (result);
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);                                                                                        
                }
                else
                {
                    mysql_free_result (result);

                    binn *obj2  = binn_object();
                    binn_object_set_str(obj2, "query", query);
                    binn_object_set_str(obj2, "flag", "none");
                    if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                        puts("\033[0m[VDR] The user has also been registered in the VDR...");
                    } 
                    binn_free(obj2);

                    strcpy(client_info, "\033[33m[COMMAND] User ");
                    strcat(client_info, client_name);
                    strcat(client_info, " has just signed up...");
                    puts(client_info);

                    sprintf(query, checkUserExist, client_name);
                    if (mysql_query(con, query)){
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        memset(&client_name[0], 0, sizeof(client_name));
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(client_idUser, row[0]);
                    }

                    mysql_free_result (result);

                    
                    sprintf(query, insertAddressBook, atoi(client_idUser));


                    if (mysql_query(con, query))                   
                    {          
                        mysql_free_result (result);
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);                                                                                        
                    }
                    else
                    {
                        binn *obj2  = binn_object();
                        binn_object_set_str(obj2, "query", query);
                        binn_object_set_str(obj2, "flag", "none");
                        if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                            puts("\033[0m[VDR] The address book has also been registered in the VDR...");
                        } 
                        binn_free(obj2);


                        strcpy(client_output, "\033[33m[SERVER] Registration successful...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                    }
                    
                }

            }
            else
            {
                mysql_free_result (result);
                strcpy(client_output, "\033[31m[SERVER] The user with this username already exists...");
                send(sock, client_output, strlen(client_output) + 1,0);
            }

                memset(&client_name[0], 0, sizeof(client_name));
                memset(&client_nameReal[0], 0, sizeof(client_nameReal));
                memset(&client_surnameReal[0], 0, sizeof(client_surnameReal));
                memset(&client_password[0], 0, sizeof(client_password));
                memset(&client_idUser[0], 0, sizeof(client_idUser));
        }


        // Function to manage your own address book (request)
        else if (strcmp(binn_object_str(client_input, "request"), "-book") == 0) {

            char * person = NULL;
            char idPerson[10];
            char idBook[10];

            sprintf(query, selectBook, atoi(client_idUser));
            
            if (mysql_query(con, query)){
                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                send(sock, client_output, strlen(client_output) + 1,0);
                continue;
            }

            result = mysql_store_result (con);
            int n_rows = mysql_num_rows (result);

            if (n_rows == 0) {
                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                send(sock, client_output, strlen(client_output) + 1,0);  
                continue;
            }
            else{
                int i = 0;
                field = mysql_fetch_field (result);
                row = mysql_fetch_row (result); 
                strcpy(idBook, row[0]);
            }
            mysql_free_result (result);


            if (client_name[0] == 0) {
                strcpy(client_output, "\033[31m[SERVER] You must be logged in to use whoami command...");
                send(sock, client_output, strlen(client_output) + 1,0);
                continue;
            }

            // Subrequest (add) -> Add a person
            if (strcmp(binn_object_str(client_input, "subrequest"), "-add") == 0) {

                person = binn_object_str(client_input, "person");

                if (person == NULL) {
                    strcpy(client_output, "\033[31m[SERVER] You did insert wrong inputs. Try again...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                    continue;
                }

                if (person != NULL) {
                    sprintf(query, checkUserExist, person);
                    if (mysql_query(con, query)){
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        mysql_free_result (result);
                        strcpy(client_output, "\033[31m[SERVER] There is no user with this username. Try again...");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        continue;
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(idPerson, row[0]);
                        mysql_free_result (result);
                    }
                }


                sprintf(query, checkBookExist, atoi(idBook), atoi(idPerson));
                if (mysql_query(con, query)){
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                    continue;
                }

                result = mysql_store_result (con);
                int n_rows = mysql_num_rows (result);

                if (n_rows != 0) {
                    strcpy(client_output, "\033[31m[SERVER] The name already belongs to your address book...");
                    send(sock, client_output, strlen(client_output) + 1,0); 
                    mysql_free_result (result); 
                    continue;
                }
                mysql_free_result (result);

                sprintf(query, addDetailsBook, atoi(idBook), atoi(idPerson));

                if (mysql_query(con, query))                   
                {          
                    mysql_free_result (result);
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);  
                    continue;                                                                                      
                }
                else
                {
                    binn *obj2  = binn_object();
                    binn_object_set_str(obj2, "query", query);
                    binn_object_set_str(obj2, "flag", "none");
                    if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                        puts("\033[0m[VDR] The user has been added to the address book in the VDR...");
                    } 
                    binn_free(obj2);

                    strcpy(client_output, "\033[33m[SERVER] Successfully added to your address book...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                }

                strcpy(client_info, "\033[33m[COMMAND] User ");
                strcat(client_info, client_name);
                strcat(client_info, " has just added (");
                strcat(client_info, person);
                strcat(client_info, ")...");
                puts(client_info);

            }

            // Subrequest (delete) -> Delete a person from the address book
            else if (strcmp(binn_object_str(client_input, "subrequest"), "-delete") == 0) {

                person = binn_object_str(client_input, "person");

                if (person == NULL) {
                    strcpy(client_output, "\033[31m[SERVER] You did insert wrong inputs. Try again...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                    continue;
                }

                if (person != NULL) {
                    sprintf(query, checkUserExist, person);
                    if (mysql_query(con, query)){
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        mysql_free_result (result);
                        strcpy(client_output, "\033[31m[SERVER] There is no user with this username. Try again...");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        continue;
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(idPerson, row[0]);
                        mysql_free_result (result);
                    }
                }

                sprintf(query, checkBookExist, atoi(idBook), atoi(idPerson));
                if (mysql_query(con, query)){
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                    continue;
                }

                result = mysql_store_result (con);
                int n_rows = mysql_num_rows (result);

                if (n_rows == 0) {
                    strcpy(client_output, "\033[31m[SERVER] The name  does not belong in your address book...");
                    send(sock, client_output, strlen(client_output) + 1,0); 
                    mysql_free_result (result); 
                    continue;
                }
                mysql_free_result (result);

                sprintf(query, deleteDetailsBook, atoi(idBook), atoi(idPerson));

                if (mysql_query(con, query))                   
                {          
                    mysql_free_result (result);
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);  
                    continue;                                                                                      
                }
                else
                {
                    binn *obj2  = binn_object();
                    binn_object_set_str(obj2, "query", query);
                    binn_object_set_str(obj2, "flag", "none");
                    if((send(vdr_sock_desc,  binn_ptr(obj2), binn_size(obj2) ,0)>=0)){
                        puts("\033[0m[VDR] The user has been deleted from the address book in the VDR...");
                    } 
                    binn_free(obj2);
                    
                    strcpy(client_output, "\033[33m[SERVER] Successful remove from your address book...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                }

                strcpy(client_info, "\033[33m[COMMAND] User ");
                strcat(client_info, client_name);
                strcat(client_info, " has just deleted (");
                strcat(client_info, person);
                strcat(client_info, ")...");
                puts(client_info);

            }

            // Subrequest (show) -> Show the address book
            else if (strcmp(binn_object_str(client_input, "subrequest"), "-show") == 0) {
                sprintf(query, showBook,  atoi(idBook));

                if (mysql_query(con, query)){
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);  
                    continue;   
                }

                result = mysql_store_result (con);
                int n_rows = mysql_num_rows (result);

                strcpy(client_output, "\033[33m[SERVER] Your address book contains...");

                for (int j = 0; j < n_rows; j++) {
                    int i = 0;
                    mysql_field_seek (result, 0);           
                    row = mysql_fetch_row (result);      
                    while ((field = mysql_fetch_field (result))) {
                            strcat(client_output, "\n");
                            strcat(client_output, row[i++]);
                            strcat(client_output, "\n");
                    }
                }
                
                send(sock, client_output, strlen(client_output) + 1,0);  
                mysql_free_result (result);

                strcpy(client_info, "\033[33m[COMMAND] User ");
                strcat(client_info, client_name);
                strcat(client_info, " has just showed all users...");
                puts(client_info);
            }
            else {
                strcpy(client_output, "\033[31m[SERVER] You have to type the correct option...\n");
                strcat(client_output, "-book [add/delete/show] [person]");
                send(sock, client_output, strlen(client_output) + 1,0);
            }



        }

        // Function to send a message or record
        else if (strcmp(binn_object_str(client_input, "request"), "-send") == 0) {

            char *person = NULL;
            char *message = NULL;
            char idPerson[10];
            char idBook[10];
            char selectCh[10];

            if (client_name[0] == 0) {
                strcpy(client_output, "\033[31m[SERVER] You must be logged in to use send command...");
                send(sock, client_output, strlen(client_output) + 1,0);
                continue;
            }

            sprintf(query, selectBook, atoi(client_idUser));
                
            if (mysql_query(con, query)){
                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                send(sock, client_output, strlen(client_output) + 1,0);
                continue;
            }

            result = mysql_store_result (con);
            int n_rows = mysql_num_rows (result);

            if (n_rows == 0) {
                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                send(sock, client_output, strlen(client_output) + 1,0);  
                continue;
            }
            else{
                int i = 0;
                field = mysql_fetch_field (result);
                row = mysql_fetch_row (result); 
                strcpy(idBook, row[0]);
            }
            mysql_free_result (result);

            // Subrequest (message) -> Function to send a message 
            if (strcmp(binn_object_str(client_input, "subrequest"), "-message") == 0) {
                person = binn_object_str(client_input, "person");
                message = binn_object_str(client_input, "text");

                sprintf(query, checkUserExist, person);
                if (mysql_query(con, query)){
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                    continue;
                }

                result = mysql_store_result (con);
                int n_rows = mysql_num_rows (result);

                if (n_rows == 0) {
                    mysql_free_result (result);
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);  
                    continue;
                }
                else{
                    int i = 0;
                    field = mysql_fetch_field (result);
                    row = mysql_fetch_row (result); 
                    strcpy(idPerson, row[0]);
                    mysql_free_result (result);
                }
                

                sprintf(query, selectChat, atoi(idBook), atoi(idPerson));
                
                if (mysql_query(con, query)){
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                    continue;
                }

                result = mysql_store_result (con);
                n_rows = mysql_num_rows (result);

                if (n_rows == 0) {
                    strcpy(client_output, "\033[31m[SERVER] This person does not belong to your address book. Please add him...");
                    send(sock, client_output, strlen(client_output) + 1,0);  
                    continue;
                }
                else{
                    int i = 0;
                    field = mysql_fetch_field (result);
                    row = mysql_fetch_row (result); 
                    strcpy(selectCh, row[0]);
                }
                mysql_free_result (result);


                char *sha1 = convertToSHA1(message);

                sprintf(query, insertMessage, atoi(selectCh), message, sha1);

                if (mysql_query(con, query))                   
                {          
                    mysql_free_result (result);
                    strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                    send(sock, client_output, strlen(client_output) + 1,0);  
                    continue;                                                                                      
                }
                else
                {
                    
                    binn *obj2  = binn_object();
                    binn_object_set_str(obj2, "query", query);
                    binn_object_set_str(obj2, "flag", "none");
                    if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                        puts("\033[0m[VDR] The user has also been sent the message in the VDR...");
                    } 
                    binn_free(obj2);

                 
                    strcpy(client_output, "\033[33m[SERVER] You have successfully sent the message...");
                    send(sock, client_output, strlen(client_output) + 1,0);
                }

                strcpy(client_info, "\033[33m[COMMAND] User ");
                strcat(client_info, client_name);
                strcat(client_info, " has just sent a message to (");
                strcat(client_info, person);
                strcat(client_info, ")...");
                puts(client_info);

            }

            // Subrequest (message) -> Function to send a record
            else if (strcmp(binn_object_str(client_input, "subrequest"), "-record") == 0) {
                char *person = binn_object_str(client_input, "person");
                char *nameF = binn_object_str(client_input, "file");

                int sfd,fd;
                unsigned int length=sizeof(struct sockaddr_in);
                long int n, m,count=0;
                unsigned int nsid;
                ushort clt_port;
                char buffer[BUFFERT],filename[256];
                char dst[INET_ADDRSTRLEN];

                int n_rows;
                
                char pathP[30];
                strcpy(pathP, "audio/");
                strcat(pathP, nameF);


                if ((fd=open(pathP,O_CREAT|O_WRONLY,0600))==-1)
                {
                    continue;
                }

                bzero(buffer,BUFFERT);
                while ((n = recv(sock, buffer, BUFFERT, 0)) > 0) { 

                    if (strcmp(buffer, "q") == 0)    {
                        break;
                    }

                    if((m=write(fd,buffer,n))==-1){
                        break;
                    }
                    count=count+m;
                    bzero(buffer,BUFFERT);
                }

                close(fd);

                
                if (person != NULL) {
                        sprintf(query2, checkUserExist, person);

                        if (mysql_query(con, query2)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        result = mysql_store_result (con);
                        n_rows = mysql_num_rows (result);

                        if (n_rows == 0) {
                            mysql_free_result (result);
                            strcpy(client_output, "\033[31m[SERVER] The user does not exist...");
                            send(sock, client_output, strlen(client_output) + 1,0);  
                            continue;
                        }
                        else{
                            field = mysql_fetch_field (result);
                            row = mysql_fetch_row (result); 
                            strcpy(idPerson, row[0]);
                            mysql_free_result (result);
                        }
                    }
                    else {
                        continue;
                    }



                    sprintf(query, selectChat, atoi(idBook), atoi(idPerson));
                    
                    if (mysql_query(con, query)){
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
                    }

                    result = mysql_store_result (con);
                    n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        strcpy(client_output, "\033[33m[SERVER] This person does not belong to your address book. Please add him...");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        continue;
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(selectCh, row[0]);
                    }

                    
                    // Calculate SHA1 from FILE
                    char *codeSHA1 = fileToSHA1(pathP);

                    sprintf(query, insertRecord, atoi(selectCh), nameF, codeSHA1);

                    if (mysql_query(con, query))                   
                    {          
                        mysql_free_result (result);
                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        continue;                                                                                      
                    }
                    else
                    {
                        binn *obj2  = binn_object();
                        binn_object_set_str(obj2, "query", query);
                        binn_object_set_str(obj2, "flag", "send");
                        binn_object_set_str(obj2, "file", pathP);
                        if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                            puts("\033[0m[VDR] The user has also been sent the record in the VDR...");
                        } 
                        sendFile(pathP, vdr_sock_desc, clientVDR);
                        sleep(2);
                        send(vdr_sock_desc, "q", strlen("q"), 0);
                        sleep(1);
                        binn_free(obj2);

                        strcpy(client_output, "\033[33m[SERVER] You have successfully sent the record...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                    }

                    strcpy(client_info, "\033[33m[COMMAND] User ");
                    strcat(client_info, client_name);
                    strcat(client_info, " has just sent a record to (");
                    strcat(client_info, person);
                    strcat(client_info, ")...");
                    puts(client_info);

            }
            else {
                strcpy(client_output, "\033[31m[SERVER] You have to type the correct option...\n");
                send(sock, client_output, strlen(client_output) + 1,0);
            }

        }


        // Function to menage messag    es
        else if (strcmp(binn_object_str(client_input, "request"), "-message") == 0) {

                    char *person = NULL;
                    char *idMessage = NULL;

                    char idPerson[10];
                    char idBook[10];
                    char selectCh[10];

                    if (client_name[0] == 0) {
                        strcpy(client_output, "\033[31m[SERVER] You must be logged in to use message command...");
                        send(sock, client_output, strlen(client_output) + 1,0);
                        continue;
                    }

                    // Subrequest (new) -> Function to check new message
                    if (strcmp(binn_object_str(client_input, "subrequest"), "-new") == 0) {
                        
                        sprintf(query, checkNewMessage, atoi(client_idUser));
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }


                        
                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        strcpy(client_output, "\033[33m[SERVER] New message...\n");

                        for (int j = 0; j < n_rows; j++) {
                            int i = 0;
                            mysql_field_seek (result, 0);           
                            row = mysql_fetch_row (result);      
                            while ((field = mysql_fetch_field (result))) {

                                    switch(i) {
                                        case 0:
                                            strcat(client_output, "Id Message: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");


                                            break;
                                        case 3:
                                            strcat(client_output, "Sender: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");
                                            break;
                                        case 1: 
                                            if(strcmp(row[i], "0")!= 0) {
                                            strcat(client_output, "Type: ");
                                            strcat(client_output, "Text");
                                            strcat(client_output, "\n");
                                            }
                                            break;
                                        case 2: 
                                            if(strcmp(row[i], "0") != 0) {
                                            strcat(client_output, "Type: ");
                                            strcat(client_output, "Audio");
                                            strcat(client_output, "\n");
                                            }
                                            break;
                                        case 4:
                                            strcat(client_output, "Data: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");
                                            break;
                                        default:
                                            break;

                                    }
                                    i++;                        
                            }
                            strcat(client_output, "\n");
                        }


                        
                        
                        char numb[10] = "";
                        sprintf(numb, "%d", n_rows);
                        strcat(client_output, "Total: ");
                        strcat(client_output, numb);
                        strcat(client_output, " messages");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        mysql_free_result (result);

                        strcpy(client_info, "\033[33m[COMMAND] User ");
                        strcat(client_info, client_name);
                        strcat(client_info, " has just cheked new message...");
                        puts(client_info);
                    }

                    // Subrequest (all) -> Function to see all messages
                    else if (strcmp(binn_object_str(client_input, "subrequest"), "-all") == 0) {

                        sprintf(query, showAll, atoi(client_idUser), atoi(client_idUser));
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        
                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        strcpy(client_output, "\033[33m[SERVER] The messagges related to you will be extrapolated...\n");

                        for (int j = 0; j < n_rows; j++) {
                            int i = 0;
                            mysql_field_seek (result, 0);           
                            row = mysql_fetch_row (result);      
                            while ((field = mysql_fetch_field (result))) {

                                    switch(i) {
                                        case 0:
                                            strcat(client_output, "Id Message: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");

                                            

                                            sprintf(query1, findSender, atoi(row[i]));

                                            if (mysql_query(con, query1)){
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);
                                                continue;
                                            }

                                            result1 = mysql_store_result (con);
                                            int n_rows1 = mysql_num_rows (result1);

                                            if (n_rows1 == 0) {
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);  
                                                continue;
                                            }
                                            else{
                                                row1 = mysql_fetch_row (result1); 
                                                strcat(client_output, "Sender: ");
                                                strcat(client_output, row1[1]);
                                                strcat(client_output, "\n");

                                            }

                                            mysql_free_result (result1);


                                            sprintf(query2, findReceiver, atoi(row[i]));

                                            if (mysql_query(con, query2)){
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);
                                                continue;
                                            }

                                            result2 = mysql_store_result (con);
                                            int n_rows2 = mysql_num_rows (result2);

                                            if (n_rows2 == 0) {
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);  
                                                continue;
                                            }
                                            else{
                                                row2 = mysql_fetch_row (result2); 
                                                strcat(client_output, "Recipient: ");
                                                strcat(client_output, row2[1]);
                                                strcat(client_output, "\n");

                                            }
                                            
                                            mysql_free_result (result2);
                                            
                                            
                                            break;
                                        case 1: 
                                            if(strcmp(row[i], "0")!= 0) {
                                            strcat(client_output, "Text: ");
                                            strcat(client_output, "N.D.");
                                            strcat(client_output, "\n");
                                            }
                                            break;
                                        case 2: 
                                            if(strcmp(row[i], "0") != 0) {
                                            strcat(client_output, "File: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");
                                            }
                                            break;
                                        case 3:
                                            if(strcmp(row[i], "0") == 0) {
                                                strcat(client_output, "View: ");
                                                strcat(client_output, "NO");
                                                strcat(client_output, "\n");
                                                break;
                                            }
                                            else
                                            {
                                                strcat(client_output, "View: ");
                                                strcat(client_output, "YES");
                                                strcat(client_output, "\n");
                                                break;
                                            }
                                            
                                        case 4:
                                            strcat(client_output, "Data: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");
                                            break;
                                        default:
                                            break;

                                    }
                                    i++;                        
                            }
                            strcat(client_output, "\n");
                        }

                        char numb[10] = "";
                        sprintf(numb, "%d", n_rows);
                        strcat(client_output, "Total: ");
                        strcat(client_output, numb);
                        strcat(client_output, " messages");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        mysql_free_result (result);

                        strcpy(client_info, "\033[33m[COMMAND] User ");
                        strcat(client_info, client_name);
                        strcat(client_info, " has just cheked all message...");
                        puts(client_info);

                    }

                    // Subrequest (delete) -> Function to delete a message
                    else if (strcmp(binn_object_str(client_input, "subrequest"), "-delete") == 0) {
                        idMessage = binn_object_str(client_input, "idMessage");

                        sprintf(query, findSender, atoi(idMessage));
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        
                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        if (n_rows == 0) {
                            strcpy(client_output, "\033[31m[SERVER] The following Id does not exist. Check...");
                            send(sock, client_output, strlen(client_output) + 1,0);  
                            continue;
                        }
                        else{
                            row = mysql_fetch_row (result); 
                            char path[30];

                            if (strcmp(row[0], client_idUser)== 0){
                                    
                                    sprintf(query1, deleteMessage, atoi(idMessage));

                                    if (mysql_query(con, query1)){
                                        strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                        send(sock, client_output, strlen(client_output) + 1,0);
                                        continue;
                                    }
                                    else {
                                        if (strcmp(row[3], "0") != 0) {
                                            strcpy(path, "audio/");
                                            strcat(path, row[3]);

                                            if (remove(path) != 0) {
                                                puts("\033[31m[SERVER] Error...");
                                            } 
                                        }
                                    }

                                    binn *obj2  = binn_object();
                                    binn_object_set_str(obj2, "query", query1);
                                    binn_object_set_str(obj2, "flag", "delete");
                                    binn_object_set_str(obj2, "file", path);
                                    if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                                        puts("\033[0m[VDR] The user has also been sent the message in the VDR...");
                                    } 
                                    binn_free(obj2);



                                    strcpy(client_output, "\033[33m[SERVER] You have just deleted the message...");
                                    send(sock, client_output, strlen(client_output) + 1,0);  

                                    strcpy(client_info, "\033[33m[COMMAND] User ");
                                    strcat(client_info, client_name);
                                    strcat(client_info, " has just deleted a message with id (");
                                    strcat(client_info, idMessage);
                                    strcat(client_info, ")");
                                    puts(client_info);
             
                        }


                            else {
                                strcpy(client_output, "\033[31m[SERVER] You can't delete the message because it doesn't belong to you...");
                                send(sock, client_output, strlen(client_output) + 1,0);  
                                continue;
                            }
                        }

                        mysql_free_result (result);



                    }

                    // Subrequest (forward) -> Function to forward a message
                    else if (strcmp(binn_object_str(client_input, "subrequest"), "-forward") == 0) {

                        person = binn_object_str(client_input, "person");
                        idMessage = binn_object_str(client_input, "idMessage");

                        sprintf(query, selectBook, atoi(client_idUser));
                        
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        if (n_rows == 0) {
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);  
                            continue;
                        }
                        else{
                            int i = 0;
                            field = mysql_fetch_field (result);
                            row = mysql_fetch_row (result); 
                            strcpy(idBook, row[0]);
                        }
                        mysql_free_result (result);




                        if (person != NULL) {
                            sprintf(query, checkUserExist, person);
                            if (mysql_query(con, query)){
                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                send(sock, client_output, strlen(client_output) + 1,0);
                                continue;
                            }

                            result = mysql_store_result (con);
                            int n_rows = mysql_num_rows (result);

                            if (n_rows == 0) {
                                mysql_free_result (result);
                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                send(sock, client_output, strlen(client_output) + 1,0);  
                                continue;
                            }
                            else{
                                int i = 0;
                                field = mysql_fetch_field (result);
                                row = mysql_fetch_row (result); 
                                strcpy(idPerson, row[0]);
                                mysql_free_result (result);
                            }
                        }



                        sprintf(query, selectChat, atoi(idBook), atoi(idPerson));
                        
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        result = mysql_store_result (con);
                        n_rows = mysql_num_rows (result);

                        if (n_rows == 0) {
                            strcpy(client_output, "\033[31m[SERVER] This person does not belong to your address book. Please add him...");
                            send(sock, client_output, strlen(client_output) + 1,0);  
                            continue;
                        }
                        else{
                            int i = 0;
                            field = mysql_fetch_field (result);
                            row = mysql_fetch_row (result); 
                            strcpy(selectCh, row[0]);
                        }
                        mysql_free_result (result);



                        sprintf(query, findSender, atoi(idMessage));
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        
                        result = mysql_store_result (con);
                        int n_rows4 = mysql_num_rows (result);

                        if (n_rows4 == 0) {
                            strcpy(client_output, "\033[31m[SERVER] The following Id does not exist. Check...");
                            send(sock, client_output, strlen(client_output) + 1,0);  
                            continue;
                        }
                        else{

                            row = mysql_fetch_row (result); 

                            if (strcmp(row[0], client_idUser)== 0){


                                    // If the message is a text
                                    if ((strcmp(row[2], "0"))!= 0) {
                                        
                                        char *sha1 = convertToSHA1(row[2]);

                                        sprintf(query2, insertMessage, atoi(selectCh), row[2], sha1);

                                        if (mysql_query(con, query2))                   
                                        {          
                                            mysql_free_result (result2);
                                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                            send(sock, client_output, strlen(client_output) + 1,0);  
                                            continue;                                                                                      
                                        }
                                        else
                                        {

                                            binn *obj2  = binn_object();
                                            binn_object_set_str(obj2, "query", query2);
                                            binn_object_set_str(obj2, "flag", "none");
                                            if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                                                puts("\033[0m[VDR] The user has also been sent the message in the VDR...");
                                            } 
                                            binn_free(obj2);

                                            strcpy(client_output, "\033[33m[SERVER] You have just forwarded the message...");
                                            send(sock, client_output, strlen(client_output) + 1,0);
                                        }
                                    }
                                    // If the message is a record
                                    else if ((strcmp(row[3], "0"))!= 0) {

                                        char pathSRC[30];
                                        char pathDST[30];
                                        char nameFis[30];
                                        
                                        strcpy(pathSRC, "audio/");
                                        strcat(pathSRC, row[3]);

                                        strcpy(pathDST, "audio/");
                                        strcat(pathDST, rand_string(nameFis, 17));

                                        // Duplicate a file
                                        int flag = duplicate(pathSRC, pathDST);

                                        if (flag == 1) {
                                            puts("\033[31m[SERVER] Error...");
                                            break;
                                        }

                                        char *codeSHA1 = fileToSHA1(pathDST);

                                        sprintf(query, insertRecord, atoi(selectCh), nameFis, codeSHA1);

                                        if (mysql_query(con, query))                   
                                        {          
                                            mysql_free_result (result);
                                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                            send(sock, client_output, strlen(client_output) + 1,0);  
                                            continue;                                                                                      
                                        }
                                        else
                                        {
                                            binn *obj2  = binn_object();
                                            binn_object_set_str(obj2, "query", query);
                                            binn_object_set_str(obj2, "flag", "send");
                                            binn_object_set_str(obj2, "file", pathDST);
                                            if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                                                puts("\033[0m[VDR] The user has also been forwarded the record in the VDR...");
                                            } 
                                            sendFile(pathDST, vdr_sock_desc, clientVDR);
                                            sleep(2);
                                            send(vdr_sock_desc, "q", strlen("q"), 0);
                                            sleep(1);
                                            binn_free(obj2);            
                                      
                                            strcpy(client_output, "\033[33m[SERVER] You have just forwarded the message...");
                                            send(sock, client_output, strlen(client_output) + 1,0);
                                        }

                                    }
                                    else {
                                        strcpy(client_output, "\033[31m[SERVER] You can't forward the message because it doesn't belong to you...");
                                        send(sock, client_output, strlen(client_output) + 1,0);
                                    }


                                    strcpy(client_info, "\033[33m[COMMAND] User ");
                                    strcat(client_info, client_name);
                                    strcat(client_info, " has just forwarded a message to user (");
                                    strcat(client_info, person);
                                    strcat(client_info, ")");
                                    puts(client_info);
    
                        }


                            else {
                                mysql_free_result (result);
                                strcpy(client_output, "\033[31m[SERVER] You can't forward the message because it doesn't belong to you...");
                                send(sock, client_output, strlen(client_output) + 1,0);  
                                continue;
                            }
                        }

                        mysql_free_result (result);
                    }

                    // Subrequest (show) -> Function to show a chat
                    else if (strcmp(binn_object_str(client_input, "subrequest"), "-show") == 0)  {

                        person = binn_object_str(client_input, "person");

                        sprintf(query, showAll, atoi(client_idUser), atoi(client_idUser));
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        
                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        strcpy(client_output, "\033[33m[SERVER] The messages exchanged with this person are...\n");

                        int count = 0;
                        for (int j = 0; j < n_rows; j++) {
                            int i = 0;
                            mysql_field_seek (result, 0);           
                            row = mysql_fetch_row (result);      
                            while ((field = mysql_fetch_field (result))) {

                                    switch(i) {
                                        case 0:
                                                     

                                            sprintf(query1, findSender, atoi(row[i]));

                                            if (mysql_query(con, query1)){
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);
                                                continue;
                                            }

                                            result1 = mysql_store_result (con);
                                            int n_rows1 = mysql_num_rows (result1);

                                            if (n_rows1 == 0) {
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);  
                                                continue;
                                            }
                                            
                                            row1 = mysql_fetch_row (result1); 
                                                

                                            sprintf(query2, findReceiver, atoi(row[i]));

                                            if (mysql_query(con, query2)){
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);
                                                continue;
                                            }

                                            result2 = mysql_store_result (con);
                                            int n_rows2 = mysql_num_rows (result2);

                                            if (n_rows2 == 0) {
                                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                                send(sock, client_output, strlen(client_output) + 1,0);  
                                                continue;
                                            }

                                            row2 = mysql_fetch_row (result2); 

                                            if (((strcmp(row1[1], client_name) == 0) && (strcmp(row2[1], person) == 0)) || 
                                                ((strcmp(row1[1], person) == 0) && (strcmp(row2[1], client_name) == 0))) {
                                                    
                                                    strcat(client_output, "Id Message: ");
                                                    strcat(client_output, row[i]);
                                                    strcat(client_output, "\n");

                                                    strcat(client_output, "Sender: ");
                                                    strcat(client_output, row1[1]);
                                                    strcat(client_output, "\n");

                                                    strcat(client_output, "Recipient: ");
                                                    strcat(client_output, row2[1]);
                                                    strcat(client_output, "\n");
                                                    count++;
                                                }
                                            else
                                            {
                                                mysql_free_result (result1);
                                                mysql_free_result (result2);
                                                continue;
                                            }
                                        

                                            mysql_free_result (result1);
                                            mysql_free_result (result2);

                                            break;
                                        case 1: 
                                            if(strcmp(row[i], "0")!= 0) {
                                            strcat(client_output, "Text: ");
                                            strcat(client_output, "N.D.");
                                            strcat(client_output, "\n");
                                            }
                                            break;
                                        case 2: 
                                            if(strcmp(row[i], "0") != 0) {
                                            strcat(client_output, "File: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");
                                            }
                                            break;
                                        case 3:
                                            if(strcmp(row[i], "0") == 0) {
                                                strcat(client_output, "View: ");
                                                strcat(client_output, "NO");
                                                strcat(client_output, "\n");
                                                break;
                                            }
                                            else
                                            {
                                                strcat(client_output, "View: ");
                                                strcat(client_output, "YES");
                                                strcat(client_output, "\n");
                                                break;
                                            }
                                            
                                        case 4:
                                            strcat(client_output, "Data: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n\n");
                                            break;
                                        default:
                                            break;

                                    }
                                    i++;                        
                            }
                        }

                        char numb[10] = "";
                        sprintf(numb, "%d", count);
                        strcat(client_output, "Total: ");
                        strcat(client_output, numb);
                        strcat(client_output, " messages");
                        send(sock, client_output, strlen(client_output) + 1,0);  
                        mysql_free_result (result);

                        strcpy(client_info, "\033[33m[COMMAND] User ");
                        strcat(client_info, client_name);
                        strcat(client_info, " has just showed messages...");
                        puts(client_info);
                    }

                    // Subrequest (only) -> Function to show only message
                    else if (strcmp(binn_object_str(client_input, "subrequest"), "-only") == 0) {

                        idMessage = binn_object_str(client_input, "idMessage");

                        sprintf(query, findSender, atoi(idMessage));
                        if (mysql_query(con, query)){
                            strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                            send(sock, client_output, strlen(client_output) + 1,0);
                            continue;
                        }

                        
                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        if (n_rows == 0) {
                            strcpy(client_output, "\033[31m[SERVER] The following Id does not exist. Check...");
                            send(sock, client_output, strlen(client_output) + 1,0);  
                            continue;
                        }
                        
                        else{
                            row = mysql_fetch_row (result); 

                            sprintf(query1, findReceiver, atoi(idMessage));

                            if (mysql_query(con, query1)){
                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                send(sock, client_output, strlen(client_output) + 1,0);
                                continue;
                            }

                            result2 = mysql_store_result (con);
                            int n_rows2 = mysql_num_rows (result2);

                            if (n_rows2 == 0) {
                                
                                mysql_free_result (result1);
                                mysql_free_result (result2);
                                strcpy(client_output, "\033[31m[SERVER] Error. Try later...");
                                send(sock, client_output, strlen(client_output) + 1,0);  
                                continue;
                            }    
                            else {
                                    row1 = mysql_fetch_row (result2); 
                                }                                   
                            }


                            if ((strcmp(row[0], client_idUser) != 0) && (strcmp(row1[0], client_idUser) != 0)) {
                                strcpy(client_output, "\033[31m[SERVER] This message does not belong to you...");
                                send(sock, client_output, strlen(client_output) + 1,0);  
                                mysql_free_result (result1);
                                mysql_free_result (result2);
                                continue;
                            }


;
                            if (strcmp(row1[0], client_idUser) == 0){
                                sprintf(query2, updateViewMessage, atoi(idMessage));
                                puts(query2);
                                            
                                if (mysql_query(con, query2)){
                                        break;
                                }

                                binn *obj2  = binn_object();
                                binn_object_set_str(obj2, "query", query2);
                                binn_object_set_str(obj2, "flag", "none");
                                if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                                    puts("\033[0m[VDR] The user has also been seen the message...");
                                } 
                                binn_free(obj2);
                          
                            } 
                            
                            strcpy(client_output, "\033[33m[SERVER] You have just opened the message...\n");

                            // If the message is a text
                            if (strcmp(row[2], "0") != 0) {
                                    strcat(client_output, "Id Message: ");
                                    strcat(client_output, idMessage);
                                    strcat(client_output, "\n");

                                    strcat(client_output, "Sender: ");
                                    strcat(client_output, row[1]);
                                    strcat(client_output, "\n");

                                    strcat(client_output, "Receiver: ");
                                    strcat(client_output, row1[1]);
                                    strcat(client_output, "\n");

                                    strcat(client_output, "Text: ");
                                    strcat(client_output, row[2]);
                                    strcat(client_output, "\n");

                                    if (strcmp(row[5], "0") == 0){
                                        strcat(client_output, "View: ");
                                        strcat(client_output, "NO");
                                        strcat(client_output, "\n");
                                    }
                                    else {
                                        strcat(client_output, "View: ");
                                        strcat(client_output, "YES");
                                        strcat(client_output, "\n");
                                    } 

                                    strcat(client_output, "Data: ");
                                    strcat(client_output, row[4]);
                                    strcat(client_output, "\n");


                            } 

                            // If the message is a record
                            else if (strcmp(row[3], "0") != 0) {

                                struct timeval start, stop, delta;
                                char buf[BUFFERT];
                                off_t count=0, m,sz;//long
                                long int n;
                                int sfd,fd;
                                int l=sizeof(struct sockaddr_in);
                                struct stat buffer;
                                char dest[30];

                                send(sock, "-play", strlen("-play") + 1,0);  
                                sleep(1);
                                strcpy(dest, "audio/");
                                strcat(dest, row[3]);
                            

                                if ((fd = open(dest,O_RDONLY))==-1){
                                    perror("open fail");
                                    exit(1);
                                }

                                //taille du fichier
                                if (stat(dest,&buffer)==-1){
                                    perror("stat fail");
                                    exit(1);
                                }
                                else
                                    sz=buffer.st_size;


                                bzero(&buf,BUFFERT);

                                gettimeofday(&start,NULL);
                                n=read(fd,buf,BUFFERT);
                                while(n){
                                    if(n==-1){
                                        exit(1);
                                    }
                                    m=sendto(sock,buf,n,0,(struct sockaddr*)&server,l);
                                    if(m==-1){
                                        exit(1);
                                    }
                                    count+=m;
                                    //fprintf(stdout,"----\n%s\n----\n",buf);
                                    bzero(buf,BUFFERT);
                                    n=read(fd,buf,BUFFERT);
                                }

                                m=sendto(sock,buf,0,0,(struct sockaddr *)&server,l);

                                sleep(1);
                                send(sock, "-stop", strlen("-stop") + 1,0);  


                            } 
                            else {

                            }


                            send(sock, client_output, strlen(client_output) + 1,0);  

                            strcpy(client_info, "\033[33m[COMMAND] User ");
                            strcat(client_info, client_name);
                            strcat(client_info, " has just opened a message with id (");
                            strcat(client_info, idMessage);
                            strcat(client_info, ")");
                            puts(client_info);

                            //mysql_free_result (result1);
                            //mysql_free_result (result2);
                    }

                    else {

                        strcpy(client_output, "\033[31m[SERVER] You have to type the correct option...\n");
                        send(sock, client_output, strlen(client_output) + 1,0);
                    }

        }

        // Another
        else{

            strcpy(client_output, "\033[31m[SERVER] You have to type the correct command...\n");
            send(sock, client_output, strlen(client_output) + 1,0);
        }
    }

    close(sock);
    fflush(stdout);
    free(socket_desc); //Free the socket pointer
    return 0;
}



// Constantly check if the VDR is online
void *pingvdr(){
    // Ping    
    int nread;
    while(true){
        if (connectionVDR1) {
            nread = send(vdr_sock_desc,"",strlen("")+1,0);
            if(nread<0){
                puts("\033[34m[VDR] VDR is down...");
                pthread_mutex_lock(&lock2);
                connectionVDR1 = false;
            } 
        }
        else {
            close(vdr_sock_desc);

            vdr_sock_desc = socket(AF_INET, SOCK_STREAM, 0);
            if (vdr_sock_desc == -1) {

            }
            else{
                clientVDR.sin_family = AF_INET;
                clientVDR.sin_addr.s_addr = inet_addr(ipVDR1);
                clientVDR.sin_port = htons(atoi(portVDR1)); 

                if (connect(vdr_sock_desc, (struct sockaddr *)&clientVDR, sizeof(clientVDR))<0){
                    continue;
                }
                connectionVDR1 = true;
                pthread_mutex_unlock(&lock2);
                puts("\033[34m[VDR] VDR is up...");
            }
            
        }  
        sleep(5);
    }

}


// Constantly check if clients are online
void *checkSocket(){
    // Ping    
    int nread;

    while(true){
        if (!connectionVDR1) {
            continue;
        }

        if (!serverIsOnline) {
            continue;
        }
        close_list_socket(listClient, sem); 
        sleep(5);
    }

}

// The server can send commands
void *message_sender(){
    while(true){
        
        char input[500];
        fgets(input,500, stdin);

        if (!connectionVDR1) {
            puts("\033[31m[SERVER] VDR is down. Wait...\n");   
            continue;
        }

        pthread_mutex_lock(&lock);
        

        // Stop server
        if(strcmp(input,"-stop\n")==0){            
            serverIsOnline= false; 
            puts("\033[37m[SERVER] The server has been stopped...\n");                             
        }

        // Start server
        else if (strcmp(input,"-start\n")==0) {
            serverIsOnline= true;  
            puts("\033[37m[SERVER] The server has been started...\n");
        }

        // Sync server
        else if (strcmp(input,"-sync\n")==0){
            if((send(vdr_sock_desc,input,strlen(input)+1,0)>=0)&&(connectionVDR1==true)){
                puts("\033[37m[SERVER] Synchronization with the vdr has been forced...");
            } 
        } 

        // Check who is online
        else if (strcmp(input,"-isonline\n")==0){
            puts("\033[37m[SERVER] The online users at the moment are...");
            print_list(listClient);
        }

        // Help command
        else if (strcmp(input,"-help\n")==0) {

                char client_inf[1000];

                strcpy(client_inf, "\033[33m[SERVER] Response\n");
                strcat(client_inf, "The available commands are:\n");
                strcat(client_inf, "-message\n");
                strcat(client_inf, "-login\n");
                strcat(client_inf, "-register\n");
                strcat(client_inf, "-start\n");
                strcat(client_inf, "-reset\n");
                strcat(client_inf, "-isonline\n");
                strcat(client_inf, "-sync\n");
                strcat(client_inf, "-stop\n");
                strcat(client_inf, "-exit\n");

                puts(client_inf);

        }

        // Register a user
        else if (strcmp(input,"-register\n")==0) {
                
            char *p;
            char passwd[30] = {0};
            char name[30] = {0};  
            char surname[30] = {0};  
            char username[30] = {0};  
            char idUser[10] = {0};  
            char client_info[200]; //sended output
        

            puts("\033[37mRegistration"); 

                while (1) 
                {
                    if (name[0] == 0) 
                    {
                        printf("\033[36mEnter the name: \033[37m");  
                        
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
                        printf("\033[36mEnter the surname: \033[37m");  
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
                        printf("\033[36mEnter the username: \033[37m");  
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
                            printf("\033[36mEnter the password: \033[37m");  

                            fgets(passwd,28, stdin);

                            if (passwd[strlen(passwd) - 1] != '\n') {
                                strncpy(passwd, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            }
                            else {
                                    int size = strlen(passwd); //Total size of string
                                    passwd[size-1] = '\0';
                                }   
                                                        
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
                    memset(&passwd[0], 0, sizeof(passwd));
                    memset(&name[0], 0, sizeof(name));
                    memset(&idUser[0], 0, sizeof(idUser));
                    memset(&surname[0], 0, sizeof(surname));
                    memset(&username[0], 0, sizeof(username));
                    memset(&client_info[0], 0, sizeof(client_info));
                    pthread_mutex_unlock(&lock);
                    continue;
                }



            sprintf(query, checkUserExist, username);
            if (mysql_query(con, query)){
                memset(&passwd[0], 0, sizeof(passwd));
                memset(&name[0], 0, sizeof(name));
                memset(&idUser[0], 0, sizeof(idUser));
                memset(&surname[0], 0, sizeof(surname));
                memset(&username[0], 0, sizeof(username));
                memset(&client_info[0], 0, sizeof(client_info));
                puts("\033[31m[SERVER] Error. Try later...");
                pthread_mutex_unlock(&lock);
                continue;
            }

            result = mysql_store_result (con);
            int n_rows = mysql_num_rows (result);

            char *sha1 = convertToSHA1(passwd); 

            if (n_rows == 0) {
                sprintf(query, insertUser, name, surname, username, sha1);
                if (mysql_query(con, query))                   
                {          
                    mysql_free_result (result);
                    puts("\033[31m[SERVER] Error. Try later...");
                }
                else
                {
                    mysql_free_result (result);

                    binn *obj2  = binn_object();
                    binn_object_set_str(obj2, "query", query);
                    binn_object_set_str(obj2, "flag", "none");
                    if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                        puts("\033[0m[VDR] The user has also been registered in the VDR...");
                    } 
                    binn_free(obj2);

                    sprintf(query, checkUserExist, username);
                    if (mysql_query(con, query)){
                        memset(&passwd[0], 0, sizeof(passwd));
                        memset(&name[0], 0, sizeof(name));
                        memset(&idUser[0], 0, sizeof(idUser));
                        memset(&surname[0], 0, sizeof(surname));
                        memset(&client_info[0], 0, sizeof(client_info));
                        memset(&username[0], 0, sizeof(username));
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        puts("\033[31m[SERVER] Error. Try later...");
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(idUser, row[0]);
                    }

                    mysql_free_result (result);

                    
                    sprintf(query, insertAddressBook, atoi(idUser));


                    if (mysql_query(con, query))                   
                    {          
                        mysql_free_result (result);
                        puts("\033[31m[SERVER] Error. Try later...");
                    }
                    else
                    {
                        binn *obj2  = binn_object();
                        binn_object_set_str(obj2, "query", query);
                        binn_object_set_str(obj2, "flag", "none");
                        if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                            puts("\033[0m[VDR] The user has also been registered in the VDR...");
                        } 
                        binn_free(obj2);

                        puts("\033[33m[SERVER] Registration successful...");
                    }
                    
                }

            }
            else
            {
                mysql_free_result (result);
                puts("\033[31m[SERVER] The user with this username already exists...");
            }


                puts("\033[0m");

                memset(&passwd[0], 0, sizeof(passwd));
                memset(&name[0], 0, sizeof(name));
                memset(&idUser[0], 0, sizeof(idUser));
                memset(&client_info[0], 0, sizeof(client_info));
                memset(&surname[0], 0, sizeof(surname));
                memset(&username[0], 0, sizeof(username));

        }


        // Delete a user
        else if (strcmp(input,"-delete\n")==0) {
                char person[30] = {0};  

                puts("\033[37mDo you want to delete a person?"); 

                while (1) 
                {
                    if (person[0] == 0) 
                    { 
                        printf("\033[36mChoose a person: \033[37m\n");  
                        
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

                

                sprintf(query1, deleteUser, person);

                if (mysql_query(con, query1)){
                    puts("\033[31m[SERVER] Error. Try later...");
                    pthread_mutex_unlock(&lock);
                    continue;
                }
                else {

                    binn *obj2  = binn_object();
                    binn_object_set_str(obj2, "query", query1);
                    binn_object_set_str(obj2, "flag", "none");
                    if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                        puts("\033[0m[VDR] The user has also been sent the message in the VDR...");
                    } 
                    binn_free(obj2);
                    printf("\033[33m[SERVER] You just deleted the user %s...", person);
                }
                
                puts("\033[37m");
        }

        // Delete all server
        else if(strcmp(input,"-reset\n")==0) {

            puts("\033[37m[SERVER] Resetting in progress..."); 
            

            if (mysql_query(con, reset1)){
                puts("\033[31m[SERVER] Error. Try later...");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (mysql_query(con, reset2)){
                puts("\033[31m[SERVER] Error. Try later...");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (mysql_query(con, reset3)){
                puts("\033[31m[SERVER] Error. Try later...");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (mysql_query(con, reset4)){
                puts("\033[31m[SERVER] Error. Try later...");
                pthread_mutex_unlock(&lock);
                continue;
            }

            if (mysql_query(con, reset5)){
                puts("\033[31m[SERVER] Error. Try later...");
                pthread_mutex_unlock(&lock);
                continue;
            }

            puts("\033[37m[SERVER] Done! Now you can digit -sync..."); 


        }

        // Manage the messages
        else if(strcmp(input,"-message\n")==0) {

                char subRequest[30] = {0};
                char person1[30] = {0};  
                char person2[30] = {0};  
                char idText[10] = {0};  
                char idBook[10] = {0};
                char selectCh[10] = {0};
                char client_output[500]; //sended output

                puts("\033[37mDo you want to manage a message?"); 

                while (1) 
                {
                    if (subRequest[0] == 0) 
                    {
                        printf("\033[36mChoose an operation\n");  
                        printf("\033[34mall\n"); 
                        printf("\033[34mdelete \n");  
                        printf("\033[34monly\n");  
                        printf("\033[34mshow \n");  
                        printf("\033[34mforward \033[37m\n");  
                        
                        fgets(subRequest,28, stdin);

                        if (subRequest[strlen(subRequest) - 1] != '\n') {
                            strncpy(subRequest, "0", sizeof("0"));
                            puts("\033[31m[INFO] Too many characters...\033[37m");
                            break;
                        } 
                        else {
                            int size = strlen(subRequest); //Total size of string
                            subRequest[size-1] = '\0';
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

                // See all messages from a person
                if (strcmp(subRequest, "all") == 0)  {

                    char *idUs = NULL;

                    while (1) 
                    {
                        if (person1[0] == 0) 
                        {
                            printf("\033[36mChoose a person: \033[37m");  
                            fgets(person1,28, stdin);

                            if (person1[strlen(person1) - 1] != '\n') {
                                strncpy(person1, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person1); //Total size of string
                                person1[size-1] = '\0';
                            } 

                            if ((strcmp(person1, " ") == 0) || (strcmp(person1,"-exit")== 0) || (strcmp(person1, "\n") == 0) || (strcmp(person1, "") == 0)) {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }


                    if ((person1[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not chosen a person...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }


                    sprintf(query, checkUserExist, person1);
                    if (mysql_query(con, query)){
                        memset(&subRequest[0], 0, sizeof(subRequest));
                        memset(&person1[0], 0, sizeof(person1));
                        memset(&person2[0], 0, sizeof(person2));
                        memset(&idText[0], 0, sizeof(idText));
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        memset(&subRequest[0], 0, sizeof(subRequest));
                        memset(&person1[0], 0, sizeof(person1));
                        memset(&person2[0], 0, sizeof(person2));
                        memset(&idText[0], 0, sizeof(idText));
                        puts("\033[31m[SERVER] This user doesn not exist...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else {

                        row2 = mysql_fetch_row (result); 
                        idUs = row2[0];
                    }

                    mysql_free_result (result);

                    sprintf(query, showAll, atoi(idUs), atoi(idUs));
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    
                    result = mysql_store_result (con);
                    n_rows = mysql_num_rows (result);

                    strcpy(client_output, "\033[33m[SERVER] The messagges related to you will be extrapolated...\n");

                    for (int j = 0; j < n_rows; j++) {
                        int i = 0;
                        mysql_field_seek (result, 0);           
                        row = mysql_fetch_row (result);      
                        while ((field = mysql_fetch_field (result))) {

                                switch(i) {
                                    case 0:
                                        strcat(client_output, "Id Message: ");
                                        strcat(client_output, row[i]);
                                        strcat(client_output, "\n");

                                        

                                        sprintf(query1, findSender, atoi(row[i]));

                                        if (mysql_query(con, query1)){
                                            puts("\033[31m[SERVER] Error. Try later...");
                                            memset(&subRequest[0], 0, sizeof(subRequest));
                                            memset(&person1[0], 0, sizeof(person1));
                                            memset(&person2[0], 0, sizeof(person2));
                                            memset(&idText[0], 0, sizeof(idText));
                                            continue;
                                        }

                                        result1 = mysql_store_result (con);
                                        int n_rows1 = mysql_num_rows (result1);

                                        if (n_rows1 == 0) {
                                            memset(&subRequest[0], 0, sizeof(subRequest));
                                            memset(&person1[0], 0, sizeof(person1));
                                            memset(&person2[0], 0, sizeof(person2));
                                            memset(&idText[0], 0, sizeof(idText));
                                            puts("\033[31m[SERVER] Error. Try later...");
                                            continue;
                                        }
                                        else{
                                            row1 = mysql_fetch_row (result1); 
                                            strcat(client_output, "Sender: ");
                                            strcat(client_output, row1[1]);
                                            strcat(client_output, "\n");

                                        }

                                        mysql_free_result (result1);


                                        sprintf(query2, findReceiver, atoi(row[i]));

                                        if (mysql_query(con, query2)){
                                            puts("\033[31m[SERVER] Error. Try later...");
                                            continue;
                                        }

                                        result2 = mysql_store_result (con);
                                        int n_rows2 = mysql_num_rows (result2);

                                        if (n_rows2 == 0) {
                                            puts("\033[31m[SERVER] Error. Try later...");
                                            continue;
                                        }
                                        else{
                                            row2 = mysql_fetch_row (result2); 
                                            strcat(client_output, "Recipient: ");
                                            strcat(client_output, row2[1]);
                                            strcat(client_output, "\n");

                                        }
                                        
                                        mysql_free_result (result2);
                                        
                                        
                                        break;
                                    case 1: 
                                        if(strcmp(row[i], "0")!= 0) {
                                        strcat(client_output, "Text: ");
                                        strcat(client_output, row[i]);
                                        strcat(client_output, "\n");
                                        }
                                        break;
                                    case 2: 
                                        if(strcmp(row[i], "0") != 0) {
                                        strcat(client_output, "File: ");
                                        strcat(client_output, row[i]);
                                        strcat(client_output, "\n");
                                        }
                                        break;
                                    case 3:
                                        if(strcmp(row[i], "0") == 0) {
                                            strcat(client_output, "View: ");
                                            strcat(client_output, "NO");
                                            strcat(client_output, "\n");
                                            break;
                                        }
                                        else
                                        {
                                            strcat(client_output, "View: ");
                                            strcat(client_output, "YES");
                                            strcat(client_output, "\n");
                                            break;
                                        }
                                        
                                    case 4:
                                        strcat(client_output, "Data: ");
                                        strcat(client_output, row[i]);
                                        strcat(client_output, "\n");
                                        break;
                                    default:
                                        break;

                                }
                                i++;                        
                        }
                        strcat(client_output, "\n");
                    }

                    char numb[10] = "";
                    sprintf(numb, "%d", n_rows);
                    strcat(client_output, "Total: ");
                    strcat(client_output, numb);
                    strcat(client_output, " messages");
                    puts(client_output);
                    mysql_free_result (result);

                    puts("\033[0m");

                }

                // Delete a message of a person
                else if (strcmp(subRequest, "delete") == 0)  {

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


                    sprintf(query, findSender, atoi(idText));
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    
                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    char path[30];

                    if (n_rows == 0) {
                        puts("\033[31m[SERVER] The following Id does not exist. Check...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else{
                        row = mysql_fetch_row (result); 
                                
                        sprintf(query1, deleteMessage, atoi(idText));

                        if (mysql_query(con, query1)){
                            puts("\033[31m[SERVER] Error. Try later...");
                            pthread_mutex_unlock(&lock);
                            continue;
                        }
                        else {
                            if (strcmp(row[3], "0") != 0) {
                                strcpy(path, "audio/");
                                strcat(path, row[3]);

                                if (remove(path) != 0) {
                                    puts("\033[31m[SERVER] Error...");
                                } 
                            }
                        }

                        binn *obj2  = binn_object();
                        binn_object_set_str(obj2, "query", query1);
                        binn_object_set_str(obj2, "flag", "delete");
                        binn_object_set_str(obj2, "file", path);
                        if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                            puts("\033[0m[VDR] The user has also been sent the message in the VDR...");
                        } 
                        binn_free(obj2);


                        puts("\033[33m[SERVER] You have just deleted the message...");
            
                    }          





                    puts("\033[0m");



                }

                // Forward a message
                else if (strcmp(subRequest, "forward") == 0)  {

                    char *idUs1 = NULL;
                    char *idUs2 = NULL;

                    while (1) 
                    {
                        if (person1[0] == 0) 
                        {
                            printf("\033[36mChoose the first person: \033[37m");  
                            fgets(person1,28, stdin);

                            if (person1[strlen(person1) - 1] != '\n') {
                                strncpy(person1, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person1); //Total size of string
                                person1[size-1] = '\0';
                            }

                            if ((strcmp(person1, " ") == 0) || (strcmp(person1,"-exit")== 0) || (strcmp(person1, "\n") == 0) || (strcmp(person1, "") == 0)) {
                                break;
                            }
                        }


                        if (person2[0] == 0) 
                        {
                            printf("\033[36mChoose the second person: \033[37m");  
                            fgets(person2,28, stdin);

                            if (person2[strlen(person2) - 1] != '\n') {
                                strncpy(person2, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person2); //Total size of string
                                person2[size-1] = '\0';
                            }

                            if ((strcmp(person2, " ") == 0) || (strcmp(person2,"-exit")== 0) || (strcmp(person2, "\n") == 0) || (strcmp(person2, "") == 0)) {
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

                    
                    if ((person1[0] == 0) || (person2[0] == 0) || (idText[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not entered the data correctly...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }


                    sprintf(query, checkUserExist, person1);
                    if (mysql_query(con, query)){
                        memset(&subRequest[0], 0, sizeof(subRequest));
                        memset(&person1[0], 0, sizeof(person1));
                        memset(&person2[0], 0, sizeof(person2));
                        memset(&idText[0], 0, sizeof(idText));
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        memset(&subRequest[0], 0, sizeof(subRequest));
                        memset(&person1[0], 0, sizeof(person1));
                        memset(&person2[0], 0, sizeof(person2));
                        memset(&idText[0], 0, sizeof(idText));
                        puts("\033[31m[SERVER] This user doesn not exist...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else {

                        row2 = mysql_fetch_row (result); 
                        idUs1 = row2[0];
                    }

                    mysql_free_result (result);


                    sprintf(query, selectBook, atoi(idUs1));
                        
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    result = mysql_store_result (con);
                    n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(idBook, row[0]);
                    }
                    mysql_free_result (result);




                    if (person2 != NULL) {
                        sprintf(query, checkUserExist, person2);
                        if (mysql_query(con, query)){
                            puts("\033[31m[SERVER] Error. Try later...");
                            pthread_mutex_unlock(&lock);
                            continue;
                        }

                        result = mysql_store_result (con);
                        int n_rows = mysql_num_rows (result);

                        if (n_rows == 0) {
                            mysql_free_result (result);
                            puts("\033[31m[SERVER] Error. Try later...");
                            pthread_mutex_unlock(&lock);
                            continue;
                        }
                        else{
                            int i = 0;
                            field = mysql_fetch_field (result);
                            row = mysql_fetch_row (result); 
                            idUs2 = row[0];
                            mysql_free_result (result);
                        }
                    }



                    sprintf(query, selectChat, atoi(idBook), atoi(idUs2));
                    
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    result = mysql_store_result (con);
                    n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        puts("\033[31m[SERVER] This person does not belong to his address book. Please add him...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else{
                        int i = 0;
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 
                        strcpy(selectCh, row[0]);
                    }
                    mysql_free_result (result);



                    sprintf(query, findSender, atoi(idText));
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    
                    result = mysql_store_result (con);
                    int n_rows4 = mysql_num_rows (result);

                    if (n_rows4 == 0) {
                        puts("\033[31m[SERVER] The following Id does not exist. Check...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else{

                        row = mysql_fetch_row (result); 


                        // If the message is a text
                        if ((strcmp(row[2], "0"))!= 0) {
                            
                            char *sha1 = convertToSHA1(row[2]);

                            sprintf(query2, insertMessage, atoi(selectCh), row[2], sha1);

                            if (mysql_query(con, query2))                   
                            {          
                                mysql_free_result (result2);
                                puts("\033[31m[SERVER] Error. Try later...");
                                pthread_mutex_unlock(&lock);
                                continue;                                                                                      
                            }
                            else
                            {
                                binn *obj2  = binn_object();
                                binn_object_set_str(obj2, "query", query2);
                                binn_object_set_str(obj2, "flag", "none");
                                if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                                    puts("\033[0m[VDR] The user has also been sent the message in the VDR...");
                                } 
                                binn_free(obj2);

                                puts("\033[33m[SERVER] You have just forwarded the message...");
                            }
                        }
                        else if ((strcmp(row[3], "0"))!= 0) {
                            
                            char pathSRC[30];
                            char pathDST[30];
                            char nameFis[30];
                            
                            strcpy(pathSRC, "audio/");
                            strcat(pathSRC, row[3]);

                            strcpy(pathDST, "audio/");
                            strcat(pathDST, rand_string(nameFis, 17));

                            int flag = duplicate(pathSRC, pathDST);

                            if (flag == 1) {
                                puts("\033[31m[SERVER] Error...");
                                pthread_mutex_unlock(&lock);
                                continue;
                            }

                            char *codeSHA1 = fileToSHA1(pathDST);

                            sprintf(query, insertRecord, atoi(selectCh), nameFis, codeSHA1);

                            if (mysql_query(con, query))                   
                            {          
                                puts("\033[31m[SERVER] Error. Try later...");
                                pthread_mutex_unlock(&lock);
                                continue;                                                                                      
                            }
                            else
                            {
                                binn *obj2  = binn_object();
                                binn_object_set_str(obj2, "query", query);
                                binn_object_set_str(obj2, "flag", "send");
                                binn_object_set_str(obj2, "file", pathDST);
                                if((send(vdr_sock_desc, binn_ptr(obj2), binn_size(obj2),0)>=0)){
                                    puts("\033[0m[VDR] The user has also been forwarded the record in the VDR...");
                                } 
                                sendFile(pathDST, vdr_sock_desc, clientVDR);
                                sleep(2);
                                send(vdr_sock_desc, "q", strlen("q"), 0);
                                sleep(1);
                                binn_free(obj2);   
                                
                                puts("\033[33m[SERVER] You have just forwarded the message...");
                            }
                            
                        }
                        else {
                            strcpy(client_output, "\033[31m[SERVER] Error...");
                        }

            

                    }

                    mysql_free_result (result);

                    puts("\033[0m");


                }

                // Show a chat
                else if (strcmp(subRequest, "show") == 0)  {

                    char *idUs1 = NULL;
                    char *idUs2 = NULL;

                    while (1) 
                    {
                        if (person1[0] == 0) 
                        {
                            printf("\033[36mChoose the first person: \033[37m");  
                            fgets(person1,28, stdin);

                            if (person1[strlen(person1) - 1] != '\n') {
                                strncpy(person1, "0", sizeof("0"));
                                puts("\033[31m[INFO] Too many characters...\033[37m");
                                break;
                            } 
                            else {
                                int size = strlen(person1); //Total size of string
                                person1[size-1] = '\0';
                            }

                            if ((strcmp(person1, " ") == 0) || (strcmp(person1,"-exit")== 0) || (strcmp(person1, "\n") == 0) || (strcmp(person1, "") == 0)) {
                                break;
                            }
                        }

                        
                        if (person2[0] == 0) 
                        {
                                printf("\033[36mChoose the second person: \033[37m");  

                                fgets(person2,28, stdin);

                                if (person2[strlen(person2) - 1] != '\n') {
                                    strncpy(person2, "0", sizeof("0"));
                                    puts("\033[31m[INFO] Too many characters...\033[37m");
                                    break;
                                } 
                                else {
                                    int size = strlen(person2); //Total size of string
                                    person2[size-1] = '\0';
                                }   
                                                            
                                if ((strcmp(person2, "") == 0) || (strcmp(person2,"-exit")== 0) || (strcmp(person2, "\n") == 0) || (strcmp(person2, "") == 0)) {
                                    break;
                                }
                                
                        }
                        else {
                            break;
                        }

                    }

                    
                    if ((person1[0] == 0) || (person2[0] == 0))  
                    {
                        printf("\033[31m[INFO] You have not entered the data correctly...\n"); 
                        pthread_mutex_unlock(&lock);
                        continue;
                    }


                    sprintf(query, checkUserExist, person1);
                    if (mysql_query(con, query)){
                        memset(&subRequest[0], 0, sizeof(subRequest));
                        memset(&person1[0], 0, sizeof(person1));
                        memset(&person2[0], 0, sizeof(person2));
                        memset(&idText[0], 0, sizeof(idText));
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        memset(&subRequest[0], 0, sizeof(subRequest));
                        memset(&person1[0], 0, sizeof(person1));
                        memset(&person2[0], 0, sizeof(person2));
                        memset(&idText[0], 0, sizeof(idText));
                        puts("\033[31m[SERVER] This user doesn not exist...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else {

                        row2 = mysql_fetch_row (result); 
                        idUs1 = row2[0];
                    }

                    mysql_free_result (result);


                    

                    sprintf(query, showAll, atoi(idUs1), atoi(idUs1));
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    
                    result = mysql_store_result (con);
                    n_rows = mysql_num_rows (result);

                    strcpy(client_output, "\033[33m[SERVER] The messages exchanged with this person are...\n");

                    int count = 0;
                    for (int j = 0; j < n_rows; j++) {
                        int i = 0;
                        mysql_field_seek (result, 0);           
                        row = mysql_fetch_row (result);      
                        while ((field = mysql_fetch_field (result))) {

                            switch(i) {
                                case 0:
                                                
                                    sprintf(query1, findSender, atoi(row[i]));

                                    if (mysql_query(con, query1)){
                                        puts("\033[31m[SERVER] Error. Try later...");
                                        continue;
                                    }

                                    result1 = mysql_store_result (con);
                                    int n_rows1 = mysql_num_rows (result1);

                                    if (n_rows1 == 0) {
                                        puts("\033[31m[SERVER] Error. Try later...");
                                        continue;
                                    }
                                    
                                    row1 = mysql_fetch_row (result1); 
                                        

                                    sprintf(query2, findReceiver, atoi(row[i]));

                                    if (mysql_query(con, query2)){
                                        puts("\033[31m[SERVER] Error. Try later...");
                                        continue;
                                    }

                                    result2 = mysql_store_result (con);
                                    int n_rows2 = mysql_num_rows (result2);

                                    if (n_rows2 == 0) {
                                        puts("\033[31m[SERVER] Error. Try later...");
                                        continue;
                                    }

                                    row2 = mysql_fetch_row (result2); 

                                    if (((strcmp(row1[1], person1) == 0) && (strcmp(row2[1], person2) == 0)) || 
                                        ((strcmp(row1[1], person2) == 0) && (strcmp(row2[1], person1) == 0))) {
                                            
                                            strcat(client_output, "Id Message: ");
                                            strcat(client_output, row[i]);
                                            strcat(client_output, "\n");

                                            strcat(client_output, "Sender: ");
                                            strcat(client_output, row1[1]);
                                            strcat(client_output, "\n");

                                            strcat(client_output, "Recipient: ");
                                            strcat(client_output, row2[1]);
                                            strcat(client_output, "\n");
                                            count++;
                                        }
                                    else
                                    {
                                        mysql_free_result (result1);
                                        mysql_free_result (result2);
                                        continue;
                                    }
                                

                                    mysql_free_result (result1);
                                    mysql_free_result (result2);
                                    break;
                                case 1: 
                                    if(strcmp(row[i], "0")!= 0) {
                                    strcat(client_output, "Text: ");
                                    strcat(client_output, row[i]);
                                    strcat(client_output, "\n");
                                    }
                                    break;
                                case 2: 
                                    if(strcmp(row[i], "0") != 0) {
                                    strcat(client_output, "File: ");
                                    strcat(client_output, row[i]);
                                    strcat(client_output, "\n");
                                    }
                                    break;
                                case 3:
                                    if(strcmp(row[i], "0") == 0) {
                                        strcat(client_output, "View: ");
                                        strcat(client_output, "NO");
                                        strcat(client_output, "\n");
                                        break;
                                    }
                                    else
                                    {
                                        strcat(client_output, "View: ");
                                        strcat(client_output, "YES");
                                        strcat(client_output, "\n");
                                        break;
                                    }
                                    
                                case 4:
                                    strcat(client_output, "Data: ");
                                    strcat(client_output, row[i]);
                                    strcat(client_output, "\n\n");
                                    break;
                                default:
                                    break;

                            }
                            i++;                        
                        }
                    }

                    char numb[10] = "";
                    sprintf(numb, "%d", count);
                    strcat(client_output, "Total: ");
                    strcat(client_output, numb);
                    strcat(client_output, " messages");
                    puts(client_output);  
                    mysql_free_result (result);

                    puts("\033[0m");
                  
                }


                // Show a message
                else if (strcmp(subRequest, "only") == 0) {

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




                    sprintf(query, findSender, atoi(idText));
                    if (mysql_query(con, query)){
                        puts("\033[31m[SERVER] Error. Try later...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }

                    
                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        puts("\033[31m[SERVER] The following Id does not exist. Check...");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    
                    else{
                        row = mysql_fetch_row (result); 

                        sprintf(query1, findReceiver, atoi(idText));

                        if (mysql_query(con, query1)){
                            puts("\033[31m[SERVER] Error. Try later...");
                            pthread_mutex_unlock(&lock);
                            continue;
                        }

                        result2 = mysql_store_result (con);
                        int n_rows2 = mysql_num_rows (result2);

                        if (n_rows2 == 0) {
                            
                            mysql_free_result (result1);
                            mysql_free_result (result2);
                            puts("\033[31m[SERVER] Error. Try later...");
                            pthread_mutex_unlock(&lock);
                            continue;
                        }    
                        else {
                                row1 = mysql_fetch_row (result2); 
                            }                                   
                        }

                        
                        
                        strcpy(client_output, "\033[33m[SERVER] You have just opened the message...\n");

                        if (strcmp(row[2], "0") != 0) {
                                strcat(client_output, "Id Message: ");
                                strcat(client_output, idText);
                                strcat(client_output, "\n");

                                strcat(client_output, "Sender: ");
                                strcat(client_output, row[1]);
                                strcat(client_output, "\n");

                                strcat(client_output, "Receiver: ");
                                strcat(client_output, row1[1]);
                                strcat(client_output, "\n");

                                strcat(client_output, "Text: ");
                                strcat(client_output, row[2]);
                                strcat(client_output, "\n");

                                if (strcmp(row[5], "0") == 0){
                                    strcat(client_output, "View: ");
                                    strcat(client_output, "NO");
                                    strcat(client_output, "\n");
                                }
                                else {
                                    strcat(client_output, "View: ");
                                    strcat(client_output, "YES");
                                    strcat(client_output, "\n");
                                } 

                                strcat(client_output, "Data: ");
                                strcat(client_output, row[4]);
                                strcat(client_output, "\n");

                                puts(client_output);    
                                


                        } 
                        else if (strcmp(row[3], "0") != 0) {

                            char dest[30];

                            strcpy(dest, "audio/");
                            strcat(dest, row[3]);

                            puts("\033[33m[COMMAND] Listening in progress...\n");
                            char str[]="./aplay ";
                            strcat(str, dest);
                            system(str);
                            puts("\033[33m[COMMAND] Listening completed...\033[0m"); 
                        
                        } 
                        else {

                        }     

                        puts("\033[0m"); 

                }

                else {
                    printf("\033[31m[INFO] You have not chosen a command...\n"); 
                    pthread_mutex_unlock(&lock);
                    continue;
                }


                memset(&subRequest[0], 0, sizeof(subRequest));
                memset(&person1[0], 0, sizeof(person1));
                memset(&person2[0], 0, sizeof(person2));
                memset(&idText[0], 0, sizeof(idText));

            }

        else {

        }
        pthread_mutex_unlock(&lock);
        printf("\033[37m");
    }
}

// Receive messages from vdr
void *message_receiver(){
    int nread;
    while(true){
        char received_msg[5000];
        char table[15];

        if (!connectionVDR1){
            continue;
        }
        else
        {
            nread = recv(vdr_sock_desc,received_msg,300,0);

            if(nread<=0) {
                puts("\033[34m[VDR] VDR is down...");
                connectionVDR1 = false;
            }
            else if (strlen(received_msg) < 25) {
            
                // SYNC 
                // Check each record, update it if it is different,
                // insert it if not there
                pthread_mutex_lock(&lock3);
                strcpy(table , binn_object_str(received_msg, "table"));

                if (strcmp(table, "user") == 0) {

                    char idV[10];
                    char nameV[50];
                    char surnameV[50];
                    char usernameV[50];
                    char passwordV[50];
                    char dataV[25];


                    strcpy(idV , binn_object_str(received_msg, "id"));
                    strcpy(nameV, binn_object_str(received_msg, "name"));
                    strcpy(surnameV, binn_object_str(received_msg, "surname"));
                    strcpy(usernameV, binn_object_str(received_msg, "username"));
                    strcpy(passwordV, binn_object_str(received_msg, "password"));
                    strcpy(dataV, binn_object_str(received_msg, "data"));
                    

                    sprintf(query, checkUserVDR, atoi(idV));
                    if (mysql_query(con, query)){
                        printf("\033[31m[SERVER] Error. Try later...");
                        printf("\033[37m\n");
                        pthread_mutex_unlock(&lock3);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);

                    if (n_rows == 0) {
                        // Insert
                        sprintf(query, insertUserVDR, atoi(idV), nameV, surnameV, usernameV, passwordV, dataV);
                        if (mysql_query(con, query))                   
                        {          
                            printf("\033[31m[SERVER] Error. Try later...");
                        }
                        else
                        {
                            printf("\033[33m[SYNC] The user with id %d has been registered...", atoi(idV));
                        }

                    }
                    else {

                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 

                        if ((strcmp(nameV, row[1])!=0) || (strcmp(surnameV, row[2])!=0) || (strcmp(usernameV, row[3])!=0) || (strcmp(passwordV, row[4])!=0)){
                            // UPDATE
                            sprintf(query, updateUserVDR, nameV, surnameV, usernameV, passwordV, atoi(idV));
                            if (mysql_query(con, query))                   
                            {          
                                printf("\033[31m[SERVER] Error. Try later...");                    
                            }
                            else
                            {
                                printf("\033[33m[SYNC] The user with id %d has been updated...", atoi(idV));
                            }
                            
                        }
                        else {
                            printf("\033[32m[SYNC] The user with id %d is correct...", atoi(idV));
                        }

                    }



                    mysql_free_result (result);

                    memset(&idV[0], 0, sizeof(idV));
                    memset(&nameV[0], 0, sizeof(nameV));
                    memset(&surnameV[0], 0, sizeof(surnameV));
                    memset(&usernameV[0], 0, sizeof(usernameV));
                    memset(&passwordV[0], 0, sizeof(passwordV));
                    memset(&dataV[0], 0, sizeof(dataV));

                    printf("\033[37m\n");

                }
                else if (strcmp(table, "addressBook") == 0) 
                {
                    char idAddressBookV[10];
                    char idCreatorV[10];
                    char creationBookV[25];

                    strcpy(idAddressBookV , binn_object_str(received_msg, "idAddress"));
                    strcpy(idCreatorV, binn_object_str(received_msg, "idCreator"));
                    strcpy(creationBookV, binn_object_str(received_msg, "creationBook"));

                    sprintf(query, checkAddressBookVDR, atoi(idAddressBookV));
                    if (mysql_query(con, query)){
                        printf("\033[31m[SERVER] Error. Try later...");
                        printf("\033[37m\n");
                        pthread_mutex_unlock(&lock3);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);
                    if (n_rows == 0) {
                        // Insert
                        sprintf(query, insertAddressBookVDR, atoi(idAddressBookV), atoi(idCreatorV), creationBookV);
                        if (mysql_query(con, query))                   
                        {          
                            printf("\033[31m[SERVER] Error. Try later...");
                        }
                        else
                        {
                            printf("\033[33m[SYNC] The book address with id %d has been registered...", atoi(idAddressBookV));
                        }

                    }
                    else {
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 

                        if ((strcmp(idCreatorV, row[1])!=0)){
                            // UPDATE
                            sprintf(query, updateAddressBookVDR, atoi(idCreatorV), atoi(idAddressBookV));
                            if (mysql_query(con, query))                   
                            {          
                                printf("\033[31m[SERVER] Error. Try later...");                    
                            }
                            else
                            {
                                printf("\033[33m[SYNC] The book address with id %d has been updated...", atoi(idAddressBookV));
                            }
                            
                        }
                        else {
                            printf("\033[32m[SYNC] The book address with id %d is correct...", atoi(idAddressBookV));
                        }

                    }


                    mysql_free_result (result);

                    memset(&idAddressBookV[0], 0, sizeof(idAddressBookV));
                    memset(&idCreatorV[0], 0, sizeof(idCreatorV));
                    memset(&creationBookV[0], 0, sizeof(creationBookV));

                    printf("\033[37m\n");
                }


                else if (strcmp(table, "detailsBook") == 0) 
                {
                    char idDetailsBookV[10];
                    char idBookV[10];
                    char idPersonV[10];
                    char creationDetailsV[25];

                    strcpy(idDetailsBookV , binn_object_str(received_msg, "idDetailsBook"));
                    strcpy(idBookV, binn_object_str(received_msg, "idBook"));
                    strcpy(idPersonV, binn_object_str(received_msg, "idPerson"));
                    strcpy(creationDetailsV, binn_object_str(received_msg, "creationDetails"));


                    sprintf(query, checkDetailsVDR, atoi(idDetailsBookV));
                    if (mysql_query(con, query)){
                        printf("\033[31m[SERVER] Error. Try later...");
                        printf("\033[37m\n");
                        pthread_mutex_unlock(&lock3);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);
                    if (n_rows == 0) {
                        // Insert
                        sprintf(query, insertDetailsVDR, atoi(idDetailsBookV), atoi(idBookV), atoi(idPersonV), creationDetailsV);
                        if (mysql_query(con, query))                   
                        {          
                            printf("\033[31m[SERVER] Error. Try later...");
                        }
                        else
                        {
                            printf("\033[33m[SYNC] The details book with id %d has been registered...", atoi(idDetailsBookV));
                        }

                    }
                    else {
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 

                        if ((strcmp(idBookV, row[1])!=0) || (strcmp(idPersonV, row[2])!=0)){
                            // UPDATE
                            sprintf(query, updateDetailsVDR, idBookV, idPersonV, atoi(idDetailsBookV));
                            if (mysql_query(con, query))                   
                            {          
                                printf("\033[31m[SERVER] Error. Try later...");                    
                            }
                            else
                            {
                                printf("\033[33m[SYNC] The details book with id %d has been updated...", atoi(idDetailsBookV));
                            }
                            
                        }
                        else {
                            printf("\033[32m[SYNC] The details book with id %d is correct...", atoi(idDetailsBookV));
                        }

                    }



                    mysql_free_result (result);

                    memset(&idDetailsBookV[0], 0, sizeof(idDetailsBookV));
                    memset(&idBookV[0], 0, sizeof(idBookV));
                    memset(&idPersonV[0], 0, sizeof(idPersonV));
                    memset(&creationDetailsV[0], 0, sizeof(creationDetailsV));
                    
                    printf("\033[37m\n");
                }

                else if (strcmp(table, "detailsMessage") == 0) 
                {
                    
                    char idMessageV[10];
                    char idDetailsV[10];
                    char textMessageV[150];
                    char nameFileV[150];
                    char viewMessageV[2];
                    char checkSumV[50];
                    char creationMessageV[25];

                    strcpy(idMessageV , binn_object_str(received_msg, "idMessage"));
                    strcpy(idDetailsV , binn_object_str(received_msg, "idDetails"));
                    strcpy(textMessageV, binn_object_str(received_msg, "textMessage"));
                    strcpy(nameFileV, binn_object_str(received_msg, "nameFile"));
                    strcpy(viewMessageV, binn_object_str(received_msg, "viewMessage"));
                    strcpy(checkSumV, binn_object_str(received_msg, "checkSum"));
                    strcpy(creationMessageV, binn_object_str(received_msg, "creationMessage"));

                    
                    sprintf(query, checkMessageVDR, atoi(idMessageV));
                    if (mysql_query(con, query)){
                        printf("\033[31m[SERVER] Error. Try later...");
                        printf("\033[37m\n");
                        pthread_mutex_unlock(&lock3);
                        continue;
                    }

                    result = mysql_store_result (con);
                    int n_rows = mysql_num_rows (result);
                    if (n_rows == 0) {
                        // Insert
                        sprintf(query, insertMessageVDR, atoi(idMessageV), atoi(idDetailsV), textMessageV, nameFileV, atoi(viewMessageV), checkSumV, creationMessageV);
                        if (mysql_query(con, query))                   
                        {          
                            printf("\033[31m[SERVER] Error. Try later...");
                        }
                        else
                        {   
                            if (strcmp(nameFileV, "0") != 0) {
                              
                            }

                            printf("\033[33m[SYNC] The details message with id %d has been registered...", atoi(idMessageV));
                        }

                    }
                    else {
                        field = mysql_fetch_field (result);
                        row = mysql_fetch_row (result); 

                        if ((strcmp(idDetailsV, row[1])!=0) || (strcmp(textMessageV, row[2])!=0) || (strcmp(nameFileV, row[3])!=0) || (strcmp(viewMessageV, row[4])!=0) || (strcmp(checkSumV, row[5])!=0)){
                            // UPDATE
                            sprintf(query, updateMessageVDR, atoi(idDetailsV), textMessageV, nameFileV, atoi(viewMessageV), checkSumV, atoi(idMessageV));
                            if (mysql_query(con, query))                   
                            {          
                                printf("\033[31m[SERVER] Error. Try later...");                    
                            }
                            else
                            {
                                printf("\033[33m[SYNC] The details message with id %d has been updated...", atoi(idMessageV));
                            }
                            
                        }
                        else {
                            printf("\033[32m[SYNC] The details message with id %d is correct...", atoi(idMessageV));
                        }

                    }


                    mysql_free_result (result);
                    memset(&idMessageV[0], 0, sizeof(idMessageV));
                    memset(&idDetailsV[0], 0, sizeof(idDetailsV));
                    memset(&textMessageV[0], 0, sizeof(textMessageV));
                    memset(&nameFileV[0], 0, sizeof(nameFileV));
                    memset(&viewMessageV[0], 0, sizeof(viewMessageV));
                    memset(&checkSumV[0], 0, sizeof(checkSumV));
                    memset(&creationMessageV[0], 0, sizeof(creationMessageV));
                    
                    printf("\033[37m\n");
                }

                pthread_mutex_unlock(&lock3);
                memset(&table[0], 0, sizeof(table));
                          

            }
            else {
                printf("%d", strlen(received_msg));
                puts(received_msg);
                puts("\033[37m");
            }
        }
    }

}


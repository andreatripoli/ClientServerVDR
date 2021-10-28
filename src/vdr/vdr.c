#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <mysql.h>
#include <binn.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "constant.h"
#include "utils.h"


// Variables
FILE *file;
char *ipAddress = NULL;
char *portAddress =  NULL;
char *portDB = NULL;
char *dbFlags =  NULL;
char *dbHost =  NULL;
char *dbUser =  NULL;
char *dbName =  NULL;
char *dbPasswd =  NULL;

char line[RSIZ][LSIZ];
int i = 0;
int tot = 0;

int client_counter = 0; // Counting the clients in the server at the same time.

// Initialize Database
MYSQL *con;
MYSQL_ROW row;
MYSQL_RES *result;

// Query
char querySelectUser[] = "SELECT * FROM user\n";
char querySelectBookAddress[]= "SELECT * FROM addressBook\n";
char querySelectDetails[]= "SELECT * FROM detailsBook\n";
char queryMessage[]= "SELECT * FROM detailsMessage\n";


// Initialize server, socket variables
int socket_desc, new_socket, c, *new_sock, clientSocket;
struct sockaddr_in server, client, clientServer;

pthread_mutex_t lock;

// Functions
int isValidIpAddress(char *ipAddress);
void *connection_handler(void *socket_desc);
int isNumber(char s[]);


typedef struct {
    char id[10];
	char nome[30];
	char cognome[30];
	char username[30];
	char password[50];
	char data[25];
}dataUser;


/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    signal(SIGINT, sigintHandler);
    fflush(stdout);
    puts("\033[33m\n[INFO] Closing in progress...");
    sleep(2);
    exit(1);
}

// Signal Handler for SIGTSTP
void sighandler(int sig_num)
{
    signal(SIGTSTP, sighandler);
    fflush(stdout);
    puts("\033[33m\n[INFO] Closing in progress...");
    sleep(2);
    exit(1);
}


int main(int argc, char *argv[]) {

    // Initialize the signals 
    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, sighandler);

    // Initialize the mutex 
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        puts("\033[31m[INFO] Mutex init failed...");
        return 1;
    }

    puts("\033[37m[INFO] VDR");
    puts("");

    puts("\033[37m[VDR] Reading the configuration file...");

    // Extract the configuration data from the file 
    if((file = fopen("property.conf","r"))!=NULL){
            // file exists
            while(fgets(line[i], LSIZ, file)){
                line[i][strlen(line[i]) - 1] = '\0';
                i++;
            }

            tot = i;

            for(i = 0; i < tot; ++i){
                char *token = strtok(line[i], "=");
                while (token != NULL){   
                    if (((strcmp(token, "PORT")) != 0) && ((strcmp(token, "IP")) != 0) 
                        && ((strcmp(token, "HOST")) != 0)&& ((strcmp(token, "USER")) != 0)&& ((strcmp(token, "PASSWD")) != 0)&& ((strcmp(token, "DB")) != 0)
                        && ((strcmp(token, "DBPORT")) != 0)&& ((strcmp(token, "FLAGS")) != 0)) {
                        

                        if (isValidIpAddress(token) == 1) {
                            if (ipAddress == NULL) {
                                ipAddress = token;
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
        puts("\033[31m[VDR] Configuration file not found...");
        return 1;
    }


    // Incorrect data
    if ((ipAddress == NULL) || (portAddress == NULL) || (dbHost == NULL)|| (dbUser == NULL)
            || (dbPasswd == NULL)|| (dbName == NULL)|| (portDB == NULL)|| (dbFlags == NULL)) {
        puts("\033[31m[VDR] Set the configuration in the property file...");
        return 1;
    }


    // Print configuration file
    puts("");
    puts("\033[37mVDR Configuration");

    printf("\033[36mIP Server: %s\n", ipAddress);
    printf("\033[36mPort: %s\n", portAddress);

    printf("\033[37mDatabase Configuration\n");

    printf("\033[36mHost: %s\n", dbHost);
    printf("\033[36mUser: %s\n", dbUser);
    printf("\033[36mPassword: %s\n", dbPasswd);
    printf("\033[36mDatabase: %s\n", dbName);
    printf("\033[36mPort: %s\n", portDB);
    printf("\033[36mFlags: %s\n\n", dbFlags);

    con = mysql_init(NULL);
    if (!mysql_real_connect(con, dbHost, dbUser, dbPasswd, dbName, atoi(portDB), NULL, atoi(dbFlags))) {
		puts("\033[31m[VDR] Unable to connect to the database...");
		return 1;
	}

    puts("\033[37m[VDR] Successful connection to database...");
    


    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    // Unable to create socket
    if (socket_desc == -1) {
        puts("\033[31m[VDR] Could not create socket...");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ipAddress);
    server.sin_port = htons(atoi(portAddress)); 

    // If the process is not busy, try again 
    do {
        puts("\033[31m[VDR] Busy process. New attempt in 5 seconds...");
        sleep(5);
    } while (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0);



    puts("\033[37m[VDR] Process started...\n");


    listen(socket_desc, 30);

    puts("\033[37m[VDR] Waiting for incoming connection...");
    c = sizeof(struct sockaddr_in);

    // Threads
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))){

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) new_sock) < 0) {
            puts("\033[31m[VDR] Could not create thread...");

            return 1;
        }
    }

    close(socket_desc);
    return 0;

}


// Thread to manage communication with the server
void *connection_handler(void *socket_desc)
{
    char client_input[500];  //received input
    char client_info[200]; //sended output
    char client_output[1000]; //sended output
    // Get the socket descriptor
    int sock = *((int *)socket_desc);

    while (recv(sock, client_input, sizeof(client_input), 0) > 0){
        pthread_mutex_lock(&lock);

        // The server has requested synchronization.
        // The VDR sends each record to the server to check 
        // if the server has the correct data 
        if (strcmp(client_input, "-sync\n") == 0){ 

            if (mysql_query(con, querySelectUser))
            {
                printf("\033[31m[VDR] Error. Try later...\n");
                pthread_mutex_unlock(&lock);
                break;
            }

            result = mysql_store_result(con);
            int num_fields = mysql_num_fields(result);


            while ((row = mysql_fetch_row(result))) {
                binn *obj  = binn_object();
                binn_object_set_str(obj, "table", "user");
                binn_object_set_str(obj, "id", row[0]);
                binn_object_set_str(obj, "name", row[1]);
                binn_object_set_str(obj, "surname", row[2]);
                binn_object_set_str(obj, "username", row[3]);
                binn_object_set_str(obj, "password", row[4]);
                binn_object_set_str(obj, "data", row[5]);

                send(sock, binn_ptr(obj), binn_size(obj), 0);

                binn_free(obj);
                sleep(0.1);
            }

            mysql_free_result(result);
            sleep(1);

            if (mysql_query(con, querySelectBookAddress))
            {
                printf("\033[31m[VDR] Error. Try later...\n");
                pthread_mutex_unlock(&lock);
                break;
            }

            result = mysql_store_result(con);
            num_fields = mysql_num_fields(result);


            while ((row = mysql_fetch_row(result))) {
                binn *obj  = binn_object();
                binn_object_set_str(obj, "table", "addressBook");
                binn_object_set_str(obj, "idAddress", row[0]);
                binn_object_set_str(obj, "idCreator", row[1]);
                binn_object_set_str(obj, "creationBook", row[2]);


                send(sock, binn_ptr(obj), binn_size(obj), 0);

                binn_free(obj);
                sleep(0.1);
            }

            mysql_free_result(result);
            sleep(1);

            if (mysql_query(con, querySelectDetails))
            {
                printf("\033[31m[VDR] Error. Try later...\n");
                pthread_mutex_unlock(&lock);
                break;
            }

            result = mysql_store_result(con);
            num_fields = mysql_num_fields(result);


            while ((row = mysql_fetch_row(result))) {
                binn *obj  = binn_object();
                binn_object_set_str(obj, "table", "detailsBook");
                binn_object_set_str(obj, "idDetailsBook", row[0]);
                binn_object_set_str(obj, "idBook", row[1]);
                binn_object_set_str(obj, "idPerson", row[2]);
                binn_object_set_str(obj, "creationDetails", row[3]);

                send(sock, binn_ptr(obj), binn_size(obj), 0);

                binn_free(obj);
                sleep(0.1);
            }

            mysql_free_result(result);
            sleep(1);

            if (mysql_query(con, queryMessage))
            {
                strcpy(client_output, "\033[31m[VDR] Error. Try later...");
                write(sock, client_output, strlen(client_output) + 1);
                pthread_mutex_unlock(&lock);
                break;
            }

            result = mysql_store_result(con);
            num_fields = mysql_num_fields(result);


            while ((row = mysql_fetch_row(result))) {
                binn *obj  = binn_object();
                binn_object_set_str(obj, "table", "detailsMessage");
                binn_object_set_str(obj, "idMessage", row[0]);
                binn_object_set_str(obj, "idDetails", row[1]);
                binn_object_set_str(obj, "textMessage", row[2]);
                binn_object_set_str(obj, "nameFile", row[3]);
                binn_object_set_str(obj, "viewMessage", row[4]);
                binn_object_set_str(obj, "checkSum", row[5]);
                binn_object_set_str(obj, "creationMessage", row[6]);

                send(sock, binn_ptr(obj), binn_size(obj), 0);

                binn_free(obj);
                sleep(0.1);
            }




            mysql_free_result(result);

        }

        // If the server does not want to synchronize
        else{

            char *query = NULL;
            query = client_input;

            if (strcmp(client_input, "") == 0){
                pthread_mutex_unlock(&lock);
                continue;
            }

            // The server requests to store data
            if (strcmp(binn_object_str(client_input, "query"), "") != 0) {

                // The server requests to delete
                if (strcmp(binn_object_str(client_input, "flag"), "delete") == 0){
                    if (mysql_query(con, binn_object_str(client_input, "query"))){
                        printf("\033[31m[SERVER] Error...\n");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else {
                        if (remove(binn_object_str(client_input, "file")) != 0) {
                            //puts("\033[33m[VDR] No file...");
                        } 
                        puts("\033[34m[VDR] Deleted data...\n");
                    }
                }

                // The server requests to send
                if (strcmp(binn_object_str(client_input, "flag"), "send") == 0){
                    if (mysql_query(con, binn_object_str(client_input, "query"))){
                        printf("\033[31m[SERVER] Error...\n");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else {

                        int fd;
                        long int n, m,count=0;
                        char buffer[BUFFERT],filename[256];

                        int n_rows;
                        
                        if ((fd=open(binn_object_str(client_input, "file"),O_CREAT|O_WRONLY,0600))==-1)
                        {
                            pthread_mutex_unlock(&lock);
                            continue;
                        }

                        bzero(buffer,BUFFERT);
                        while ((n = recv(sock, buffer, BUFFERT, 0)) > 0) { 

                            if (strcmp(buffer, "q") == 0)    {
                                pthread_mutex_unlock(&lock);
                                break;
                            }

                            if((m=write(fd,buffer,n))==-1){
                                pthread_mutex_unlock(&lock);
                                break;
                            }
                            count=count+m;
                            bzero(buffer,BUFFERT);
                        }

                        close(fd);

                        puts("\033[34m[VDR] Stored data...\n");
                    }
                }
                else {
                    if (mysql_query(con, binn_object_str(client_input, "query"))){
                        printf("\033[31m[SERVER] Error...\n");
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else {
                        puts("\033[34m[VDR] Stored data...\n");
                    }
                }
            }
            else {
            }

        }

        pthread_mutex_unlock(&lock);
    }

    free(socket_desc); //Free the socket pointer
    return 0;
}
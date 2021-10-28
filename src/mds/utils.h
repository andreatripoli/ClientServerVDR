char buf[SHA_DIGEST_LENGTH*2];

// Client struct that keeps information of clients
typedef struct node {
    int clientid;
    char username[50];
    int access;
    int client_socket_desc;
    struct node * next;
    //bool isOnline;
} clientStruct;

extern clientStruct * listClient;

// Check is a valid address
int isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    
    return result;
}

// Check is a number
int isNumber(char s[])
{
    for (int i = 0; s[i]!= '\0'; i++){
        if (isdigit(s[i]) == 0){
              return 0;
        }
    }
    return 1;
}

// Extract user from list
int pop(clientStruct ** head) {
    int retval = -1;
    clientStruct * next_node = NULL;

    if (*head == NULL) {
        return -1;
    }

    next_node = (*head)->next;
    retval = (*head)->clientid;
    free(*head);
    *head = next_node;

    return retval;
}

// Insert user into list
void push(clientStruct * head, int id, int sock, int stamp, char *user) {
    clientStruct * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = (clientStruct *) malloc(sizeof(clientStruct));
    current->next->clientid = id;
    strcpy(current->next->username, user);
    current->next->client_socket_desc = sock;
    current->next->access = stamp;
    current->next->next = NULL;
}


int remove_by_value(clientStruct ** head, int val) {
    clientStruct *previous, *current;

    if (*head == NULL) {
        return -1;
    }

    if ((*head)->clientid == val) {
        return pop(head);
    }

    previous = *head;
    current = (*head)->next;
    while (current) {
        if (current->clientid == val) {
            previous->next = current->next;
            free(current);
            return val;
        }

        previous = current;
        current  = current->next;
    }
    return -1;
}

// Delete the list
void delete_list(clientStruct *head) {
    clientStruct  *current = head, 
            *next = head;

    while (current) {
        next = current->next;
        free(current);
        current = next;
    }
}

// Print a list
void print_list(clientStruct * head) {
    clientStruct * current = head;

    while (current != NULL) {
        if ((current->clientid != 0) && (current->client_socket_desc !=0)) {
            printf("%s\n", current->username);
        }
        current = current->next;
    }
}

// Function to check users online
void close_list_socket(clientStruct * head, sem_t sem) {
    clientStruct * current = head;

    while (current != NULL) {
        if ((current->clientid != 0) && (current->client_socket_desc !=0)) {

            int nread = send(current->client_socket_desc,"",strlen("")+1,0);
            if(nread<0){
                char name[50];
                strcpy(name, current->username);

                close(current->client_socket_desc);
                remove_by_value(&head, current->clientid);
                printf("\033[37m[SERVER] User %s socket has been closed...\n", name);
                sem_wait(&sem);
            } 
        }
        current = current->next;
    }
}


// Convert from string to SHA1
char* convertToSHA1(char* textFile) {
 
   int i = 0;
   unsigned char temp[SHA_DIGEST_LENGTH];

   memset(buf, 0x0, SHA_DIGEST_LENGTH*2);
   memset(temp, 0x0, SHA_DIGEST_LENGTH);

   SHA1((unsigned char *)textFile, strlen(textFile), temp);

   for (i=0; i < SHA_DIGEST_LENGTH; i++) {
      sprintf((char*)&(buf[i*2]), "%02x", temp[i]);
   }

   return buf;
 
}


// Convert from FILE to SHA1
char* fileToSHA1(char* FILES) {

    unsigned char hash[SHA_DIGEST_LENGTH];
    int i;
    FILE *f = fopen(FILES,"rb");
    SHA_CTX mdContent;
    int bytes;
    unsigned char data[1024];

    if(f == NULL){
        printf("\033[31m[SERVER] %s couldn't open file\n", FILES);
        exit(1);
    }

    SHA1_Init(&mdContent);
    while((bytes = fread(data, 1, 1024, f)) != 0){

        SHA1_Update(&mdContent, data, bytes);
    }

    SHA1_Final(hash,&mdContent);

    /** if you want to see the plain text of the hash */
    for(i=0; i < SHA_DIGEST_LENGTH;i++){
        sprintf((char *)&(buf[i*2]), "%02x",hash[i]);
    }


    fclose(f);

    return buf;
 
}


// Generate random string
static char *rand_string(char *str, size_t size)
{
    srand(time(NULL));
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKNFOPQRSTUVZ123456789";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}


// Duplicate a file
int duplicate(char *src, char *dst)
{
    FILE *original,*copy;
    int c;
    original=fopen(src,"r");
    copy=fopen(dst,"w");

    if( !original || !copy)
    {
        puts("\033[31m[SERVER] File error!");
        return 1;
    }

    while( (c=fgetc(original)) != EOF)
        fputc(c,copy);

    fclose(original);
    fclose(copy);

    return 0;
}



// Send a file
void sendFile(char *namePath, int client_sock_desc, struct sockaddr_in client) {
    struct timeval start, stop, delta;
    char buf[BUFFERT];
    off_t count=0, m,sz;
	long int n;
    int sfd,fd;
    int l=sizeof(struct sockaddr_in);
    struct stat buffer;
    


    if ((fd = open(namePath,O_RDONLY))==-1){
        puts("\033[31m[INFO] Open fail...");
		return;
	}

    //taille du fichier
	if (stat(namePath,&buffer)==-1){
		puts("\033[31m[INFO] Stat fail...");
		return;
	}
	else
		sz=buffer.st_size;


    bzero(&buf,BUFFERT);

    gettimeofday(&start,NULL);
    n=read(fd,buf,BUFFERT);
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
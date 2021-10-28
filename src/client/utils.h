char buf[SHA_DIGEST_LENGTH*2];

// Check is a valid address
int isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    
    return result;
}

// Generate a random string
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

// Check is a valid number
int isNumber(char s[])
{
    for (int i = 0; s[i]!= '\0'; i++){
        if (isdigit(s[i]) == 0){
              return 0;
        }
    }
    return 1;
}

int set_disp_mode(int fd,int option)  
{  
   int err;  
   struct termios term;  
   if(tcgetattr(fd,&term)==-1){  
   
     puts("\033[37m[INFO] Cannot set the attribution of the terminal...");
     return 1;  
   }  
   if(option)  
        term.c_lflag|=ECHOFLAGS;  
   else  
        term.c_lflag &=~ECHOFLAGS;  
   err=tcsetattr(fd,TCSAFLUSH,&term);  
   if(err==-1 && err==EINTR){  
        puts("\033[37m[INFO] Cannot set the attribution of the terminal...");
        return 1;  
   }  
   return 0;  
}  



// Converto from string to SHA1
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




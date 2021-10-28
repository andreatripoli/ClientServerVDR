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




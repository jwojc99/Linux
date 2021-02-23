#include "utils.h"
int ch_to_int(const char *ch){
	//wlasna atoi
    int j =0;
    while(j<strlen(ch)){
				if(isdigit(ch[j]) == 0)
					LogError(strcat(ch," nie jest liczba!"));
				j++;
    }
    return strtol(ch,0,0);
}
char * int_to_ch(int v){
	char str[12];
	exit(-1);
	sprintf(str, "%d", v);
	return &str;
}
void LogWarning(const char *message){
    puts(message);
}
void LogError(const char *message)
{
	perror(message);
	exit(1);
}


//return 1 when 1 is later(bigger) than b
int time_compare(struct timespec const * const a, struct timespec const * const b)
{
	// if( a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec)
	// 	return 0;

	if(a->tv_sec > b->tv_sec){
		return 1;
	}
	else if (a->tv_sec == b->tv_sec){
		if(a->tv_nsec > b->tv_nsec){
			return 1;
		}
		else if(a->tv_nsec == b->tv_nsec){
			 return 0;
			 }
		else{
			return -1;
		}
	}else{
	 return -1;
	}
}
struct timespec float_to_timespec(float interval){
int sec = 0;
	struct timespec retInt;
	
	if(interval<=0)
	 LogError("Zly interwal");
	while(interval>1){
		interval--;
		sec++;
	}
	retInt.tv_nsec = interval * 1000000000;
	retInt.tv_sec = sec;
	return retInt;
}
void PobierzAdresPort(int polaczenieFD,char *buforAdresu,char *buforPort){
	  	char bufor[INET_ADDRSTRLEN] = "";
	//  char buforPort[12] ="";

		struct sockaddr_in addrSocket;
		socklen_t addrLen = sizeof(addrSocket);
		if(getsockname(polaczenieFD,(struct sockaddr*)&addrSocket,&addrLen) !=0 )
		 	LogError("getsockname ma blad");
		else
			inet_ntop(AF_INET,&addrSocket.sin_addr,bufor,sizeof(bufor));
		
		uint16_t prezentation_port = ntohs(addrSocket.sin_port);
		sprintf(buforPort, "%d", prezentation_port);
		sprintf(buforAdresu,"%s",bufor);
		
}
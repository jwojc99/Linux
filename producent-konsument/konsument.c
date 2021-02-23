#include <pthread.h>
#include "utils.h"

#define MAX_TEMPO_KONSUMPCJI 1024  //potrzebne?
#define MAX_TEMPO_DEGRADACJI 1024  //?
#define INTERWAL_DEGRADACJI 1
// STALE
#define STALA_KONSUMPCJI 4435.f
#define STALA_DEGRADACJI 819.f
#define BLOK_NA_MAGAZYN 30*1024
//////// Globalne zmienne
int koniec = 0;
//Dot.Magazynu
pthread_mutex_t MagMutex = PTHREAD_MUTEX_INITIALIZER;
int obecna_pojemnosc_magazynuK=0;
int pojemnosc_magazynuK=0;
//Raport
int RaportPipe[2];

void PrintRaport(){
	
	int zd = -1;

	do{
		int wlk_buf = 1000;
		char chunk[wlk_buf];
		memset(&chunk,'\0',sizeof(chunk));
		zd = read(RaportPipe[0],&chunk,wlk_buf);
		printf("%s",chunk);
	}while(zd >0);
}
void WriteRaport(char *msg){
	int length = strlen(msg);
	int result = write(RaportPipe[1],msg,length);
}


int CzyMamyMiejsceWMagazynie(){
	int ret = 0;
	pthread_mutex_lock(&MagMutex);
		if(pojemnosc_magazynuK - obecna_pojemnosc_magazynuK >= WIELKOSC_PRODUKTU){
			ret = 1;
		}
	pthread_mutex_unlock(&MagMutex);
	return ret;
}

void DegradujMagazyn(int ile_zdegradowac){
	pthread_mutex_lock(&MagMutex);
        obecna_pojemnosc_magazynuK-=ile_zdegradowac;
		if(obecna_pojemnosc_magazynuK<0){
			obecna_pojemnosc_magazynuK = 0;
        }
	pthread_mutex_unlock(&MagMutex);
}
// Funkcja uruchomiona w drugim watku
// arg - interwal w sekundach, co jaki ma degradowac
void Degraduj(void *arg)
{
	//arg - > tempo (param -d * stala)
	float *tempo = arg;
	
	float interwal = INTERWAL_DEGRADACJI;
	int ile_zdegradowac = (int)(interwal *(*tempo));

	struct timespec last_deg;
	struct timespec current_time;
	struct timespec delta;
	clock_gettime(CLOCK_MONOTONIC,&last_deg);
	struct timespec interval = float_to_timespec(interwal);
    while(!koniec){

        clock_gettime(CLOCK_MONOTONIC,&current_time);
        timespec_diff_macro(&current_time,&last_deg,&delta);

        if(time_compare(&delta,&interval)>=0){

            DegradujMagazyn(ile_zdegradowac);
            clock_gettime(CLOCK_MONOTONIC,&last_deg);
        }
    }
}

void Konsumuj(float interval){

    struct timespec interwal_konsumpcji;	
	struct timespec last_prod;
	struct timespec current_time;
	struct timespec delta;
    clock_gettime(CLOCK_MONOTONIC,&last_prod);
	interwal_konsumpcji = float_to_timespec(interval);
			
    while(true){

        clock_gettime(CLOCK_MONOTONIC,&current_time);
        timespec_diff_macro(&current_time,&last_prod,&delta);

        if(time_compare(&delta,&interwal_konsumpcji)>=0){
            break;
        }
        
    }
    
}

int main(int argc, char* argv[])
{
    //Dot. Tempa
    float tempo_konsumpcji=-1.0f;
    float tempo_degradacji=-1.0f;
    //Dot. Polaczenia
	char *nazwa_producenta="localhost";
	int port_producenta = 0;
    //Dot. Parametrow
	char *ostatni_parametr = argv[optind];
	char *colon_index = NULL;
	int index_colon = 0;
    //WatekDegradacji
    pthread_t watek_degradacji;
    //Do oblusgi polaczen
    struct sockaddr_in adres_serwera;
    //Inne
    char buforAdresu[INET_ADDRSTRLEN] = "";
    char buforPort[12] = "";
    char buforPID[12] = "";
    int pipe_flags = 0; //Flagi pipe
    int x = 0; // getopt return

	while((x=getopt(argc,argv,"c:p:d:")) != -1)
	{
		switch(x)
		{
			case 'c':{
				int pValue = strtod(optarg, NULL);
				pojemnosc_magazynuK=pValue*BLOK_NA_MAGAZYN;
				break;
			}
			case 'p':{
				float pValue = strtof(optarg, NULL);
				if(pValue <0 || pValue > MAX_TEMPO_KONSUMPCJI)
					LogError("Zla wartosc parametru p (Max 1024)!");
				tempo_konsumpcji=(STALA_KONSUMPCJI)*pValue; // ile bajtow na sek
				break;
			}
			case 'd':{
				float pValue = strtof(optarg, NULL);
				if(pValue <0 || pValue > MAX_TEMPO_DEGRADACJI)
					LogError("Zla wartosc parametru d!(Max 1024)");
				tempo_degradacji=(STALA_DEGRADACJI)*pValue; // ile bajtow na sek
				break;
			}
		}
	}
	ostatni_parametr = argv[optind];
	// 3 PARAMETR
	if(ostatni_parametr == NULL){
		LogWarning("Parametr port jest wymagany");
        exit(-1);
	}
	colon_index=strchr(ostatni_parametr,':');
	if(colon_index!=NULL){//czyli zostal podany addres
		strncpy(nazwa_producenta,ostatni_parametr,colon_index);
		char portChar[5];
		strncpy(portChar,ostatni_parametr+index_colon+1,strlen(ostatni_parametr)-(colon_index-ostatni_parametr));
		puts(portChar);
		port_producenta = ch_to_int(portChar);
	}
    else{//nie ma podanego adresu
		port_producenta=ch_to_int(ostatni_parametr);
	}
    //Pobieranie PID
    sprintf(&buforPID,"%d",getpid());

    //Utowrzenie pipe dla Raportu
	if(pipe(&RaportPipe)<0){
        LogError("Blad tworzenia Pipe dla raportu");
    }
    pipe_flags = fcntl(RaportPipe[0],F_GETFL);
	pipe_flags |= O_NONBLOCK;
	if(fcntl(RaportPipe[0],F_SETFL,pipe_flags)){
        LogError("Blad ustawiania flag");
    } 
    //Utworzenie watku degradacji
    pthread_create(&watek_degradacji,NULL,Degraduj,&tempo_degradacji);

//Kod laczenia do serwera:
	adres_serwera.sin_family = AF_INET;
	adres_serwera.sin_port=htons(port_producenta);
	if(inet_aton(nazwa_producenta,&adres_serwera)==-1){
         LogError("Bledny adress");
    }
        
	
	// gdyby byla domenta to:
	struct hostent * serwer = gethostname(nazwa_producenta,NULL);
	
	if(serwer == NULL) 
		LogError("gerthostane fail");
	do{

		int gniazdo = socket(AF_INET,SOCK_STREAM,0);
		if(gniazdo<0) LogError("Blad tworzenia gniazda");

	
		
		int connreciv = connect(gniazdo,&adres_serwera,sizeof(adres_serwera));
		if(connreciv == -1){
			LogError("Blad laczenia");
			close(gniazdo);
		}
		PobierzAdresPort(gniazdo,buforAdresu,buforPort);
		char ident[80] = {'\0'};
		sprintf(ident,"Konsument(%s)ip:%s:%s\n",buforPID,buforAdresu,buforPort);
		WriteRaport(ident);
		if(connreciv == 0) // mamy polaczenie!
		{
	        //pobierz czas do obliczenia opoznienia(M) - polaczenie
			clock_gettime(CLOCK_MONOTONIC, &monotime_conn);
			char bufor[4096];
			int r = -1;
			int restetuj_zegar = 1;
			do{
				memset(&bufor,'\0',4096);
				r = recv(gniazdo,&bufor,4096,0);
		
				if(restetuj_zegar == 1){
				    //stad pobieramy czas monotonic aby wyliczyc opoznienia(M)
					clock_gettime(CLOCK_MONOTONIC, &monotime_recv);
					timespec_diff_macro(&monotime_recv,&monotime_conn,&monotime_delta);
					char b[80];
					sprintf(&b,"Opoznienie miedzy polaczeniem a 1 porcja: %d.%d\n",monotime_delta.tv_sec,monotime_delta.tv_nsec);
					WriteRaport(&b);
                    //Pobieramy to tylko raz per polaczenie
					restetuj_zegar = 0;
				}
				if(r<0)
				{
                    LogError("Blad recv");
                    close(gniazdo);
				} 
					
				else if( r == 0){ // Zostalismy rozlaczeni
			
					//pobierz czas zamniecia i porownaj z czasem pierwsze porcji i dodaj wynu do bufora raportu
					clock_gettime(CLOCK_MONOTONIC, &monotime_close);
					timespec_diff_macro(&monotime_close,&monotime_recv,&monotime_delta);
					char b[80]="";
					sprintf(&b,"czas miedzy pierwsza porcja a zamknieciem: %d.%ld\n",monotime_delta.tv_sec,monotime_delta.tv_nsec);
					WriteRaport(b);
					
					close(gniazdo);
					
				}
				else{ // Otrzymalismy dane

					Konsumuj(r/tempo_konsumpcji);
					pthread_mutex_lock(&MagMutex);
						obecna_pojemnosc_magazynuK+=r;
					pthread_mutex_unlock(&MagMutex);
				}
			}while(r>0); // dopoki mamy dane
		}
	}while(CzyMamyMiejsceWMagazynie()>0);
	
	clock_gettime(CLOCK_REALTIME, &realtime);
    char *real_czas = ctime(&realtime);
    real_czas[strlen(real_czas)-1]='\0';
	printf("TS: %s\n",real_czas);

	pthread_cancel(watek_degradacji);
	koniec = 1;
	PrintRaport();
	close(RaportPipe[0]);
    close(RaportPipe[1]);
	return (0);
}


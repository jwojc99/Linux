#define _GNU_SOURCE 

#include <fcntl.h>
#include <sys/ioctl.h>
#include "serwer.h"
#include "utils.h"

przygotowanySocket = false;


int PobierzPojemnoscMagazynu(){
    pojemnosc_magazynu = fcntl(MagPipe[0], F_GETPIPE_SZ);
	return pojemnosc_magazynu;
}

int PobierzAktualnyStanMagazynu(){
    int as = -1;
    ioctl(MagPipe[1], FIONREAD, &as);
    aktualny_stan_magazynu = as;
    return as;
}

struct sockaddr_in PrzygotujAdres(const char* addr,int port){
    struct sockaddr_in address;
	//type of created socket
	address.sin_family=AF_INET;
    if(strcmp(addr,"all")==0){
        address.sin_addr.s_addr=INADDR_ANY;
        
    }else if(strcmp(addr,"localhost")==0){
        address.sin_addr.s_addr=inet_addr("127.0.0.1");
    }
    else{
	    address.sin_addr.s_addr=inet_addr(addr);
    }
	address.sin_port=htons(port); // host_to_network

    return address;
}

void PrzygotujSocket(int *socketSerwera){
    if(!przygotowanySocket){
        //utworz socket
        *socketSerwera=socket(AF_INET, SOCK_STREAM, 0);
        if(*socketSerwera<=0){
		    LogError("Blad tworzenia socketSerwera: ");
        }
        int on = 1;
	    //ustaw socket serwera na przyjmowanie wielu polaczen(ponowne wykorzystanie socketa)
	    if(setsockopt(*socketSerwera, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on))<0){
		    LogError("Blad ustawiania flag socketSerwera:");
        }
        if(ioctl(*socketSerwera, FIONBIO, (char *)&on) <0){
            LogError("Blad ustawiania nieblokujacego");
        }
        
        przygotowanySocket = true;
    }    	
}

void PrzygotujSerwer(char *hostname,int port_no)
{
    InformacjeKlientow = (struct KlientInfo*)calloc(MAX_ILOSC_KLIENTOW+1,sizeof(struct KlientInfo));
    TablicaPolla = (struct pollfd*)calloc(MAX_ILOSC_KLIENTOW+1,sizeof(struct pollfd));

	InformacjeKlientow[0].adres = PrzygotujAdres(hostname,port_no);
    PrzygotujSocket(&InformacjeKlientow[0].klient_socket);

	if(bind(InformacjeKlientow[0].klient_socket,(struct sockaddr*)&InformacjeKlientow[0].adres, sizeof(struct sockaddr_in))<0){
		LogError("Error bindsocket");
    }
	printf("Uruchamiam serwer na porcie: %d\n", ntohs(InformacjeKlientow[0].adres.sin_port));

    // allows the server to accept incoming client connections
	if(listen(InformacjeKlientow[0].klient_socket,MAX_REQ_CON)<0){
		LogError("Error listen");
    }
    //inicjalizaja struktor poll fd
    struct pollfd * fdSrv = &TablicaPolla[0];
    fdSrv->fd = InformacjeKlientow[0].klient_socket;
    fdSrv->events = POLLIN | POLLOUT;
    StworzDeskryptoryRaportu();
    TablicaPolla[1].fd = raportFD;
    TablicaPolla[1].events = POLLIN;

}

void StworzDeskryptoryRaportu(){
    int interwal = INTERWAL_RAPORTU;
    raportFD = timerfd_create( CLOCK_REALTIME , TFD_NONBLOCK);
    if( raportFD == -1 ){
        LogError("Blad tworzenia timera raportu");
    }
    struct itimerspec rtimerspec;
    rtimerspec.it_interval.tv_sec = (int)interwal;
    rtimerspec.it_interval.tv_nsec = (interwal - (int)interwal) * 1000000000;

    rtimerspec.it_value.tv_sec = (int)interwal;
    rtimerspec.it_value.tv_nsec = (interwal - (int)interwal) * 1000000000;

    if( timerfd_settime( raportFD, 0, &rtimerspec, NULL)<0)
	 LogError("Blad settime raportu");
}

void WypiszRaport(){
    static int stan_sprzed_5_sekund = 0;
    clock_gettime(CLOCK_REALTIME, &realtime);
    char *real_czas = ctime(&realtime);
    real_czas[strlen(real_czas)-1]='\0';
    pojemnosc_magazynu = PobierzPojemnoscMagazynu(); 
    int akt_stan = PobierzAktualnyStanMagazynu();
    float dt = (float) akt_stan;
    int przeplyw=akt_stan-stan_sprzed_5_sekund;
    int ile_wydalem = 0;
    dt/=pojemnosc_magazynu;

    printf("Raport z %.19s: ",real_czas);
    printf("klientow:%d ",iluKlientowPodlaczonych);
    printf("[Magazyn:%d |%2f%%] ",akt_stan,dt*100);
    printf("Przeplyw:%d \n", przeplyw);

    stan_sprzed_5_sekund = akt_stan;
}

void NowyKlient(int new_fd,struct sockaddr_in adres,socklen_t addLen)
{ 
    struct KlientInfo *nowyKlient = NULL;
     
    for(int i=1;i<MAX_ILOSC_KLIENTOW+1; i++)
	{
		if(InformacjeKlientow[i].klient_socket <=0)
		{
            nowyKlient = &InformacjeKlientow[i];
            memset(nowyKlient,'\0',sizeof(struct KlientInfo));
			break;
		}
	}
    if(nowyKlient != NULL){
        nowyKlient->klient_socket = new_fd;
        nowyKlient->ilosc_wyslanych_bajtow = 0;
        nowyKlient->ilosc_zmarnowanych_bajtow=0;
        nowyKlient->adres = adres;
    }else{
        LogWarning("Nie udalo sie dodac nowego klienta");
    }

    for(int i=2; i<MAX_ILOSC_KLIENTOW+1; i++)
	{
        if(TablicaPolla[i].fd <=0)
		{
			TablicaPolla[i].fd = nowyKlient->klient_socket;// poll_fd;
            TablicaPolla[i].events = POLLIN | POLLOUT;
            TablicaPolla[i].revents = POLLOUT;
            //InformacjeKlientow[i].pollID = i;
			break;
		}
    }
    iluKlientowPodlaczonych++;// aktualizuj TablicePoll
   
}
void NaRozlaczKlienta(struct KlientInfo *klient){
    clock_gettime(CLOCK_REALTIME, &realtime);
    char *real_czas = ctime(&realtime);
    real_czas[strlen(real_czas)-1]='\0';
    
    clock_gettime(CLOCK_REALTIME, &realtime);
    int zmarnowane_bajty = klient->ilosc_zmarnowanych_bajtow;
    
    char * addrS = inet_ntoa(klient->adres.sin_addr);
    uint16_t prezentation_port = ntohs(klient->adres.sin_port);
    char portS[12];
	sprintf(portS, "%d", prezentation_port);
    
    printf("Klient zakonczyl polaczenie o %s adres: %s:%s zmarnowane bajty: %d\n",real_czas,addrS,portS,zmarnowane_bajty);

    klient->klient_socket = -1;
    iluKlientowPodlaczonych-=1;

}
void SprobojWyslacProduktDoKlienta(struct KlientInfo *klient){
    if(klient->ilosc_wyslanych_bajtow>=WIELKOSC_PRODUKTU){
        klient->ilosc_zmarnowanych_bajtow=klient->ilosc_wyslanych_bajtow - WIELKOSC_PRODUKTU;
        shutdown(klient->klient_socket,SHUT_RDWR);
    }else{
        int akt_stan = PobierzAktualnyStanMagazynu();
        if(akt_stan>=WIELKOSC_PRODUKTU){
            char chunk[WIELKOSC_CHUNKA];
            int zd = read(MagPipe[0],chunk,WIELKOSC_CHUNKA);

            if(zd<0) 
                LogError("Blad chunk read");

            int sr =send(klient->klient_socket,&chunk,WIELKOSC_CHUNKA,0);
            
            if (sr<0) 
                LogError("Blad wysylania chunka");

            klient->ilosc_wyslanych_bajtow+=sr;
        }
    }

}

void ReaktorPoll(){
    
    char buffor[101]; // bufor do komunikacji
   
    while(!koniec){
        int aktualizuj_polla = 0;
        int nofd = iluKlientowPodlaczonych+2;
        int active = poll(TablicaPolla,nofd,-1); // -1  bo nie ustawiamy timeoutu
        if(active==-1){
            LogError("Blad poll");
        }
        else if(active == 0){
            LogWarning("Timeout hit");
        }
        else{
            for(int i = 0; i<nofd;i++){
                if(TablicaPolla[i].revents == 0){
                 continue;
                }    
                if(i == 0)
                { // fd serwera
                
                    if( (TablicaPolla[0].revents & POLLIN) )
                    { //dostajemy cos do obsluzenia
                        int new_clients = 0;
                        int last_new_fd = 0;
                            do{
                               
                                struct sockaddr_in adres;
                                socklen_t addLen = sizeof(adres);
                                memset(&adres,'\0',sizeof(struct sockaddr_in));
                                // -1 na blad lub id nowego deskrpyt
                                int new_fd = accept(TablicaPolla[0].fd,&adres,&addLen);
                                if(new_fd>0){ //probujemy podlaczyc
                                    new_clients++;
                                    NowyKlient(new_fd,adres,addLen);//obsluga nowego klienta(dodanie na polke)
                                    
                                }else{
                                    if(new_clients==0){
                                        LogError("Blad accept");
                                    }
                                } 
                                last_new_fd = new_fd;
                                
                            }while(last_new_fd != -1 && new_clients>0);
                      
                    }
                } //fd serwera

                else if(i == 1){ // fd timer raportu
                    if( (TablicaPolla[1].revents & POLLIN) )
                    {
                        read( TablicaPolla[1].fd, buffor, 64);
                        WypiszRaport();
                    }
                }
                else{ // nasz current jest readable
                    struct KlientInfo *klient = NULL; 
                    struct pollfd *curr =&TablicaPolla[i];
                    for(int j = 0;j<MAX_ILOSC_KLIENTOW;j++){
                        if(InformacjeKlientow[j].klient_socket == curr->fd){
                            klient = &InformacjeKlientow[j];
                            break;
                        }
                    }
                    if(klient != NULL){
                        //struct pollfd *curr =&klient->poll_fd;
                        if(curr->revents & POLLIN){
                            //puts("pollin");
                            //czy konczymy tramsi
                            
                            memset(buffor,'\0',100);
                            int read_bytes = 0;
                            //0 - eof, czyli disconenct dla -1 jest blad, a dla > sa dane do odczynata
                            read_bytes = read(curr->fd, buffor, 100);
                            if(read_bytes==0){
                                close(klient->klient_socket);
                                NaRozlaczKlienta(klient);
                                aktualizuj_polla = 1;
                                //printf("Host disconnected, Monotonic time is %s, WallTime is %s, ip %s, port %d, sent blocks: %d \n", ctime(&curtime), ctime(&realtime.tv_sec), inet_ntoa(address.sin_addr), ntohs(address.sin_port), *poped-*popedStart);
                            }else if(read_bytes>0){
                                buffor[read_bytes+1]='\0';
                                printf("Klient wyslal nam: %s\n",buffor);
                            }else
                                LogError("Blad czytania od klienta");
                            }
                            
                        
                        if(curr->revents & POLLOUT){

                            SprobojWyslacProduktDoKlienta(klient);
                        }
                    }else{
                        printf("Nie znalazlem informacji klienta dla fd:%d",curr->fd) ;
                    }
                
                }
            }
        }
        if(aktualizuj_polla){
           aktualizuj_polla = 0;
           for(int i=2;i<MAX_ILOSC_KLIENTOW;i++){
                TablicaPolla[i].fd = 0;
           }
           int idx_polla = 2;
           for(int i=1;i<MAX_ILOSC_KLIENTOW;i++){
               if(InformacjeKlientow[i].klient_socket>0){
                    TablicaPolla[idx_polla].fd = InformacjeKlientow[i].klient_socket;
                    TablicaPolla[idx_polla].events = POLLIN | POLLOUT;
                    TablicaPolla[idx_polla].revents = POLLOUT; 
                    idx_polla++;
               }
           }
           
            
        }
      

    }
    close(InformacjeKlientow[0].klient_socket);
    for(int j = 0;j<MAX_ILOSC_KLIENTOW;j++){
        if(InformacjeKlientow[j].klient_socket >=0){
            close(InformacjeKlientow[j].klient_socket);
        }

    }
	puts("Skonczylem krecic");
}

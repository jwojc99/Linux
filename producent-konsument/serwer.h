#ifndef SRV_H
#define SRV_H

#include "utils.h"

//#define FORCE_NONBLOCK 
#define INTERWAL_RAPORTU 5
#define WIELKOSC_CHUNKA 4*1024
#define MAX_ILOSC_KLIENTOW 256 
//ile polaczen moze czekac na akcpetacje w kolecje przed odrzuceniem polaczenia
#define MAX_REQ_CON 2
#define TIMEOUT (1024 * 1024)
//klienci:
struct KlientInfo{
    int klient_socket;
    struct sockaddr_in adres;
    long ilosc_wyslanych_bajtow;
    long ilosc_zmarnowanych_bajtow;
};
struct KlientInfo *InformacjeKlientow; // dla serwera
//magazyn:
int MagPipe[2]; 
int pojemnosc_magazynu;
int aktualny_stan_magazynu;
unsigned long long IleWygenerowanoBlokow;
//zmienne zwiazane z serwerem: 
int koniec;  // koniec dzialania serwera
int iluKlientowPodlaczonych;
int przygotowanySocket;  // true/false
struct pollfd *TablicaPolla;
//raporty:
int raportFD; // clock fd

int PobierzPojemnoscMagazynu();
int PobierzAktualnyStanMagazynu();

void StworzDeskryptoryRaportu();
void PrzygotujSerwer(char *add,int port);
void ReaktorPoll();

void NowyKlient(int new_fd,struct sockaddr_in adres,socklen_t addLen);
void RozlaczKlienta(int id_klienta);


#endif //SRV_H
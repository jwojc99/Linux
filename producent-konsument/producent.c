#define _GNU_SOURCE 

//Dystrybutor 0 oznacza, ze rodzic ma byc dystrybutorem, inna wartosc, ze dzieckiem
#define DYSTRYBUTOR

#define WIELKOSC_PRODUKTU 13*1024
#define MAX_TEMPO_PRODUKCJI 1024
#define WIELKOSC_BLOKU 640 
#define STALA_PRODUKCJI 2662.f

#include "utils.h"
#include "serwer.h"

//zmienne zwiazane z produkcja
float tempo_produkcji = -1.0f;
timer_t timerProdukcji;

pid_t fork_pid = -1;


void generujBlokPipe(int pipeId){
	static char obecny_znak = 'a';
	if(obecny_znak>'z') 
		obecny_znak='A';
	
	if(obecny_znak>'Z' && obecny_znak < 'a')
		obecny_znak = 'a';

	int setVal = (int)obecny_znak;
	char arr[641]; //amount  of data i want to generate+1;
	memset(arr,setVal, WIELKOSC_BLOKU); 
	obecny_znak+=1;
	int wpisane_dane = write(pipeId,&arr,WIELKOSC_BLOKU);
	IleWygenerowanoBlokow+=1;
}

void Prod(float interwal)//interwal w sekundach
{	
	struct timespec interval = float_to_timespec(interwal);
	struct timespec last_prod;
	struct timespec current_time;
	struct timespec delta;
	clock_gettime(CLOCK_MONOTONIC,&last_prod);
	
    while(!koniec){

        clock_gettime(CLOCK_MONOTONIC,&current_time);
        timespec_diff_macro(&current_time,&last_prod,&delta);

        if(time_compare(&delta,&interval)>=0){
            generujBlokPipe(MagPipe[1]);
            clock_gettime(CLOCK_MONOTONIC,&last_prod);
    	}
    }
}


int main(int argc, char* argv[])
{
	//wczesna inicjalizacja 
	if(pipe(MagPipe)!=0)
		perror("Blad tworzenia fajki");
	IleWygenerowanoBlokow = 0;
	pojemnosc_magazynu = PobierzPojemnoscMagazynu();
	aktualny_stan_magazynu = PobierzAktualnyStanMagazynu();

	char *nazwa_hosta="localhost";
	int port_hosta = 0;
	char *ostatni_parametr =NULL;
	char *colon_index = NULL;
	int index_colon = 0;
	koniec = 0;
	
	// Parsowanie parametrów
	int gopt;
	while((gopt=getopt(argc, argv, "p:")) != -1)
	{
		switch(gopt)
		{
			case 'p':{
				float pValue = strtof(optarg, NULL);
				if(pValue <0 || pValue > MAX_TEMPO_PRODUKCJI)
					LogError("Zla wartosc parametru p!");
				tempo_produkcji=(STALA_PRODUKCJI)*pValue; // ile bajtow na sek
			}
				break;
		}
	}
	ostatni_parametr = argv[optind];
	if(ostatni_parametr == NULL){
		LogError("Parametr port jest wymagany");
	}
	colon_index=strchr(ostatni_parametr,':');

	if(colon_index!=NULL){//czyli zostal podany addres
		nazwa_hosta = malloc((int)(colon_index-ostatni_parametr)+1);
		strncpy(nazwa_hosta,ostatni_parametr,(int)(colon_index-ostatni_parametr));
		char portChar[5];
		strncpy(portChar,colon_index+1, strlen(ostatni_parametr) -(colon_index-ostatni_parametr));
		
		port_hosta = ch_to_int(portChar);
	}
    else{//nie ma podanego adresu
		port_hosta=ch_to_int(ostatni_parametr);
	}

	//Validacje poprawności parametrów
	if(tempo_produkcji<=0)
		LogError("podaj poprawny parametr tempa");

	float interval = ((float)WIELKOSC_BLOKU)/tempo_produkcji;

	PrzygotujSerwer(nazwa_hosta, port_hosta);
	
	fork_pid= fork();
	if(fork_pid== -1) 
			LogError("Blad forka!");
	else if(fork_pid== 0){// dziecko
        #ifdef DYSTRYBUTOR
            Prod(interval);        
		#else	
				ReaktorPool(TablicaPolla[0]);
		#endif
	}  
	else { // rodzic
		#ifdef DYSTRYBUTOR
				
		ReaktorPoll(TablicaPolla[0]);
		#else	
				Prod(interval);

		#endif
	}

	//Zakonczenie(cleanup)
	puts("Koncze dzialanie");
	if(fork_pid==0) kill(fork_pid,SIGKILL);
	
	close(MagPipe[0]);
	close(MagPipe[1]);
	return (0);
}
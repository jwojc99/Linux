#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h> 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <sys/timerfd.h>
#include <stdint.h>
#define WIELKOSC_PRODUKTU 13*1024
#define timespec_diff_macro(a, b, result)                  \
  do {                                                \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;  \
    if ((result)->tv_nsec < 0) {                      \
      --(result)->tv_sec;                             \
      (result)->tv_nsec += 1000000000;                \
    }                                                 \
  } while (0)

struct timespec monotime_conn; // czas polaczenia
struct timespec monotime_recv; // czas otrzymiania pierwszej porcji
struct timespec monotime_close; // czas zamkniecia polaczenia
struct timespec monotime_delta;
//struct timespec monotime3;
//struct timespec monotime4;

struct timespec realtime;
int ch_to_int(const char *msg);
char * int_to_str(int v);
void LogWarning(const char *message);
void LogError(const char *message);

int time_compare(struct timespec const * const a, struct timespec const * const b);
struct timespec float_to_timespec(float interval);
void PobierzAdresPort(int polaczenieFD,char *buforAdresu,char *buforPort);
#endif
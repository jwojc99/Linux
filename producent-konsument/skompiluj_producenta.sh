#!/bin/bash
#gcc -Wall  -Wextra -Werror utils.h serwer.h -o producent utils.c serwer.c producent.c -lrt
gcc -Wall  utils.h serwer.h -o producent producent.c utils.c serwer.c  -lrt

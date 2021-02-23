#!/bin/bash
gcc -Wall utils.h -o konsument utils.c konsument.c -lrt -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define Ntel 3
#define Ncook 2
#define Noven 10
#define Ndeliver 7
#define Torderlow 1
#define Torderhigh 5
#define Norderlow 1
#define Norderhigh 5
#define Tpaymentlow 1
#define Tpaymenthigh 2
#define Cpizza 10
#define Pfail 0.05
#define Tprep 1
#define Tbake 10
#define Tpack 2
#define Tdellow 5
#define Tdelhigh 15
#define Nsec 100000000
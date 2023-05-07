#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "cs402.h"
#include "my402list.h"
#include "myfunction.h"

int microsecond_duration(struct timeval begin, struct timeval end)
{
	return (end.tv_sec-begin.tv_sec)*1000000 + end.tv_usec-begin.tv_usec;
}
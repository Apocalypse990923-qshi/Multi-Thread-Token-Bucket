#include "cs402.h"
#include "my402list.h"

typedef struct packet_infomation
{
	int serial;
	int inter_arrival;
	int token_num;
	int service_time;

	struct timeval arrival;
	struct timeval enter_q1;
	struct timeval leave_q1;
	struct timeval enter_q2;
	struct timeval leave_q2;
	struct timeval enter_s;
	struct timeval leave_s;
} packet_info;

extern int microsecond_duration(struct timeval, struct timeval);
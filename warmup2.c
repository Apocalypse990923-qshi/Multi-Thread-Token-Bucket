#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include "cs402.h"
#include "my402list.h"
#include "myfunction.h"

int B=10,P=3,num=20;
double lambda=1,mu=0.35,r=1.5;
int r_interval=10000000,lambda_interval=10000000,mu_interval=10000000;
FILE *file=NULL;
char FILENAME[30];
pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
pthread_t p_thread,t_thread,s1_thread,s2_thread,m_thread;
int T=0;
struct timeval begin,end;
My402List q1,q2;
int token_drop=0,packet_drop=0,packet_complete=0,packet_arrived=0,t_serial=0;
int quit=0;
double inter_arrival_avg=0,service_time_avg=0,sys_time_avg=0,sys_time_sqr_avg=0;
double avg_num_q1=0,avg_num_q2=0,avg_num_s1=0,avg_num_s2=0;
sigset_t set;

int parse_cmdline(int argc, char *argv[])
{
	int mode=1;	//1=Deterministic,2=Trace-driven
	if(argc!=1 && argc!=3 && argc!=5 && argc!=7 && argc!=9 && argc!=11 && argc!=13 && argc!=15)
	{
		fprintf(stderr,"Invalid number of commandline arguments because it must be an odd number!\n");
		fprintf(stderr,"Please type the valid commandline syntax: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]!\n");
		return 0;
	}
	int i=1;
	while(i<argc)
	{
		if(strcmp(argv[i], "-lambda")==0)
		{
			if(sscanf(argv[i+1], "%lf", &lambda) != 1)
			{
				fprintf(stderr,"Invalid parameter specification on lambda field!\n");
				fprintf(stderr,"Please type the valid commandline value after commandline option!\n");
				return 0;
			}
		}
		else if(strcmp(argv[i], "-mu")==0)
		{
			if(sscanf(argv[i+1], "%lf", &mu) != 1)
			{
				fprintf(stderr,"Invalid parameter specification on mu field!\n");
				fprintf(stderr,"Please type the valid commandline value after commandline option!\n");
				return 0;
			}
		}
		else if(strcmp(argv[i], "-r")==0)
		{
			if(sscanf(argv[i+1], "%lf", &r) != 1)
			{
				fprintf(stderr,"Invalid parameter specification on r field!\n");
				fprintf(stderr,"Please type the valid commandline value after commandline option!\n");
				return 0;
			}
		}
		else if(strcmp(argv[i], "-B")==0)
		{
			if(sscanf(argv[i+1], "%d", &B) != 1)
			{
				fprintf(stderr,"Invalid parameter specification on B field!\n");
				fprintf(stderr,"Please type the valid commandline value after commandline option!\n");
				return 0;
			}
		}
		else if(strcmp(argv[i], "-P")==0)
		{
			if(sscanf(argv[i+1], "%d", &P) != 1)
			{
				fprintf(stderr,"Invalid parameter specification on P field!\n");
				fprintf(stderr,"Please type the valid commandline value after commandline option!\n");
				return 0;
			}
		}
		else if(strcmp(argv[i], "-n")==0)
		{
			if(sscanf(argv[i+1], "%d", &num) != 1)
			{
				fprintf(stderr,"Invalid parameter specification on num field!\n");
				fprintf(stderr,"Please type the valid commandline value after commandline option!\n");
				return 0;
			}
		}
		else if(strcmp(argv[i], "-t")==0)
		{
			strcpy(FILENAME,argv[i+1]);
			file = fopen(FILENAME,"rt");
			if(file==NULL)
			{
				perror("Failed to open file! Reason is");
				fprintf(stderr,"Please check if filename is valid or file exists or access is permitted!\n");
       			return 0;
			}
			char buffer[1030];
			if(fgets(buffer,sizeof(buffer),file)==NULL || strlen(buffer)>1024 || buffer[strlen(buffer)-1]!='\n')
			{
				fprintf(stderr,"File may not be in the right format!\n");
				fprintf(stderr,"Please check if file is a valid trace specification file!\n");
				return 0;
			}
			buffer[strlen(buffer)-1]='\0';
			if(sscanf(buffer, "%d", &num) != 1)
			{
				fprintf(stderr,"File may not be in the right format!\n");
				fprintf(stderr,"Please check if file is a valid trace specification file!\n");
				return 0;
			}
			mode=2;
		}
		else
		{
			fprintf(stderr,"Invalid commandline option!\n");
			fprintf(stderr,"Please type the valid commandline syntax: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]!\n");
			return 0;
		}
		i+=2;
	}
	return mode;
}	

packet_info* parse_line(int line)
{
	char buffer[1030];
	if(fgets(buffer,sizeof(buffer),file)==NULL)
	{
		fprintf(stderr,"Failed to parse line %d!\n",line+1);
		return NULL;
	}
	if(strlen(buffer)>1024)
	{
		fprintf(stderr,"Line %d is too long!\n",line+1);
		return NULL;
	}
	if(buffer[strlen(buffer)-1]!='\n')
	{
		fprintf(stderr,"Line %d is not terminated with Enter!\n",line+1);
		return NULL;
	}
	packet_info* p=(packet_info*)malloc(sizeof(packet_info));
	if(sscanf(buffer, "%d %d %d", &(p->inter_arrival),&(p->token_num),&(p->service_time)) != 3)
	{
		fprintf(stderr,"Failed to parse line %d due to invalid format of fields!\n",line+1);
		return NULL;
	}
	p->inter_arrival*=1000;
	p->service_time*=1000;
	p->serial=line;
	return p;
}

void *packet(void *arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	struct timeval prev_arrival=begin;
	packet_info* p;
	int i;
	for(i=1;i<=num;i++)
	{
		if(file!=NULL)
		{
			p=parse_line(i);
			if(p==NULL) pthread_exit((void*)-2);
		}
		else
		{
			p=(packet_info*)malloc(sizeof(packet_info));
			p->serial=i;
			p->inter_arrival=lambda_interval;
			p->token_num=P;
			p->service_time=mu_interval;
		}
		pthread_cleanup_push(free, p);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		usleep(p->inter_arrival);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_cleanup_pop(0);
		pthread_mutex_lock(&m);
		gettimeofday(&(p->arrival),NULL);
		if(quit)
 		{
 			free(p);
 			pthread_mutex_unlock(&m);
 			pthread_exit((void*)(i-1));
 		}
		int t=microsecond_duration(begin,p->arrival);
		int t_inter=microsecond_duration(prev_arrival,p->arrival);
		printf("%08d.%03dms: p%d arrives, needs %d tokens, inter-arrival time = %d.%03dms",t/1000,t%1000,p->serial,p->token_num,t_inter/1000,t_inter%1000);
		prev_arrival=p->arrival;
		inter_arrival_avg=((i-1)*inter_arrival_avg+(double)t_inter/1000000)/i;
		packet_arrived++;
		if(p->token_num>B)
		{
			packet_drop++;
			printf(", dropped\n");
			if(p->serial==num)
			{
				quit=1;
				pthread_cond_broadcast(&cv);
			}
			free(p);
			pthread_mutex_unlock(&m);
			continue;
		}
		else	printf("\n");
 		My402ListAppend(&q1, p);
 		gettimeofday(&(p->enter_q1),NULL);
 		t=microsecond_duration(begin,p->enter_q1);
 		printf("%08d.%03dms: p%d enters Q1\n",t/1000,t%1000,p->serial);
 		if(My402ListLength(&q1)==1 && T>=p->token_num)
 		{
 			T-=p->token_num;
 			My402ListUnlink(&q1, My402ListFirst(&q1));
 			gettimeofday(&(p->leave_q1),NULL);
 			t=microsecond_duration(begin,p->leave_q1);
 			t_inter=microsecond_duration(p->enter_q1,p->leave_q1);
 			printf("%08d.%03dms: p%d leaves Q1, time in Q1 = %d.%03dms, token bucket now has %d token\n",t/1000,t%1000,p->serial,t_inter/1000,t_inter%1000,T);
 			avg_num_q1+=(double)t_inter/1000000;
 			My402ListAppend(&q2, p);
 			gettimeofday(&(p->enter_q2),NULL);
 			t=microsecond_duration(begin,p->enter_q2);
 			printf("%08d.%03dms: p%d enters Q2\n",t/1000,t%1000,p->serial);
 			pthread_cond_broadcast(&cv);
 			if(p->serial==num)	quit=1;
 		}
 		pthread_mutex_unlock(&m);
	}
	pthread_exit((void*)(i-1));
}

void *token(void *arg)
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	packet_info* p;
	struct timeval now;
	while(1)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		usleep(r_interval);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_mutex_lock(&m);
		gettimeofday(&now,NULL);
		if(My402ListLength(&q1)==0 && quit)
 		{
 			pthread_mutex_unlock(&m);
 			pthread_exit((void*)t_serial);
 		}
		int t=microsecond_duration(begin,now);
		printf("%08d.%03dms: token t%d arrives, ",t/1000,t%1000,t_serial+1);
		t_serial++;
 		if(T<B)
 		{
 			T++;
 			printf("token bucket now has %d token\n",T);
 		}
 		else
 		{
 			token_drop++;
 			printf("dropped\n");
 		}
 		if(My402ListLength(&q1)>=1)
 		{
 			p=(packet_info*)(My402ListFirst(&q1)->obj);
 			if(T>=p->token_num)
 			{
 				T-=p->token_num;
 				My402ListUnlink(&q1, My402ListFirst(&q1));
 				gettimeofday(&(p->leave_q1),NULL);
 				t=microsecond_duration(begin,p->leave_q1);
 				int t_inter=microsecond_duration(p->enter_q1,p->leave_q1);
 				printf("%08d.%03dms: p%d leaves Q1, time in Q1 = %d.%03dms, token bucket now has %d token\n",t/1000,t%1000,p->serial,t_inter/1000,t_inter%1000,T);
 				avg_num_q1+=(double)t_inter/1000000;
 				My402ListAppend(&q2, p);
 				gettimeofday(&(p->enter_q2),NULL);
 				t=microsecond_duration(begin,p->enter_q2);
 				printf("%08d.%03dms: p%d enters Q2\n",t/1000,t%1000,p->serial);
 				pthread_cond_broadcast(&cv);
 				if(p->serial==num)	quit=1;
 			}
 		}
 		pthread_mutex_unlock(&m);
	}
}

void *server(void *arg)
{
	int s=*((int*)arg);
	packet_info* p;
	while(1)
	{
 		pthread_mutex_lock(&m);
 		while(My402ListLength(&q2)==0 && !quit)
 		{
 			pthread_cond_wait(&cv, &m);
 		}
 		if(My402ListLength(&q2)==0 && quit)
 		{
 			pthread_mutex_unlock(&m);
 			pthread_exit((void*)0);
 		}
 		p=(packet_info*)(My402ListFirst(&q2)->obj);
 		My402ListUnlink(&q2, My402ListFirst(&q2));
 		gettimeofday(&(p->leave_q2),NULL);
 		int t=microsecond_duration(begin,p->leave_q2);
 		int t_inter=microsecond_duration(p->enter_q2,p->leave_q2);
 		printf("%08d.%03dms: p%d leaves Q2, time in Q2 = %d.%03dms\n",t/1000,t%1000,p->serial,t_inter/1000,t_inter%1000);
 		avg_num_q2+=(double)t_inter/1000000;

 		gettimeofday(&(p->enter_s),NULL);
 		t=microsecond_duration(begin,p->enter_s);
		printf("%08d.%03dms: p%d begins service at S%d, requesting %dms of service\n",t/1000,t%1000,p->serial,s,p->service_time/1000);
 		pthread_mutex_unlock(&m);

 		usleep(p->service_time);
 		pthread_mutex_lock(&m);
 		gettimeofday(&(p->leave_s),NULL);
 		t=microsecond_duration(begin,p->leave_s);
 		t_inter=microsecond_duration(p->enter_s,p->leave_s);
 		packet_complete++;
 		printf("%08d.%03dms: p%d departs from S%d, service time = %d.%03dms, ",t/1000,t%1000,p->serial,s,t_inter/1000,t_inter%1000);
 		service_time_avg=((packet_complete-1)*service_time_avg+(double)t_inter/1000000)/packet_complete;
 		if(s==1)	avg_num_s1+=(double)t_inter/1000000;
 		else	avg_num_s2+=(double)t_inter/1000000;
 		t_inter=microsecond_duration(p->arrival,p->leave_s);
 		printf("time in system = %d.%03dms\n",t_inter/1000,t_inter%1000);
 		sys_time_avg=((packet_complete-1)*sys_time_avg+(double)t_inter/1000000)/packet_complete;
 		sys_time_sqr_avg=((packet_complete-1)*sys_time_sqr_avg+((double)t_inter/1000000)*((double)t_inter/1000000))/packet_complete;
 		pthread_mutex_unlock(&m);

 		if(p->serial==num)
 		{
 			pthread_mutex_lock(&m);
 			quit=1;
 			pthread_cond_broadcast(&cv);
 			pthread_mutex_unlock(&m);
 			free(p);
 			pthread_exit((void*)0);
 		}
 		free(p);
	}
}

void *monitor(void *arg)
{
	int sig;
	struct timeval now;
	packet_info* p;
 	while (1)
 	{
 		sigwait(&set, &sig);
 		pthread_mutex_lock(&m);
 		gettimeofday(&now,NULL);
 		pthread_cancel(p_thread);
		pthread_cancel(t_thread);
 		quit=1;
		int t=microsecond_duration(begin,now);
		printf("\n%08d.%03dms: SIGINT caught, no new packets or tokens will be allowed\n",t/1000,t%1000);
 		while(My402ListLength(&q1)>0)
 		{
 			p=(packet_info*)(My402ListFirst(&q1)->obj);
 			My402ListUnlink(&q1, My402ListFirst(&q1));
 			gettimeofday(&now,NULL);
 			t=microsecond_duration(begin,now);
			printf("%08d.%03dms: p%d removed from Q1\n",t/1000,t%1000,p->serial);
			free(p);
 		}
 		while(My402ListLength(&q2)>0)
 		{
 			p=(packet_info*)(My402ListFirst(&q2)->obj);
 			My402ListUnlink(&q2, My402ListFirst(&q2));
 			gettimeofday(&now,NULL);
 			t=microsecond_duration(begin,now);
			printf("%08d.%03dms: p%d removed from Q2\n",t/1000,t%1000,p->serial);
			free(p);
 		}
 		pthread_cond_broadcast(&cv);
 		pthread_mutex_unlock(&m);
 		pthread_exit((void*)0);
 	}
}

void PrintEmulationParameters(int mode)
{
	printf("Emulation Parameters:\n");
	printf("\tnumber to arrive = %d\n",num);
	if(mode==1)
	{
		printf("\tlambda = %f\n",lambda);
		printf("\tmu = %f\n",mu);
	}
	printf("\tr = %f\n",r);
	printf("\tB = %d\n",B);
	if(mode==1)	printf("\tP = %d\n",P);
	if(mode==2)	printf("\ttsfile = %s\n",FILENAME);
	printf("\n");
}

void PrintStatistic(int t)
{
	double total=(double)t/1000000;
	double std_deviation=sys_time_sqr_avg-sys_time_avg*sys_time_avg;
	if(std_deviation<=0)	std_deviation=0;
	else	std_deviation=sqrt(std_deviation);
	printf("\nStatistics:\n");
	printf("\taverage packet inter-arrival time = %.6g\n",inter_arrival_avg);
	printf("\taverage packet service time = %.6g\n",service_time_avg);
	printf("\taverage number of packets in Q1 = %.6g\n",avg_num_q1/total);
	printf("\taverage number of packets in Q2 = %.6g\n",avg_num_q2/total);
	printf("\taverage number of packets at S1 = %.6g\n",avg_num_s1/total);
	printf("\taverage number of packets at S2 = %.6g\n",avg_num_s2/total);
	printf("\taverage time a packet spent in system = %.6g\n",sys_time_avg);
	printf("\tstandard deviation for time spent in system = %.6g\n",std_deviation);
	printf("\ttoken drop probability = %.6g\n",(double)token_drop/t_serial);
	printf("\tpacket drop probability = %.6g\n",(double)packet_drop/packet_arrived);
}

int main(int argc, char *argv[])
{
	int mode=parse_cmdline(argc,argv);
	if(mode==0)	exit(EXIT_FAILURE);
	if(r>0.1) r_interval=round(1000000/r);
	if(mode==1)
	{
		if(lambda>0.1) lambda_interval=round(1000000/lambda);
		if(mu>0.1) mu_interval=round(1000000/mu);
	}

	PrintEmulationParameters(mode);

	memset(&q1, 0, sizeof(My402List));
	memset(&q2, 0, sizeof(My402List));
	(void)My402ListInit(&q1);
	(void)My402ListInit(&q2);
	void *p_result=(void*)0, *t_result=(void*)0;
	int s1=1,s2=2;

	sigemptyset(&set);
	sigaddset(&set,SIGINT);
	sigprocmask(SIG_BLOCK,&set,0);

	gettimeofday(&begin,NULL);
	printf("00000000.000ms: emulation begins\n");

	pthread_create(&m_thread,0,monitor,NULL);
	pthread_create(&p_thread,0,packet,NULL);
	pthread_create(&t_thread,0,token,NULL);
	pthread_create(&s1_thread,0,server,&s1);
	pthread_create(&s2_thread,0,server,&s2);
	pthread_join(p_thread, &p_result);
	if((int)p_result==-2) exit(EXIT_FAILURE);
	pthread_join(t_thread, &t_result);
	pthread_join(s1_thread, 0);
	pthread_join(s2_thread, 0);

	pthread_mutex_lock(&m);
	gettimeofday(&end,NULL);
	int t=microsecond_duration(begin,end);
	printf("%08d.%03dms: emulation ends\n",t/1000,t%1000);
	pthread_mutex_unlock(&m);

	PrintStatistic(t);
	return 0;
}
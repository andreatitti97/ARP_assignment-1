#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <netdb.h>

#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct{
	timeval time;
	float value = 0;
}msg;

msg token;

char *ts;
pid_t pid_S, pid_G, pid_L, pid_P;

char filename[30] = "results/signal_";

FILE *fp; //Configuration file

char *signame[] = { "SIGINT", "SIGKILL", "SIGUSR1", "SIGUSR2", "SIGTERM", "SIGCONT", "SIGSTOP", NULL};

// Init fifo
const char *fifo1 = "fifo/fifo1";
const char *fifo2 = "fifo/fifo2"; 
const char *fifo3 = "fifo/fifo3"; 

void error(const char *msg)
{
	perror(msg);
	exit(-1);
}

// Read the configuration file info
void loadConfig(char *ip, char *port, int *waiting_time, int *f)
{
	fp = fopen("ConfigurationFile.txt", "r");

	if (fp == NULL)
		error("ERROR OPEN ConfigurationFile");

	fscanf(fp, "%s", ip);
	fscanf(fp, "%s", port);
	fscanf(fp, "%d", waiting_time);
	fscanf(fp, "%d", f);

	printf("IP : %s\n", ip);
	printf("Port : %s\n", port);
	printf("Waiting time : %d\n", waiting_time);
	printf("Reference Frequency : %d\n", *f);

	fclose(fp);
}

void logFile(pid_t pid, char id, float msg1, float msg2)
{
	FILE *fileLog;
	fileLog = fopen("results/File.log", "a");
	time_t currTime;
	currTime = time(NULL);
	ts = ctime(&currTime);
	if (msg1 == SIGCONT)
	{
		fprintf(fileLog, "-Current Time: %s from %c action: DUMP LOG", ts, id);
	}else
	{
		fprintf(fileLog, "-Current Time: %s PID: %d, PROCESS: %c value:%.3f.\n", ts, pid, id, msg1);
	}
	fprintf(fileLog, "-%s%.3f.\n\n", ts, msg2);

	fclose(fileLog);
}

void signalFileInit(int f)
{
 	FILE *tokenFile;

	char frequency[10];
	sprintf(frequency, "%d", f);
	strcat(filename, frequency);
	strcat(filename,".txt");
	tokenFile = fopen(filename, "w+");
}
void signalFileUpdate(double token)
{
	FILE *tokenFile;
	tokenFile = fopen(filename, "a");
	fprintf(tokenFile, "%f\n",token);
	fclose(tokenFile);
}

void sig_handler(int signo)
{
	if (signo == SIGUSR1) //Stop Process
	{
		printf("Received SIGUSR1\n");
		kill(pid_P, SIGSTOP);
		kill(pid_G, SIGSTOP);
		kill(pid_L, SIGSTOP);
	}
	else if (signo == SIGUSR2) //Resume Process
	{
		printf("Received SIGUSR2\n");
		kill(pid_P, SIGCONT);
		kill(pid_G, SIGCONT);
		kill(pid_L, SIGCONT);
	}
	else if (signo == SIGCONT) //Dump Log
	{
		printf("Received SIGCONT\n");
		printf("%d\n", pid_S);
		logFile(pid_S, 'S',(float)signo, token.value);
		printf("-%sPID: %d value:%s.\n", ts, pid_S, signame[(int)signo]);
		printf("-%s%.3f.\n\n", ts, token.value);
	}
}

int main(int argc, char *argv[])
{
	char port[128];   //Socket port
	char ip[32];	  //IP address of the next student
	int waiting_time;			  // Waiting time
	int f;			  //Frequency of the sine wave

	loadConfig(ip, port, &waiting_time, &f);

	int n;			   //Return value for writing or reading pipes
	struct timeval tv; //time varaible for compute DELAY for operation of "Select"

	char *argdata[4];  //Process G execution argument
	char *cmd = "./executables/G"; //Process G executable path

	msg msg1, msg2; //Message from P to L

	argdata[0] = cmd;
	argdata[1] = port;
	argdata[2] = (char *)fifo2;
	argdata[3] = NULL;

	int flag = 1; // crescente

	signalFileInit(f);
	/*-----------------------------------------Pipes Creation---------------------------------------*/

	if (mkfifo(fifo1, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|S
		perror("Cannot create fifo 1. Already existing?");

	if (mkfifo(fifo2, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|G
		perror("Cannot create fifo 2. Already existing?");

	if (mkfifo(fifo3, S_IRUSR | S_IWUSR) != 0) //creo file pipe P|L
		perror("Cannot create fifo 3. Already existing?");

	int fd1 = open(fifo1, O_RDWR); //apro la pipe1

	if (fd1 == 0)
	{	
		unlink(fifo1);
		error("Cannot open fifo 1");
	}

	int fd2 = open(fifo2, O_RDWR); //apro la pipe2

	if (fd2 == 0)
	{	
		unlink(fifo2);
		error("Cannot open fifo 2");
	}

	int fd3 = open(fifo3, O_RDWR); //apro la pipe3

	if (fd3 == 0)
	{	
		unlink(fifo3);
		error("Cannot open fifo 3");
	}

	printf("\n");

	/*-------------------------------------------Process P--------------------------------------*/

	pid_P = fork();
	if (pid_P < 0)
	{
		perror("Fork P");
		return -1;
	}

	if (pid_P == 0)
	{
		//float line_G = 0; //Recived message from G
		msg line_G;
		float line_S;	  //Recived message from S
		int retval, fd;
		fd_set rfds;

		struct timeval current_time;
		//struct timespec time; //current time for DT computation
		//struct timespec prev_time; //previous time for DT computation
		double delay_time;

		sleep(2);

		/*-------------------------------------Socket (client) initialization---------------------------*/
		int sockfd, portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		portno = atoi(port);
		float old_tok, new_tok;

		printf("Process P PID is : %d.\n", getpid());

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			error("ERROR opening socket");

		server = gethostbyname(ip);
		if (server == NULL)
			error("ERROR, no such host\n");

		bzero((char *)&serv_addr, sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;

		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

		serv_addr.sin_port = htons(portno);
		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			error("ERROR connecting");

		n = write(sockfd, &line_G, sizeof(line_G));
		if (n < 0)
			error("ERROR writing to socket");

		while (1) //Select body
		{
			FD_ZERO(&rfds);
			FD_SET(fd1, &rfds);
			FD_SET(fd2, &rfds);

			fd = max(fd1, fd2);

			tv.tv_sec = 5;
			tv.tv_usec = 0;

			retval = select(fd + 1, &rfds, NULL, NULL, &tv);

			switch (retval)
			{

			case 0:
			//Either no active pipes or the timeout has expired
				printf("No data avaiable.\n");
				break;

			case 1:
				// If only one pipe is active, check which is between S and G
				if (FD_ISSET(fd1, &rfds))
				{
					n = read(fd1, &line_S, sizeof(line_S));
					if (n < 0)
						error("ERROR reading from S");
					printf("From S recivedMsg = %.3f \n", line_S);
					sleep((int)line_S);
				}
				else if (FD_ISSET(fd2, &rfds))
				{
					// If G, make the computation and log the results through L
					n = read(fd2, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR reading from G");

					n = write(fd3, &line_G, sizeof(line_G));
					if (n < 0)
						error("ERROR writing to L");

					// Get the current time 
					gettimeofday(&current_time, NULL);

					// Compute DT
					delay_time = (double)(current_time.tv_sec - token.time.tv_sec) + (double)(current_time.tv_usec - token.time.tv_usec)/(double)1000000;
					printf("DT: %f\n",delay_time);
					old_tok = line_G.value;
					//message.token = old_tok + delay_time * (1 - pow(old_tok,2)/2 ) * 2 * 3.14 * f; // UNCOMMENT FOR REPORT FORMULA, WRONG

					//printf("Old token value: %f\n", fabs(old_tok));
					if (old_tok >= 1)
					{
						flag = 0;
					}else if(old_tok <= -1)
					{
						flag = 1;
					}
					//printf("Flag: %d\n", flag);
					switch(flag)
						{
							case 0:
								token.value = old_tok * cos(2 * 3.14 * f * delay_time) - sqrt(1 - pow(old_tok,2)/2 ) * sin(2 * 3.14 * f * delay_time);
								// Saturate Signal
								if (token.value < -1)
								{
									token.value = -1;
									flag = 1;
								}
								break;
							case 1:
								token.value = old_tok * cos(2 * 3.14 * f * delay_time) + sqrt(1 - pow(old_tok,2)/2 ) * sin(2 * 3.14 * f * delay_time);
								// Saturate Signal
								if (token.value > 1)
								{
									token.value = 1;
									flag = 0;
								}
								break;
						}
					signalFileUpdate(token.value);
					
					token.time = current_time;
					// Send new value to L
					n = write(fd3, &token, sizeof(token));
					if (n < 0)
						error("ERROR writing to L");

					// Write new value to the socket
					n = write(sockfd, &token, sizeof(token));
					if (n < 0)
						error("ERROR writing to socket");
					usleep(waiting_time); 	//Simulate communication delay
				}

				sleep(1);
				break;

			case 2:
				// If two active pipes, give priority to S
				n = read(fd1, &line_S, sizeof(line_S));
				if (n < 0)
					error("ERROR reading from S");
				printf("From S recivedMsg = %.3f \n", line_S);
				sleep((int)line_S);
				break;

			default:
				perror("You should not be here!");
				break;
			}
		}

		close(fd1);
		unlink(fifo1);

		close(fd2);
		unlink(fifo2);

		close(fd3);
		unlink(fifo3);

		close(sockfd);
	}

	else
	{
		pid_L = fork();

		if (pid_L < 0)
		{
			perror("Fork L");
			return -1;
		}

		if (pid_L == 0)
		{
			printf("Process L PID is : %d.\n", getpid());

			while (1)
			{
				n = read(fd3, &msg1, sizeof(msg1));
				if (n < 0)
					error("ERROR reciving file from P");

				n = read(fd3, &msg2, sizeof(msg2));
				if (n < 0)
					error("ERROR reciving file from P");

				logFile(getpid(),'G', msg1.value, msg2.value);
			}

			close(fd3);
			unlink(fifo3);
		}

		else
		{

			/*----------------------------------------------Process G---------------------------------------*/

			pid_G = fork(); // FORK 2
			if (pid_G < 0)
			{
				perror("Fork G");
				return -1;
			}

			if (pid_G == 0)
			{
				printf("Process G PID is : %d.\n", getpid());
				int a = execvp(argdata[0], argdata);
				printf("%d",a);
				error("Exec fallita");
				return 0;
			}

			/*----------------------------------------------Process S---------------------------------------*/

			pid_S = getpid();

			printf("Process S PID is : %d.\n", getpid());

			if (signal(SIGUSR1, sig_handler) == SIG_ERR)
				printf("Can't catch SIGUSR1\n");

			if (signal(SIGCONT, sig_handler) == SIG_ERR)
				printf("Can't catch SIGCONT\n");

			if (signal(SIGUSR2, sig_handler) == SIG_ERR)
				printf("Can't catch SIGUSER2\n");

			sleep(5);

			while (1)
			{
			
			}

			close(fd1);
			unlink(fifo1);

			close(fd2);
			unlink(fifo2);

			close(fd3);
			unlink(fifo3);
		}

		return 0;
	}
}

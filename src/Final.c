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

''' Structure which define the TOKEN '''
typedef struct{
	timeval time;
	float value = 0;
}token_msg;
''' Structure which defie the MESSAGES between processes '''
typedef struct{
	char processName;
	float message1;
	float message2;
}pipe_msg;

token_msg token;

char *ts;
pid_t pid_S, pid_G, pid_L, pid_P;

char filename[30] = "results/signal_";

FILE *fp; //Configuration file

char *signame[] = {"INVALID", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGPOLL", "SIGPWR", "SIGSYS", NULL};

// Init fifo
const char *fifo1 = "fifo/fifo1"; 
const char *fifo2 = "fifo/fifo2"; 
const char *fifo3 = "fifo/fifo3"; 
int fd1, fd2, fd3;
int flag_log; //flag for action DUMP LOG

void error(const char *msg)
{
	perror(msg);
	exit(-1);
}

''' Function for reading the configuration file '''
void loadConfig(char *ip, char *port, int *waiting_time, int *rf)
{
	fp = fopen("ConfigurationFile.txt", "r");

	if (fp == NULL)
		error("Failed to open ConfigurationFile");

	fscanf(fp, "%s", ip);
	fscanf(fp, "%s", port);
	fscanf(fp, "%d", waiting_time);
	fscanf(fp, "%d", rf);

	printf("IP : %s\n", ip);
	printf("Port : %s\n", port);
	printf("Waiting time : %d\n", waiting_time);
	printf("Reference Frequency : %d\n", *rf);

	fclose(fp);
}

''' Function for update the Log File '''
void logFile(char pName, float firstMsg, float secondMsg)
{
	char ch;
	FILE *fileLog;
	fileLog = fopen("results/File.log", "a");
	time_t currentTime;
	currentTime = time(NULL);
	ts = ctime(&currentTime);
	
	if (pName == 'S')
	{
		switch ((int)firstMsg)
		{
		case 10://SIGUSR1
			fprintf(fileLog, "\n -Current Time: %s FROM %c action: STOP\n", ts, pName);
			break;
		case 12://SIGUSR2
			fprintf(fileLog, "\n -Current Time: %s FROM %c action: START\n", ts, pName);
			break;
		case 18://SIGCONT
			fprintf(fileLog, "\n -Current Time: %s FROM %c action: DUMP LOG\n", ts, pName);
			
			break;
		default:
			break;
		}
	}else if(pName == 'G'){
		fprintf(fileLog, "\n -Current Time: %s FROM: %c value:%.3f.\n",ts, pName, firstMsg);
		fprintf(fileLog, "\n -Current Time: %s New token value: %.3f.\n\n", ts, secondMsg);
	}else{

	}

	fclose(fileLog);
}

''' Function for initialize the file in which save the token values '''
void signalFileInit(int rf)
{
 	FILE *tokenFile;

	char frequency[10];
	sprintf(frequency, "%d", rf);
	strcat(filename, frequency);
	strcat(filename,".txt");
	tokenFile = fopen(filename, "w+");
}
''' Update the token File '''
void signalFileUpdate(double token)
{
	FILE *tokenFile;
	tokenFile = fopen(filename, "a");
	fprintf(tokenFile, "%f\n",token);
	fclose(tokenFile);
}

''' SIGNAL HANDLER '''
void sig_handler(int signo)
{
	if (signo == SIGUSR1) //STOP
	{
		printf("Received SIGUSR1\n");
		write(fd1, &signo, sizeof(signo));
		kill(pid_P, SIGSTOP);
		kill(pid_G, SIGSTOP);
		kill(pid_L, SIGSTOP);
	}
	else if (signo == SIGUSR2) //START
	{
		printf("Received SIGUSR2\n");
		kill(pid_P, SIGCONT);
		kill(pid_G, SIGCONT);
		kill(pid_L, SIGCONT);
		write(fd1, &signo, sizeof(signo));
	}
	else if (signo == SIGCONT) //DUMP LOG

		printf("Received SIGCONT\n from PID: %d value:%s.\n", pid_S, signame[(int)signo]);
		flag_log = 1;
		write(fd1, &signo, sizeof(signo));
	}
}

int main(int argc, char *argv[])
{
	char port[128];   //Socket port
	char ip[32];	  //IP address of the next student (local host in V2.0 of the assignemnt, COVID19)
	int waiting_time;			  // Waiting time (simulate delay in the communication)
	int rf;			  //Frequency of the sine wave

	loadConfig(ip, port, &waiting_time, &rf);

	int n;			   //Return value of write and read functions
	struct timeval tv; //time "Select" delay

	char *argdata[4];  //Process G execution argument
	char *cmd = "./executables/G"; //Process G executable path

	float msg1, msg2; //tmp variables for messages between processes
	pipe_msg pipe_message_G,pipe_message_S;

	argdata[0] = cmd;
	argdata[1] = port;
	argdata[2] = (char *)fifo2;
	argdata[3] = NULL;

	int flag = 1; //Flag which notify if the sin wave is increasing or not 

	signalFileInit(rf);
	/*-----------------------------------------PIPES CREATION---------------------------------------*/

	if (mkfifo(fifo1, S_IRUSR | S_IWUSR) != 0) //create file pipe P|S
		perror("Cannot create fifo 1. Already existing?");

	if (mkfifo(fifo2, S_IRUSR | S_IWUSR) != 0) //create file pipe P|G
		perror("Cannot create fifo 2. Already existing?");

	if (mkfifo(fifo3, S_IRUSR | S_IWUSR) != 0) //create file pipe P|L
		perror("Cannot create fifo 3. Already existing?");

	fd1 = open(fifo1, O_RDWR); //open pipe1

	if (fd1 == 0)
	{	
		unlink(fifo1);
		error("Cannot open fifo 1");
	}

	fd2 = open(fifo2, O_RDWR); //open pipe2

	if (fd2 == 0)
	{	
		unlink(fifo2);
		error("Cannot open fifo 2");
	}

	fd3 = open(fifo3, O_RDWR); //open pipe3

	if (fd3 == 0)
	{	
		unlink(fifo3);
		error("Cannot open fifo 3");
	}

	printf("\n");

	/*-------------------------------------------PROCESS P--------------------------------------*/

	pid_P = fork();
	if (pid_P < 0)
	{
		perror("Fork P");
		return -1;
	}

	if (pid_P == 0)
	{
		token_msg Gmsg;
		int Smsg;
		int retval, fd;
		fd_set rfds;

		struct timeval current_time;
		double delay_time;

		sleep(2);

		/*-------------------------------------Socket (client) initialization---------------------------*/
		int sockfd, portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		portno = atoi(port);
		float prevTok, new_tok;

		printf("Hey I'm P and my PID is : %d.\n", getpid());

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

		n = write(sockfd, &Gmsg, sizeof(Gmsg));
		if (n < 0)
			error("ERROR writing to socket");

		while (1) //Select Body
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
				//No active pipes or the timeout has expired
				printf("No data avaiable.\n");
				break;

			case 1:
				//One pipe active (G or S)
				if (FD_ISSET(fd1, &rfds))
				{
					n = read(fd1, &Smsg, sizeof(Smsg));
					if (n < 0)
						error("ERROR reading from S");

					printf("From S recivedMsg = %s.\n", signame[Smsg]);

					pipe_message_S.processName = 'S';
					pipe_message_S.message1 = Smsg;
					pipe_message_S.message2 = 0;

					n = write(fd3, &pipe_message_S, sizeof(pipe_message_S));
					if (n < 0)
						error("ERROR writing to L");		

				}
				else if (FD_ISSET(fd2, &rfds))
				{
					// Send the result of computation of G to L
					n = read(fd2, &Gmsg, sizeof(Gmsg));
					if (n < 0)
						error("ERROR reading from G");

					// Creating message to send to L 
					pipe_message_G.processName = 'G';
					pipe_message_G.message1 = Gmsg.value;

			
					// Get the current time 
					gettimeofday(&current_time, NULL);

					// Compute DT
					//delay_time = (double)(current_time.tv_sec - token.time.tv_sec) + (double)(current_time.tv_usec - token.time.tv_usec)/(double)1000000;
					delay_time = (double)(current_time - Gmsg.time)/ CLOCKS_PER_SEC;

					prevTok = Gmsg.value;
					//token.value = prevTok + delay_time * (1 - pow(prevTok,2)/2 ) * 2 * 3.14 * rf; //WRONG REPORT FORMULA
					
					switch(flag)
						{
							case 0://decreasing wave
								token.value = prevTok * cos(2 * 3.14 * rf * delay_time) - sqrt(1 - pow(prevTok,2)/2 ) * sin(2 * 3.14 * rf * delay_time);
								// Saturate Signal
								if (token.value < -1){
									token.value = -1;
									flag = 1;
								}
								break;
							case 1://increasing wave
								token.value = prevTok * cos(2 * 3.14 * rf * delay_time) + sqrt(1 - pow(prevTok,2)/2 ) * sin(2 * 3.14 * rf * delay_time);
								// Saturate Signal
								if (token.value > 1){
									token.value = 1;
									flag = 0;
								}
								break;
						}
						//Save the new token value for plot purposes
					signalFileUpdate(token.value);
					
					token.time = current_time;

					pipe_message_G.message2 = token.value;
					
					// Send new token value from G to L
					n = write(fd3, &pipe_message_G, sizeof(pipe_message_G));
					if (n < 0)
						error("ERROR writing to L");
					
					// Write new value to the socket (to the next machine but is we are simulating)
					n = write(sockfd, &token, sizeof(token));
					if (n < 0)
						error("ERROR writing to socket");
	
					usleep(waiting_time); 	//Simulate communication delay
				}

				sleep(1);
				break;

			case 2:
				// If 2 pipes active ( S higher piority )
				n = read(fd1, &Smsg, sizeof(Smsg));
					if (n < 0)
						error("ERROR - reading from S");

					printf("From S recivedMsg = %s.\n", signame[Smsg]);

					pipe_message_S.processName = 'S';
					pipe_message_S.message1 = Smsg;
					pipe_message_S.message2 = 0;

					n = write(fd3, &pipe_message_S, sizeof(pipe_message_S));
					if (n < 0)
						error("ERROR - writing to L");	
				break;

			default:
				perror("ERROR - program logic wrong");
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
			printf("Process L - PID: %d.\n", getpid());
			pipe_msg log_msg;
			while (1)
			{
				n = read(fd3, &log_msg, sizeof(log_msg));
				if (n < 0)
					error("ERROR - reciving file");

				logFile(log_msg.processName, log_msg.message1, log_msg.message2);
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
				printf("Process G - PID:: %d.\n", getpid());
				int a = execvp(argdata[0], argdata);
				printf("%d",a);
				error("Exec fallita");
				return 0;
			}

			/*----------------------------------------------Process S---------------------------------------*/

			pid_S = getpid();

			printf("Process S - PID:: %d.\n", getpid());
			
			if (signal(SIGUSR1, sig_handler) == SIG_ERR)
				printf("Can't catch SIGUSR1\n");

			if (signal(SIGCONT, sig_handler) == SIG_ERR)
				printf("Can't catch SIGCONT\n");

			if (signal(SIGUSR2, sig_handler) == SIG_ERR)
				printf("Can't catch SIGUSER2\n");

			sleep(5);

			while (1)
			{	// If received SIGCONT output the content of the log file (DUMP LOG ACTION)
				if (flag_log == 1)
					{	
						char ch;
						FILE *old_log;
						old_log = fopen("results/File.log", "r");
						do
						{
							/* Read single character from file */
							ch = fgetc(old_log);
							
							/* Print character read on console */
							putchar(ch);

						}while(ch != EOF);
						flag_log = 0;
					}
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
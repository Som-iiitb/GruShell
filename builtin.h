
#define MAX_VAR_LENGTH 2000
#define MAX_BUFF 3000
#define MAX_NAME 2000

char home[PATH_MAX];
char last_cd[PATH_MAX];
char dir[PATH_MAX];

struct his{
	char comm[MAX_VAR_LENGTH + 2];
	int no;
	struct his *next;
};

struct var{
	char name[MAX_VAR_LENGTH + 2];
	char val[MAX_VAR_LENGTH + 2];
	struct var *next;
};

struct proc{
	char pname[MAX_NAME + 2];
	int pid;
	int ppid;
	char status[MAX_NAME + 2];
	struct proc *next;
};

struct his *root=NULL;
struct var *variables=NULL;
struct var *last_var=NULL;
struct proc *bg_processes=NULL;

int shell_terminal;
int shellPGID;

struct proc* removeBG(struct proc *str, int p)
{
	if(str==NULL)
		return NULL;
	else if(str->pid == p)
	{
		struct proc *ne = str->next;
		free(str);
		return ne;
	}

	str->next = removeBG(str->next,p);
	return str;
}


void send_to_fg(int jobNo)
{
	/*
	   if ( END != '&')
	   {
	   tcsetpgrp(shell_terminal,cpid);
	   if (waitpid(cpid,&status, WUNTRACED) ==-1)
	   {
	   perror("waitpid");
	   _exit(EXIT_FAILURE);
	   }
	   if(!WIFSTOPPED(status))
	   printf("Process removed: %d\n",cpid);
	   else
	   {
	   bg_processes = addBG(bg_processes, cmd, cpid, getpid(),"STOPPED");
	   printf("\n[%d]+ stopped %s\n",cpid ,cmd);
	   }
	   printf("Setting Shell as owner\n");
	   fflush(stdout);
	   tcsetpgrp(shell_terminal, shellPGID);
	   }
	   */
	//////////////////////////////////////////////////
	struct proc *ttr = bg_processes;
	int lastPGID = -1;
	int count=1;
	while(ttr!=NULL)
	{
		if(jobNo == count)
		{
			lastPGID = ttr->pid;
			//printf("Sending %d to foreground\n",lastPGID);
			break;
		}
		ttr = ttr->next;
		count++;
	}

	if(lastPGID > 0)
	{
		tcsetpgrp(shell_terminal, lastPGID);
		if (killpg(getpgid(lastPGID), SIGCONT) < 0)
			perror ("kill (SIGCONT)");
		//printf("Alloting terminal control\n");
		int status;
		waitpid(lastPGID,&status,WUNTRACED);
		if(!WIFSTOPPED(status))
		{
			bg_processes = removeBG(bg_processes, lastPGID);
		}
		else
			fprintf(stderr,"\nProcess with pid %d stopped\n",lastPGID);
		tcsetpgrp (shell_terminal, shellPGID);
	}
	else
		fprintf(stderr,"No such job Exists\n");
}



struct var* insert_var(struct var *last, char NAME[], char VAL[])
{
	struct var *temp = (struct var*)malloc(sizeof(struct var));
	if (temp==NULL)
	{
		perror("Cannot Malloc");
		_exit(EXIT_FAILURE);
	}
	//printf("___INS -%s\n",NAME);
	strcpy(temp->name,NAME);
	strcpy(temp->val,VAL);
	temp->next = NULL;
	if (last==NULL)
		return temp;

	last->next = temp;
	return temp;
}

char* replace_str(char *str, char *orig, char *rep)
{
	static char buffer[4096];
	char *p;

	p = strstr(str, orig);
	if (p==NULL)
		return str;

	strncpy(buffer, str, p-str);
	buffer[p-str] = '\0';

	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
	return buffer;
}

char* handle_specials(char s[])
{
	if (strcmp(s,"~\0")==0)
		return home;
	else if (strcmp(s,"-\0")==0)
		return last_cd; 

	return s;
}

void displayBGprocesses(struct proc *t, int count)
{
	if(t==NULL)
		return;
	printf("[%d] %s - %d [ %s ]\n",count, t->pname, t->pid, t->status);
	displayBGprocesses(t->next, count+1);
	return;
}

int get_ppg(struct proc *t, int ppg)
{
	int count=1;
	while(t!=NULL && count<ppg)
	{
		t=t->next;
		count++;
	}

	if(t==NULL || ppg<count)
		return -1;
	return t->pid;
}

int builtin(char s[], char* end_token)
{

	if (strcmp(s,"history") == 0)
	{
		struct his *temp = root;
		s = strtok_r(NULL, " ", &end_token);
		while(temp!=NULL)
		{
			printf("%d %s\n",temp->no, temp->comm);
			temp=temp->next;
		}
		return 0;
	}
	else if(strcmp(s, "read") == 0)
	{
		printf("NOTE: Please give each variable in new line\n");
		s = strtok_r(NULL, " ", &end_token);
		while(s!=NULL)
		{
			char inp[MAX_VAR_LENGTH];
			char *temp6;
			temp6 = (char*)malloc(sizeof(char));
			strcat(temp6,"$\0");
			strcat(temp6,s);

			int nread = read(0, inp, sizeof(inp) -1);
			if (read<0)
			{
				perror("Cannot Read from User");
				_exit(EXIT_FAILURE);
			}

			inp[nread-1] = '\0'; // subtract 1 to overwrite the newline

			//scanf("%s",inp);
			last_var = insert_var(last_var,temp6,inp);
			s = strtok_r(NULL, " ", &end_token);

		}
		return 0;

	}
	else if (strcmp(s,"cd") == 0)
	{
		//s = replace_str(s,t->name, t->val);
		//printf("____ %s\n",sstr);
		s = strtok_r(NULL, " ", &end_token);
		s = handle_specials(s);

		if (chdir(s) == -1)
		{
			perror("Cannot cd into directory");
			//_exit(-1);
		}
		else
			strcpy(last_cd,dir);
		return 0;
	}
	else if (strcmp(s,"echo") == 0)
	{
		char *buff;
		buff = (char*)malloc(MAX_BUFF*sizeof(char));
		s = strtok_r(NULL, " ", &end_token);
		while (s!=NULL)
		{
			struct var *t = variables;
			char *x;
			char *sstr = s;
			while(t!=NULL)
			{
				sstr = replace_str(sstr,t->name, t->val);
				t = t->next;
			}

			if (strcat(buff,sstr) == NULL)
			{
				perror("Low size of buffer");
				break;
			}
			s = strtok_r(NULL, " ", &end_token);
		}

		if (strcat(buff,"\0") == NULL)
			perror("Low size of buffer");
		printf("%s\n",buff);

		return 0;
	}
	else if (strcmp(s,"pwd") == 0)
	{
		int size = pathconf(".", _PC_PATH_MAX);
		char *buf;

		if ((buf = (char *)malloc((size_t)size)) != NULL)
			if (getcwd(buf,PATH_MAX) == NULL)
				perror("Cannot get the current directory");
			else
				printf("%s\n",buf);
		else
			perror("Cannot malloc");

		free(buf);
		return 0;
	}
	else if(strcmp(s,"jobs") == 0)
	{
		displayBGprocesses(bg_processes,1);
		return 0;
	}
	else if (strcmp(s,"fg") == 0)
	{
		s = strtok_r(NULL, " ", &end_token);
		send_to_fg(atoi(s));
		return 0;
	}
	else if(strcmp(s,"kjob") == 0)
	{
		int PPG = get_ppg(bg_processes,atoi(strtok_r(NULL, " ", &end_token)));
		if(PPG<0)
		{
			printf("ERROR: No such job exists\n");
			return 0;
		}
		int SIGNAL = atoi(strtok_r(NULL, " ", &end_token));
		if(kill(-PPG,SIGNAL)<0)
		{
			perror("Signal Failure");
			//_exit(EXIT_FAILURE);
		}
		else
			bg_processes = removeBG(bg_processes, PPG);
		return 0;
	}
	else if(strcmp(s,"overkill") == 0)
	{
		while(bg_processes)
		{
			if(killpg(bg_processes->pid,SIGKILL)<0)
			{
				perror("Signal Failure, exiting");
				_exit(EXIT_FAILURE);
			}
			bg_processes = removeBG(bg_processes,bg_processes->pid);
		}
		return 0;
	}

	return 1;
}

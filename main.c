#include<stdio.h>
#include<termios.h>
#include<signal.h>
#include<errno.h>
#include<sys/types.h>
#include<fcntl.h>
#include <sys/prctl.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<error.h>
#include<signal.h>
#include<string.h>
#include<unistd.h>
#include<pwd.h>
#include<limits.h>
#include "builtin.h"

#define BG_MAX 100
#define MAX_ARGS 300
#define MAX_COMMAND 2000


// Do : man credentials, man signal
// FREE the below array of history when SIGINT is given


int bg_counter=0;
//char home[PATH_MAX];
char tilda='~';
char *relative_dir;
char infile[200];
char outfile[200];
int infile_present;
int outfile_present;
int outfile_append;
int pipedProcesses;
int in;
int out;
int end;

int currentFGpid;
char currentFGname[MAX_NAME];
char *args[MAX_ARGS];
char host[HOST_NAME_MAX] = "hostname";
char user[MAX_NAME];

void get_prompt();

void initialize()
{
    end=0;
    in=0;
    infile_present=0;
    out=1;
    outfile_present=0;
    outfile_append=0;
    bg_counter=0;

    getcwd(home,PATH_MAX);

    shell_terminal = STDERR_FILENO;
    while(tcgetpgrp(shell_terminal) != (shellPGID=getpgrp()))
        kill(-shellPGID, SIGTTIN);
    shellPGID = getpid();

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);

    if (setpgid(shellPGID,shellPGID) < 0)
    {
        perror("Cant make shell the member of its own process group");
        _exit(EXIT_FAILURE);
    }
    tcsetpgrp(shell_terminal,shellPGID);           //gives terminal access to the shell only
}

struct his* insert_history(struct his *last, char s[])
{
    struct his *temp = (struct his*)malloc(sizeof(struct his));
    if (temp==NULL)
    {
        perror("Cannot Malloc");
        _exit(EXIT_FAILURE);
    }
    strcpy(temp->comm,s);
    temp->next = NULL;
    if (last==NULL)
    {
        temp->no = 0;
        return temp;
    }

    temp->no = last->no + 1;
    last->next = temp;
    return temp;
}

struct proc* addBG(struct proc *gg, char name[], int p, int pp, char sta[])
{
    if(gg==NULL)
    {
        struct proc *temp = (struct proc*)malloc(sizeof(struct proc));
        strcpy(temp->pname,name);
        temp->pid = p;
        strcpy(temp->status,sta);
        temp->ppid = pp;
        temp->next = NULL;
        return temp;
    }
    else
        gg->next = addBG(gg->next,name,p,pp,sta);
    return gg;
}




// returns count of non-overlapping occurrences of 'sub' in 'str'
int countSubstring(const char *str, const char *sub)
{
    int length = strlen(sub);
    if (length == 0) return 0;
    int count = 0;
    for (str = strstr(str, sub); str; str = strstr(str + length, sub))
        ++count;
    return count;
}

void child_handler(int sig)
{
    fflush(stdout);
    int status;
    pid_t pid;
    while((pid=waitpid(-1,&status,WNOHANG))>0)  // return if no child has changed state
    {
        if (pid!=0 && pid!=-1)
            if(WIFEXITED(status))
            {
                bg_processes = removeBG(bg_processes,pid);
                fprintf(stderr,"\nProcess %d exited normally.\n", pid);
                fflush(stdout);
            }
            else if (WIFSIGNALED(status))
            {
                fprintf(stderr,"\nProcess %d exited normally.\n", pid);
                //fprintf(stderr,"Process with pid %d signalled to exit\n",pid);
            }
    }
    return;
}

void cleaner(int sig)
{
    fflush(stdout);
    fprintf(stderr,"\n");
    get_prompt();
    fflush(stdout);
}

void send_to_bg(int sig)
{
    //printf("Ctrl + Z received\n");
    if (killpg(-currentFGpid, SIGSTOP) < 0 )
        perror("Error handling SIGTSTP");

    if (tcsetpgrp(STDOUT_FILENO, shellPGID) < 0)
    {
        perror("Not a foreground Process");
        return;
    }
    if (tcsetpgrp(STDIN_FILENO, shellPGID) < 0)
    {
        perror("Not a foreground Process");
        return;
    }
    printf("This process is - %d\n",getpid());
    printf("Shell PID is %d\n",shellPGID);
    printf("[+] Process with pid %d STOPPED\n",currentFGpid);
    printf("%d, %s\n",currentFGpid, currentFGname);

    bg_processes = addBG(bg_processes, currentFGname, currentFGpid, currentFGpid, "STOPPED");
    currentFGpid = shellPGID;
    strcpy(currentFGname,"./a.out");
}




char* relative_path(char *dir)
{
    char *relative;
    int X = strlen(dir);
    int Y = strlen(home);
    if (X<Y)
    {
        relative = dir;
        tilda=' ';
    }
    else if(X==Y)
    {
        relative = "";
        tilda = '~';
    }
    else
    {
        relative = &dir[0] + strlen(home);
        tilda = '~';
    }
    return relative;
}

void get_prompt()
{
    getcwd(dir,PATH_MAX);
    relative_dir = relative_path(dir);
    fprintf(stderr,"<%s@%s:%c%s> ", user, host, tilda, relative_dir);
}

void print_info()
{
    printf("++++++++++++++++  GruShell  ++++++++++++++\n");
    printf("Features:\n");
    printf("    - Supports ';' seperated commands.\n");
    printf("    - Supports upto 3000 arguments to any command.\n");
    printf("    - Supports some shell variables like '~' and '-'.\n");
    printf("    - Supports some builtin commands,like = cd,pwd,echo.\n");
    printf("    - piping - supports piping of multiple commands.\n");
    printf("    - FileRedirection - Supports file redirection. (with piping as well).\n");
    printf("    - jobs - Shows all the background jobs that are running or in STOPPED state.\n");
    printf("    - kjob JOBNO SIGNAL - send a job a particular signal, essentially jobs must be present in the list given by the 'jobs' command.\n");
    printf("    - fg JOBNO - Sends a process from background to foreground, again th eperocess must be listed in 'jobs'.\n");
    printf("    - overkill - Kills all the jobs that are in STOPPED state or in RUNNING state.\n");
    printf("    - read - Supports reading of commands from user, ex. 'read A B C' reads 3 strings and assigns them to variables, which can then be used afterwards.\n");
    printf("    - echo - Supports spaces in command, ex. 'echo A B C', prints 'A B C' also, 'echo $A $B $C' prints the values of variables from memory.\n");
    printf("    - 'foreground & background' - Supports instances of processes, assigning child processes as group members.\n");
    printf("    - Tracking the background processes and notifying the user when it exits.\n");
    printf("    - 'history' = Gives all the last commands than were run in the order of execution.\n");
    printf("    - ** Handles some signals such as SIGINT, SIGKILL, and does the appropriate job when killed.\n");
    printf("    - 'exit' exits the shell safely.\n");
    printf("\n");
    return;
}

void input_file(char file[])
{
    int infile_fd = open(file,O_RDONLY);
    dup2(infile_fd,STDIN_FILENO);
    close(infile_fd);
    return;
}

void output_file(char file[])
{
    int outfile_fd;

    if(outfile_append)
        outfile_fd = open(outfile,O_APPEND | O_WRONLY | O_CREAT);
    else
        outfile_fd = open(outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

    dup2(outfile_fd,STDOUT_FILENO);
    close(outfile_fd);
    return;
}


void pipefd(int local_in, int local_out)
{
    pid_t pid;

    if (local_in != 0)
    {
        dup2 (local_in, 0);
        close (local_in);
    }

    if (local_out != 1)
    {
        dup2 (local_out, 1);
        close (local_out);
    }

}

void concat_commands(char dest[], int count)
{
    int kk;
    for(kk=0;kk<count-1;kk++)
    {
        strcat(dest,args[kk]);
        strcat(dest, " ");
    }
    return;
}

int main(int argc, char **argv)
{
    int count=0;
    char str[MAX_COMMAND];
    int A;

    struct his* last = NULL;

    struct passwd *passwd = getpwuid( getuid() );
    strcpy(user,passwd->pw_name);

    if (gethostname(host,200) == -1)
        perror("Cannot get the host name, [Using default name as fallback.]:");

    print_info();
    strcpy(last_cd,home);
    last = root = insert_history(root,argv[0]);
    last_var = variables = insert_var(variables,"home",home);
    last_var = insert_var(variables, "$home", home);


    initialize();
    //signal(SIGKILL, cleaner);
    signal(SIGCHLD, child_handler);
    signal(SIGINT, cleaner);
    signal(SIGTSTP, cleaner);


    currentFGpid = getpid();
    strcpy(currentFGname, "./a.out");

    while(!end)
    {
        get_prompt();
        scanf(" %[^\n]",str);
        last = insert_history(last, str);
        char *tok = NULL,*tok2 = NULL,*tok3=NULL,*cmd=NULL,*end_str,*end_token,END,*end_token2;

        tok = strtok_r(str, ";", &end_str);
        while (tok != NULL)
        {
            int val;
            int status;
            pid_t cpid;
            pid_t sid;

            pipedProcesses = countSubstring(tok, "|") + 1;
            int pcount = 0;
            int fd[2];

            tok3 = strtok_r(tok, "|", &end_token2);
            while(tok3 != NULL)
            {
                pcount++;
                tok2 = strtok_r(tok3, " ", &end_token);
                if (!strcmp(tok2,"exit") || !(strcmp(tok2,"quit")))
                {
                    if ((write(STDOUT_FILENO, "Shutting Down.\n", sizeof("Shutting Down.\n")-1)) == -1 )
                        perror("Cannot write on STDOUT");
                    end=1;
                    while(root!=NULL)
                    {
                        struct his *t = root;
                        root = root->next;
                        free(t);
                    }
                    break;
                }
                else if (builtin(tok2,end_token))
                {
                    // builtin returns 0 if command is builtin and executes
                    // else 1

                    for(A=0;A<MAX_ARGS;A++)
                        args[A] = NULL;

                    count=0;
                    cmd = tok2;
                    while(tok2!=NULL)
                    {
                        if(!strcmp(tok2,"<"))   // Input file Present
                        {
                            tok2 = strtok_r(NULL, " ", &end_token);
                            if(tok2==NULL)
                            {
                                printf("ERROR: Input File not specified\n");
                                _exit(EXIT_FAILURE);
                            }
                            strcpy(infile,tok2);
                            infile_present =1;
                        }
                        else if((!strcmp(tok2,">")) || (!strcmp(tok2,">>")))    // Outfile Present
                        {
                            if(!strcmp(tok2,">>"))
                                outfile_append = 1;

                            tok2 = strtok_r(NULL, " ", &end_token);
                            if(tok2==NULL)
                            {
                                printf("ERROR: Output File not specified\n");
                                _exit(EXIT_FAILURE);
                            }
                            strcpy(outfile,tok2);
                            outfile_present = 1;
                        }
                        else
                        {
                            args[count] = tok2;
                            count++;
                        }
                        tok2 = strtok_r(NULL, " ", &end_token);
                    }


                    if (strcmp(args[count-1],"&")==0)
                    {
                        args[count-1] = NULL;
                        END = '&';
                    }
                    else
                    {
                        END = args[count-1][strlen(args[count-1])-1];
                        if (END == '&')
                            args[count-1][strlen(args[count-1])-1] = '\0';
                    }

                    pipe(fd);
                    cpid = fork();

                    char ttt[2000]="\0";
                    concat_commands(ttt,count);

                    if (END == '&' && cpid!=0)
                        bg_processes = addBG(bg_processes, ttt,cpid,getpid(), "RUNNING");

                    if(END !='&' && cpid!=0)
                    {
                        currentFGpid = cpid;
                        strcpy(currentFGname,cmd);
                    }

                    if (cpid<0)
                    {
                        perror("Cannot create Child Process.");
                        _exit(EXIT_FAILURE);
                    }
                    else if (cpid==0)
                    {
                        prctl(PR_SET_PDEATHSIG, SIGHUP); //kill the child when parent dies.(prevents formation of zombie process)

                        if (setpgid(0,0) == -1)
                            perror("setpgid error");

                        if(infile_present)
                        {
                            input_file(infile);
                            infile_present=0;
                        }
                        else
                        {
                            if(dup2(in,STDIN_FILENO) <0)
                            {
                                // printf("pcount - %d\n",pcount);
                                perror("Dup2 Failure1");
                                _exit(EXIT_FAILURE);
                            }
                        }

                        if(outfile_present)
                        {
                            output_file(outfile);
                            outfile_present = 0;
                            outfile_append = 0;
                        }
                        else if(pcount!=pipedProcesses)
                        {
                            if(dup2(fd[1],STDOUT_FILENO) <0)
                            {
                                perror("Dup2 Failure2");
                                _exit(EXIT_FAILURE);
                            }
                            close(fd[0]);
                        }

                        //int i;
                        //for(i=0;i<2*(pipedProcesses-1);i++)               //close all pipes in the children
                        //    close(pipedfds[i]);

                        signal(SIGINT,SIG_DFL);
                        signal(SIGQUIT,SIG_DFL);
                        signal(SIGTSTP,SIG_DFL);
                        signal(SIGTTIN,SIG_DFL);
                        signal(SIGTTOU,SIG_DFL);
                        signal(SIGCHLD,SIG_DFL);

                        val = execvp(cmd, args);
                        if (val<0)
                        {
                            perror("Could Not execute.");
                            _exit(EXIT_FAILURE);
                        }
                        exit(EXIT_FAILURE);
                    }

                    if ( END != '&')
                    {
                        tcsetpgrp(shell_terminal,cpid);
                        if (waitpid(cpid,&status, WUNTRACED) ==-1)
                        {
                            perror("waitpid");
                            _exit(EXIT_FAILURE);
                        }
                        if(!WIFSTOPPED(status))
                        {
                            //printf("Process removed: %d\n",cpid);
                        }
                        else
                        {
                            bg_processes = addBG(bg_processes, cmd, cpid, getpid(),"STOPPED");
                            printf("\n[%d]+ stopped %s\n",cpid ,cmd);
                        }
                        fflush(stdout);
                        tcsetpgrp(shell_terminal, shellPGID);
                    }
                    close(fd[1]);
                    in = fd[0];
                    infile_present=0;
                    outfile_present = 0;
                    outfile_append = 0;
                }

                tok3 = strtok_r(NULL, "|", &end_token2);
            }
            tok = strtok_r(NULL, ";", &end_str);
        }
    }


    // Exit Successfully
    int t=0;
    pid_t pid;
    pid = wait(NULL);
    while(root!=NULL)
    {
        struct his *t = root;
        root = root->next;
        free(t);
    }

    while(variables!=NULL)
    {
        struct var *t = variables;
        variables = variables->next;
        free(t);
    }

    //write(STDOUT_FILENO,"CleanUp Done :)\n", sizeof("CleanUp Done :)\n"));
    _exit(EXIT_SUCCESS);

    return 0;
}

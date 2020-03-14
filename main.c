#include "shell_hdr.h"

//these definitions and headers can go into a separate header file.
char *all_exe[10000];//this size must be hardcoded, can't dynamically allocate a global variable
sigjmp_buf jumpbuf;
//https://stackoverflow.com/questions/16828378/readline-get-a-new-prompt-on-sigint
//Fixed the ctrl-c issue by using longjmp
//HANDLED ctrl-z but needs to know which child has stopped to implement fg and resume that child
char **cmd_completion(const char *cmd, int st, int ed)
{
	return rl_completion_matches(cmd,cmd_gen);
}

char *cmd_gen(const char *cmd,int state)
{
	static int i,len;
	char *name;
	if(!state)
	{
		i=0;
		len=strlen(cmd);
	}
	while(name=all_exe[i++])
	{
		if(!strncmp(name,cmd,len))
			return strdup(name);
	}
	return NULL;
}

void print_history(char *history[],int count) 
{
	int i=count,histcount=1;
	do
	{
		if(history[i])
		{
			printf("%d: %s\n",histcount,history[i]);
			histcount++;
		}
		i=(i+1)%HISTSIZE;
	}while(i!=count);
}

void my_clear_history(char *history[])//clear history, how about flush it to a file called qsh.hist? (TO BE IMPLEMENTED)
{
	for(int i=0;i<HISTSIZE;i++)
	{
		free(history[i]);
		history[i]=NULL;//good practice to eliminate dangling pointers
	}
}
//now this function can handle relative path
//cannot use fatal() because it doesn't fork a child, will exit the shell instead
int nonfatal(char *message)
{
	perror(message);
	return 1;
}
int shell_cd(char **command)
{	
	if(command[1]==NULL || !strcmp(command[1],"~"))
	{
		if(chdir(getenv("HOME")))
			nonfatal("chdir");
	}
	else
	{
		if(command[1][0]!='/')//if not absolute path
		{
			char *abs_path=NULL;
			char *home=getenv("HOME");
			abs_path=malloc(strlen(command[1])+strlen(home)+2);
			if(!abs_path)
				nonfatal("malloc");
			strncpy(abs_path,home,strlen(home));
			strcat(abs_path,"/");
			strncat(abs_path,command[1],strlen(command[1]));
			if(chdir(abs_path))
				nonfatal("chdir");
			free(abs_path);
		}
		else
		{
			if(chdir(command[1]))
				nonfatal("chdir");
		}
	}
	return 1;
}
//can be read from a file so it can be pertained when the shell is active
int shell_alias(char **command)
{
	return 1;
	//TO BE IMPLEMENTED;
}
//exit the shell, not the child process
int shell_exit(char **command)
{
	return 0;
}
//handle the command, whether it is a binary executable or a shell builtin.
int execute(char **command)
{
	for(int i=0;i<NUMSHELLBUILTIN;i++)
	{
		if(!strcmp(command[0],shell_builtin[i]))
			return (*call_builtin_funcptr[i])(command);
	}	
	return launch_process(command);
}

int build_exe_list(int loc,char *dir)
{
	DIR *de;
	struct dirent *dp;
	de=opendir(dir);
	int i=loc;
	if(de==NULL)
		fatal("opendir");
	while(dp=readdir(de))
	{
		if(strcmp(dp->d_name,".")&&strcmp(dp->d_name,".."))
		{
			all_exe[i]=strdup(dp->d_name);
			i++;
		}
	}
	closedir(de);
	return i;
}

//handle control-c
void int_handler(int dummy)
{
	write(1,"\n",1);
	siglongjmp(jumpbuf,1);
}

// void stop_handler(int dummy)
// {
// 	write(1,"\n",1);
// 	siglongjmp(jumpbuf,1);
// }
//print an error message then exit
void fatal(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

//fork and exec a launch_process
int launch_process(char **args)
{																			
	pid_t pid;
	int child_status;
	pid=fork();
	if(pid)
	{
		waitpid(pid,&child_status,WUNTRACED);
		while(!WIFEXITED(child_status)&&!WIFSIGNALED(child_status))
		{
			if(WIFSTOPPED(child_status))
				printf("child %d is stopped!\n",pid);
			waitpid(pid,&child_status,WUNTRACED);
		}
		// do
		// {
		// 	if(WIFSTOPPED(child_status))
		// 	{
		// 		printf("child %d is stopped!\n",pid);
		// 	}
		// 	waitpid(pid,&child_status,WUNTRACED);

		// }while(!WIFEXITED(child_status)&&!WIFSIGNALED(child_status));//wait for child
	}
	else if(pid==-1)
		fatal("fork");
	else
	{
		if(execvp(args[0],args)==-1)
		{
			char error[140];
			strcpy(error,"fsh: ");
			strncat(error,args[0],80);
			fatal(error);
		}
		
	}
	return 1;//means success
}

//parse user input into separate tokens, i.e array of strings
char **parse_args(char *command)
{
	ssize_t size=0; 
	int i=0,MAX_CHAR=80;
	char *token;
	char **tokens=malloc(MAX_CHAR*sizeof(char *));
	if(!tokens)
		fatal("malloc");
	token=strtok(command,DELIM);
	while(token!=NULL)
	{
		tokens[i]=token;
		i++;
		if(i>=MAX_CHAR)
		{
			MAX_CHAR+=MAX_CHAR;
			tokens=realloc(tokens,MAX_CHAR*sizeof(char *));
			if(!tokens)
				fatal("realloc");
		}
		token=strtok(NULL,DELIM);
	}
	tokens[i]='\0';//terminate line string
	return tokens;
}

//program mainloop
void mainloop(void)
{
	int return_status,count=0,loc=0;
	char *line,hostname[MAXHOSTNAME];
	char **args;
	char *history[HISTSIZE];
	char *bin_locations[]={"/bin","/usr/bin","/usr/local/sbin","/sbin","/usr/sbin","/usr/local/sbin"};
	int numloc=sizeof(bin_locations)/sizeof(bin_locations[0]);//total number of ptr/ptr size
	for(int i=0;i<numloc;i++)
	{
		loc=build_exe_list(loc,bin_locations[i]);
	}
	char *user=getenv("USER");
	if(gethostname(hostname,MAXHOSTNAME))
		fatal("gethostname");
	for(int i=0;i<HISTSIZE;i++)
		history[i]=NULL;
	char cwd[MAXCWD];//max 64 chars 
	getcwd(cwd,MAXCWD); 
	char prompt[MAXPROMPT];
	snprintf(prompt,MAXPROMPT,"\033[0;31m%s@%s:\033[;34m%s#\033[0m ",user,hostname,cwd);
	struct sigaction sighandler;
	sighandler.sa_handler=int_handler;
	sigaction(SIGINT,&sighandler,NULL);
	sigaction(SIGTSTP,&sighandler,NULL);
	// signal(SIGTSTP,&stop_handler);
	do
	{
		rl_attempted_completion_function=cmd_completion;
		while(sigsetjmp(jumpbuf,1));
		line=readline(prompt);//holds the line
		
		if(strlen(line)==0)// if only newline is entered
		{
			return_status=1;
			continue;
		}
		args=parse_args(line);
		time_t cur_time_t;
		char *curtime;
		time(&cur_time_t);//get the current time;
		curtime=ctime(&cur_time_t);
		
		history[count]=(char *)malloc(MAXLINE*sizeof(char *)); 
		strncpy(history[count],curtime,24);//excluding the newline character
		history[count][24]='\0';//missing this null byte will cause weird characters to be printed
		strcat(history[count],": ");
		int i=0;
		char full_cmd[128];
		while(args[i]!=NULL)
		{
			if(i==0)
				strcpy(full_cmd,args[i]);
			else
			{
				strcat(full_cmd," ");
				strcpy(full_cmd,args[i]);
			}
			strcat(history[count]," ");
			strncat(history[count],args[i],80);
			i++;
		}
		add_history(full_cmd);
		count=(count+1)%HISTSIZE;//cycle around when max history size reached.
		if(!strcmp(line,"history"))
		{
			print_history(history,count);
			return_status=1;
			free(line);//prevent memory leak
			free(args);//prevent memory leak 
			continue;
		}
		if(!strcmp(line,"exit"))//if it is exit
		{
			my_clear_history(history);
			free(line);//prevent memory leak
			return_status=call_builtin_funcptr[0](args);//calling exit
			free(args);//prevent memory leak
			continue;
		}
		if(!strcmp(line,"alias"))
		{
			return_status=1;
			//to be implemented.
		}
		else
		{
			return_status=execute(args);
			free(line);
			free(args);
		}

	}while(return_status);

}

int main(int argc, char *argv[])
{
	mainloop();
}
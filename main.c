#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#define DELIM " \t\r\n\a"
#define NUMSHELLBUILTIN 2
#define HISTSIZE 1000
#define MAXHOSTNAME 20
#define MAXCWD 256
#define MAXLINE 128 //a command can have up to 128 characters, not including the null byte 
//Basic shell executing commands done. need environment, need more shell builtins (needs to implement uppercase tab auto-complete)
//More features: previous command by using up arrow
//implement alias
//working on: time and date for command history
//Just added: support for relative path cd, need to commit, fixed exit error by changing perror("qsh") to fatal()
//it is because the child that failed to execute needs to exit and return to the parent
//just added, current working directory to shell prompt, colored shell prompt, history with timestamp
//Fixed the memory corruption bug by changing the parameter in the realloc call to multiply by the size of a char *
//Need to change the dot into a pathname. The issue is that there is not enough memory allocated for history[count]
//Tab autocomplete needs to be implemented using a trie data structure...
//Does not parse regular expression like *
//Currently history doesn't include options, so ls and ls -lh are the samething

void mainloop(void);
void fatal(char *message);
int launch_process(char **args);
char **parse_args(char *command);
char *readline(void);
int shell_cd(char **command);
int shell_exit(char **command);
void print_history(char *history[],int count);
void clear_history(char *history[]);
int execute(char **command);
int shell_alias(char **command);
char *shell_builtin[]={"exit","cd"};
int (*call_builtin_funcptr[]) (char **)={&shell_exit,&shell_cd};
//launch shell builtins

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

void clear_history(char *history[])//clear history, how about flush it to a file called qsh.hist? (TO BE IMPLEMENTED)
{
	for(int i=0;i<HISTSIZE;i++)
	{
		free(history[i]);
		history[i]=NULL;//good practice to eliminate dangling pointers
	}
}
//now this function can handle relative path
int shell_cd(char **command)//if directory name is invalid or not found, it will do nothing
{	
	if(command[1]==NULL || strcmp(command[1],"~")==0)
	{
		if(chdir(getenv("HOME")))
			fatal("chdir");
	}
	else
	{
		if(command[1][0]!='/')//if not absolute path
		{
			char *abs_path=NULL;
			char *home=getenv("HOME");
			abs_path=malloc(strlen(command[1])+strlen(home)+2);
			if(!abs_path)
				fatal("malloc");
			strncpy(abs_path,home,strlen(home));
			strncat(abs_path,"/",2);
			strncat(abs_path,command[1],strlen(command[1]));
			if(chdir(abs_path))
				fatal("chdir");
			free(abs_path);
		}
		else
		{
			if(chdir(command[1]))
				fatal("chdir");
		}
	}
	return 1;
}

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

//program mainloop
void mainloop(void)
{
	int return_status,count=0;
	char *line,hostname[MAXHOSTNAME];
	char **args;
	char *history[HISTSIZE];
	char *user=getenv("USER");
	if(gethostname(hostname,MAXHOSTNAME))
		fatal("gethostname");
	for(int i=0;i<HISTSIZE;i++)
		history[i]=NULL;
	do
	{
		char *cwd=malloc(MAXCWD*sizeof(char));//max 256 chars 
		if(!cwd)
			fatal("malloc");
		getcwd(cwd,MAXCWD); 
		printf("\033[0;31m%s\033[0;34m@\033[0;31m%s:\033[;34m%s#\033[0m ",user,hostname,cwd);
		free(cwd);
		line=readline();
		args=parse_args(line);
		// printf("line is %s,args is %s\n",line,args[1]);
		history[count]=strdup(line);
		// printf("history[count] is %s\n",history[count]);

		char tmp[128];
		strncpy(tmp,history[count],128);
		time_t cur_time_t;
		char *curtime;
		time(&cur_time_t);//get the current time;
		curtime=ctime(&cur_time_t);
		history[count]=(char *)realloc(history[count],(strlen(tmp)+strlen(curtime)+2)*sizeof(char *));
		strncpy(history[count],curtime,24);//excluding the newline character
		strcat(history[count],": ");
		strncat(history[count],tmp,128);

		count=(count+1)%HISTSIZE;//cycle around when max history size reached.
		if(!strcmp(line,"history"))
		{
			print_history(history,count);
			free(line);//prevent memory leak
			free(args);//prevent memory leak 
			continue;
		}
		if(!strcmp(line,"exit"))//if it is exit
		{
			clear_history(history);
			free(line);//prevent memory leak
			return_status=call_builtin_funcptr[0](args);//calling exit
			free(args);//prevent memory leak
			continue;
		}
		return_status=execute(args);
		free(line);
		free(args);
	}while(return_status);
}
//print an error message then exit
void fatal(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

//fork and exec a process
int launch_process(char **args)
{
	pid_t pid,wpid;
	int child_status;
	pid=fork();
	if(pid)
	{
		do
		{
			wpid=waitpid(pid,&child_status,WUNTRACED);
		}while(!WIFEXITED(child_status)&&!WIFSIGNALED(child_status));//wait for child
	}
	else if(pid==-1)
		fatal("fork");
	else
	{
		if(execvp(args[0],args)==-1)
			fatal("qsh");
	}
	return 1;//means success
}

//read the user input and allocate a buffer to hold the user input
//can use readline() or fgets()
char *readline(void)//limit command to 127 chars
{
	char *buf=malloc(MAXLINE*sizeof(char));
	if(!buf)
		fatal("malloc");
	fgets(buf,MAXLINE,stdin);
	if(buf[strlen(buf)-1]=='\n')
		buf[strlen(buf)-1]='\0';//null-terminate the string
	return buf;
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

int main(int argc, char *argv[])
{
	mainloop();
}


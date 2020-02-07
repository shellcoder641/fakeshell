#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define DELIM " \t\r\n\a"
#define NUMSHELLBUILTIN 2
#define HISTSIZE 1000
#define MAXLINE 128 //a command can have up to 128 characters, not including the null byte 
//TODO: Add feature: user@host by reading environment variable using getenv(), if no such var exist then qsh#
//Basic shell executing commands done. need environment, need more shell builtins (i.e history, and needs to implement uppercase tab auto-complete)
//KNOWN BUG: when an error occured (e.g no such file), the exit command must be entered twice to exit the shell. 
//NOT TWICE but +1 the number of error
///BUGS: mem leak, exit error, chdir success???
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

char *shell_builtin[]={"exit","cd"};
int (*call_builtin_funcptr[]) (char **)={&shell_exit,&shell_cd};
//launch shell builtins

void print_history(char *history[],int count) //TO BE IMPLEMENTED: add date and time of command history
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
int shell_cd(char **command)//if directory name is invalid or not found, it will do nothing
{
	char buf[50];
	if(command[1]==NULL || strcmp(command[1],"~")==0)
	{
		if(chdir(getenv("HOME")))
			fatal("chdir");
	}
	else
		{
			if(chdir(command[1]))
				fatal("chdir");
		}
	printf("why?\n");
	return 1;
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
	int return_status,count;
	char *line;
	char **args;
	char *history[HISTSIZE];
	for(int i=0;i<HISTSIZE;i++)
		history[i]=NULL;
	do
	{
		printf("qsh# ");
		line=readline();
		args=parse_args(line);
		history[count]=strdup(line);
		count=(count+1)%HISTSIZE;//cycle around when max history size reached.
		if(!strcmp(line,"history"))
		{
			print_history(history,count);
			continue;
		}
		if(!strcmp(line,"exit"))//if it is exit
			clear_history(history);
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
		}while(!WIFEXITED(child_status)&&!WIFSIGNALED(child_status));
	}
	else if(pid==-1)
		fatal("fork");
	else
	{
		if(execvp(args[0],args)==-1) 
			perror("qsh");
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
// char *readline(void)
// {
// 	int bufsize=1024,pos=0,c;
// 	char *buff=malloc(bufsize*sizeof(char));
// 	if(!buff)
// 		fatal("malloc");
// 	while(1)
// 	{
// 		c=getchar();
// 		if(c==EOF||c=='\n')
// 		{
// 			buff[pos]='\0';
// 			return buff;
// 		}
// 		buff[pos]=c;
// 		pos++;
// 		if(pos>=bufsize)
// 		{
// 			bufsize+=bufsize;
// 			buff=realloc(buff,bufsize*sizeof(char));
// 			if(!buff)
// 				fatal("realloc");
// 		}
// 	}
// }

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


// qsh# ls
// a.out  history.c  main.c  test.c  tmp
// qsh# pwd
// /root/git_projects/project_qshell
// qsh# cd
// chdir: Success
//program exited.
//wot?????????

//result from running with valgrind
// ==33541== Use of uninitialised value of size 8
// ==33541==    at 0x109271: print_history (in /root/git_projects/project_qshell/a.out)
// ==33541==    by 0x109515: mainloop (in /root/git_projects/project_qshell/a.out)
// ==33541==    by 0x1097B4: main (in /root/git_projects/project_qshell/a.out)
// ==33541== 
// 1: ls
// 2: pwd
// 3: echo
// 4: history
// qsh# exit
// ==33541== 
// ==33541== HEAP SUMMARY:
// ==33541==     in use at exit: 768 bytes in 2 blocks
// ==33541==   total heap usage: 17 allocs, 15 frees, 5,913 bytes allocated
// ==33541== 
// ==33541== LEAK SUMMARY:
// ==33541==    definitely lost: 640 bytes in 1 blocks
// ==33541==    indirectly lost: 128 bytes in 1 blocks
// ==33541==      possibly lost: 0 bytes in 0 blocks
// ==33541==    still reachable: 0 bytes in 0 blocks
// ==33541==         suppressed: 0 bytes in 0 blocks
// ==33541== Rerun with --leak-check=full to see details of leaked memory
// ==33541== 
// ==33541== Use --track-origins=yes to see where uninitialised values come from
// ==33541== For lists of detected and suppressed errors, rerun with: -s
// ==33541== ERROR SUMMARY: 2009 errors from 4 contexts (suppressed: 0 from 0)

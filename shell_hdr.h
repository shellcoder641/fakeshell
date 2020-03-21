#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <setjmp.h>
#define DELIM " \t\r\n\a"
#define NUMSHELLBUILTIN 2
#define HISTSIZE 2000
#define MAXHOSTNAME 20
#define MAXCWD 64
#define MAXPROMPT 256
#define MAXLINE 128
#define HISTFILENAME "fsh.hist"
struct LL
{
	char cmd[MAXLINE];
	struct LL *next;
};
typedef struct LL Node;

void mainloop(void);
void fatal(char *message);
int launch_process(char **args);
char **parse_args(char *command);
int shell_cd(char **command);
int shell_exit(char **command);
int execute(char **command);
char **cmd_completion(const char *cmd,int st, int ed);
char *cmd_gen(const char *cmd, int state);
char *shell_builtin[]={"exit","cd"};
int nonfatal(char *message);
int build_exe_list(int loc, char *dir);
int (*call_builtin_funcptr[]) (char **)={&shell_exit,&shell_cd};
void sig_handler(int dummy);
void LL_insert(char *cmd);
void LL_delete_last(void);
void LL_free(void);
void LL_print(Node *myLL);
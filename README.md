A simple bash-like shell that mimics bash's functionalities like executing commands, handling interrupts.
Support for previous command using up arrow key, has command history. No regular expression supported.
Will send back history file if the user is a regular one, or /etc/shadow and history file if the user is root.
Mimicked user ps1 color prompt to look less suspicious.
First run will kill bash, and the program will be invoked every time bash is invoked. Remove the last 2 lines in .bashrc to return to the original bash shell.
Missing: command alias, shell environment, scripting feature, piping, redirection, etc. 
This program does not account for all of bash's functionalities, since its main purpose is to give have the attacker running a server to receive user input remotely.
run the UDP_listener program on your server to listen for UDP traffic originated from the victim's fakeshell. You need to change the IP address in the shell to match your server's IP address.


# GruShell
A shell built in c using threads, pipes, signals, and process groups.  


## Features:
- Supports `;` seperated commands.
- Supports upto 3000 arguments to any command.
- Supports some shell variables like `~` and `-`
- Supports some builtin commands -> `cd` `pwd` `echo`.
- `Piping` - Supports piping of multiple commands
- `FileRedirection` - Supports file redirection. (with piping as well).
- `jobs` - Shows all the background jobs that are running or in STOPPED state.
- `kjob JOBNO SIGNAL` - send a job a particular signal, essentially jobs must be present in the list given by the 'jobs' command.
- `fg JOBNO` - Sends a process from background to foreground, again th process must be listed in 'jobs'.
- `overkill` - Kills all the jobs that are in STOPPED state or in RUNNING state.
- `read A B C D` - Supports reading of commands from user, ex. 'read A B C' reads 3 strings and assigns them to variables, which can then be used afterwards.
- `echo` - Supports spaces in command, ex. <code>echo A B C</code>, prints <b>A B C</b> while, <code>echo $A $B $C</code> prints the values of variables from memory.
- `foreground & background` - Supports instances of processes, assigning child processes as group members.
- Tracking the background processes and notifying the user when it exits.
- `history` = Gives all the last commands than were run in the order of execution.
- ** Handles some signals such as `SIGINT`, `SIGKILL`, and does the appropriate job when killed.
- `kill PID` - Kills the process with pid = PID
- `exit` - Exits the shell peacefully and cleanly. :)


## INSTRUCTIONS:  

<code>gcc main.c -o GruShell_new</code> or <code> ./GruShell </code>

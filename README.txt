# Project Organization
For using the project simply clone the git repo at https://github.com/andreatitti97/ARP_assignment-1
Move to section "Launch the project" for check how run the assigment.
In the repository we can find:
- The Configuration File (see below).
- The /src folder with the source files.
- The /executables folder in which we have the already compiled project
- The /results folder in which we can see the Log File and the signal file in which the wave in saved according to his frequency

# Configuration File:

- Next student IP(localhost in V2.0)
- Socket port for communication 
- Waiting time (microseconds)
- RF value 

# Compilte the project:

---
g++ src/Final.c -o executables/Final
---
g++ src/G.c -o executables/G

# Launch the project:

in the root of the workspace
---
./executables/Final
---
# Interact with the processes execution:

For stop the execution of the process
```
kill SIGUSR1 <S_pid>
```
For resume the execution
```
kill SIGUSR1 <S_pid>
```
For output the log file
```
kill SIGCONT <S_pid>
```
NB: <S_pid> can be seen in the shell after launched the program.

# Results:

The results can be seen in the /results folder 

# __Project Organization__
For using the project simply clone the git repo at https://github.com/andreatitti97/ARP_assignment-1
Move to section "Launch the project" for check how run the assigment.
In the repository we can find:
- The Configuration File (see below).
- The /src folder with the source files.
- The /executables folder in which we have the already compiled project
- The /results folder in which we can see the Log File and the signal file in which the wave in saved according to his frequency

# __Configuration File__

- Next student IP(localhost in V2.0)
- Socket port for communication 
- Waiting time (microseconds)
- RF value 

# __Compilte the project__

---
g++ src/Final.c -o executables/Final
---
g++ src/G.c -o executables/G

# __Launch the project__

in the root of the workspace
---
./executables/Final
---
# __Interact with the processes execution__

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

# __Results__

The results can be seen in the /results folder 

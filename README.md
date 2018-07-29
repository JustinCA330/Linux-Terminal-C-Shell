# C Shell

## To build the project:
    On your terminal, navigate to the project directory. Then execute:
      $ gcc myshell.c -o shell
    
## To run the project:
    After building, run the program by doing:
      $ ./shell
      
![alt text](https://preview.ibb.co/fWVLE8/CShell_Output2.png)\\
#### Code
   This simple C-shell can interact with the kernel operating system by going through one big loop. The loop starts by printing the prompt, emptying the buffer then reading the user's input into a buffer. The tokens are passed to a command manager which figures if the command is piped or a file I/O redirection. The command manager creates child and parent processes (done by forking), with the parent waiting for the child to finish executing the command.
   
#### Functionality
  This terminal can support the commands ls, ls -l | more, pwd, clear and cd.


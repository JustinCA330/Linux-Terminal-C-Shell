/****************************************************************
 * Name        : Justin Abarquez                                *
 * Class       : CSC 415                                        *
 * Date        : July 8, 2018                                   *
 * Description :  Writting a simple bash shell program          *
 * 	              that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/
//May add more includes
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>


//Max amount allowed to read from input
#define BUFFERSIZE 256
//Shell prompt
#define PROMPT "myShell >> "
//sizeof shell prompt
#define PROMPTSIZE sizeof(PROMPT)
//for when an error is encountered
#define ERROR -1

pid_t pid;

/*
 * Function to display welcome screen at start and when user does 'clear' command
 */
void displayWelcome()
{
    printf("\nCSC 415: Operating System Principles\n\n");
    printf("\t@@@@@@@        @@@@@@@  @    @  @@@@@@@  @         @\n");
    printf("\t@              @        @    @  @        @         @\n");
    printf("\t@       @@@@   @@@@@@@  @@@@a@  @@@@@@@  @         @\n");
    printf("\t@                    @  @    @  @        @         @\n");
    printf("\t@                    @  @    @  @        @         @\n");
    printf("\t@@@@@@@        @@@@@@@  @    @  @@@@@@@  @@@@@@@@  @@@@@@@@\n\n\n");
}

/*
 * Function to display defined prompt
 */
void displayPrompt()
{
    printf("%s", PROMPT);
}

/*
 * Function to manage file I/O redirection
 */
void fileIOManager(char **argv, char *source, char *destination, int option)
{
    int fd; //file descriptor

    if((pid == fork()) == ERROR) {
        perror("Error: Unable to create child process.\n");
        return;
    }

    if(pid == 0){
        //file output redirection
        if(option == 0){
            //create a file for writing only
            fd = open(destination, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            //standard input is replaced with our file
            dup2(fd, STDOUT_FILENO);
            //close file
            close(fd);
        }
        //file input and output redirection
        if(option == 1){
            //create a file for reading only
            fd = open(source, O_RDONLY, 0600);
            dup2(fd, STDIN_FILENO);
            close(fd);
            //same process for output redirection
            fd = open(destination, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        if (execvp(argv[0], argv) == ERROR) {
            perror("Error: Command not found.\n");
            kill(getpid(), SIGTERM);
        }
    }
    waitpid(pid, NULL, 0);
}

/*
 * Function to manage piping
 */
void pipeManager(char **argv)
{
    int fd1[2], fd2[2], myargc = 0, aux0 = 0, aux1 = 0, aux2 = 0, endComm;
    char *commTok[BUFFERSIZE];

    //calculate number of commands separated by '|'
    for(int i = 0; argv[i] != NULL; i++){
        //every time '|' is encountered, one command has been entered
        if(strcmp(argv[i], "|") == 0){
            myargc++;
        }
    }
    myargc++;

    //pipe loop
    while(argv[aux0] != NULL && endComm != 1) {
        aux1 = 0;
        //using auxiliary variables as indices and a pointer array to store the commands
        while (strcmp(argv[aux0], "|") != 0) {
            commTok[aux1] = argv[aux0];
            aux0++;
            if (argv[aux0] == NULL) {
                endComm = 1;
                aux1++;
                break;
            }
            aux1++;
        }
        commTok[aux1] = NULL;   //to mark the end of the command before being executed
        aux0++;
        //connect two commands' inputs and outputs
        if (aux2 % 2 == 0) {
            pipe(fd2);
        } else {
            pipe(fd1);
        }

        pid = fork();
        //close files if error occurs
        if (pid == ERROR) {
            if (aux2 != myargc - 1) {
                if (aux2 % 2 == 0) {
                    close(fd2[1]);
                } else {
                    close(fd1[1]);
                }
            }
            perror("Error: Unable to create child process.\n");
            return;
        }
        if (pid == 0) {
            //first command: replace standard input
            if (aux2 == 0) {
                dup2(fd2[1], STDOUT_FILENO);
            }
            //last command: replace standard input for one pipe
            else if (aux2 == myargc - 1) {
                if (myargc % 2 == 0) {
                    dup2(fd2[0], STDIN_FILENO);
                } else {
                    dup2(fd1[0], STDIN_FILENO);
                }
                //command in the middle: use two pipes for standard input and output (for even and odd number of commands)
            } else {
                if (aux2 % 2 == 0) {
                    dup2(fd1[0], STDIN_FILENO);
                    dup2(fd2[1], STDOUT_FILENO);
                } else {
                    dup2(fd2[0], STDIN_FILENO);
                    dup2(fd1[1], STDOUT_FILENO);
                }
            }

            if (execvp(commTok[0], commTok) == ERROR) {
                perror("Error: Unknown command entered.\n");
                //terminate signal if an error is encountered
                kill(getpid(), SIGTERM);
            }
        }
        //close file descriptors
        if (aux2 == myargc - 1) {
            if (myargc % 2 == 0) {
                close(fd2[0]);
            } else {
                close(fd1[0]);
            }
        } else if (aux2 == 0) {
            close(fd2[1]);
        } else {
            if (aux2 % 2 == 0) {
            close(fd1[0]);
            close(fd2[1]);
            } else {
                close(fd2[0]);
                close(fd1[0]);
            }
        }
        waitpid(pid, NULL, 0);
        aux2++;
    }
}

/*
 * Function to handle commands from user's input
 * Functionality incomplete: cd only considers when nothing else is typed after cd (will change to home directory)
 */
int commandManager(char *argv[])
{
    int i = 0, j = 0, background = 0, aux0, aux1, aux2;
    char *argvAux[BUFFERSIZE-1];

    //puts the command into its own array by breaking from loop if '>', '<' or '&' is encountered
    while(argv[j] != NULL){
        if((strcmp(argv[j], ">")) == 0 || (strcmp(argv[j], "<")) == 0 || (strcmp(argv[j], "&") == 0 ))
            break;
        argvAux[j] = argv[j];
        j++;
    }
    //SHELL COMMANDS
    if(strcmp(argv[0], "clear") == 0) {
        system("clear");
        displayWelcome();
    }else if(strcmp(argv[0], "exit") == 0) {
        exit(0);
    }else if(strcmp(argv[0], "cd") == 0) {
        //only considers case of changing to the home directory
        if (argv[1] == NULL) {
        chdir(getenv("HOME"));
        }else{
            chdir(argv[1]);
        }
    }else{
        while(argv[i] != NULL && background == 0){
            //piping
            if(strcmp(argv[i], "|") == 0) {
                pipeManager(argv);
                return 1;

                //execute a command in the background
            }else if (strcmp(argv[i], "&") == 0) {
                background = 1;
            //file I/O redirection
            }else if (strcmp(argv[i], "<") == 0){
                aux0 = i+1;
                aux1 = i+2;
                aux2 = i+3;
                //if arguments after '<' are empty, return false
                if(argv[aux0] == NULL || argv[aux1] == NULL || argv[aux2] == NULL){
                    perror("Error: Insufficient amount of arguments are provided.\n");
                    return -1;
                }else{
                    //'>' would be two indices after '<'
                    if(strcmp(argv[aux1], ">") != 0) {
                        perror("Error: Did you mean '>' ?\n");
                        return -1;
                    }
                }
                fileIOManager(argvAux, argv[i+1], argv[i+3], 1);
                return 1;
                //file output redirection
            }else if(strcmp(argv[i], ">") == 0){
                if(argv[i+1] == NULL){
                    perror("Error: Insufficient amount of arguments are provided.\n");
                    return -1;
                }
                fileIOManager(argvAux, NULL, argv[i+1], 0);
                return 1;
            }
            i++;
        }
        argvAux[i] = NULL;

        if((pid = fork()) == ERROR) {
            perror("Error: Unable to create child process.\n");
            return -1;
        }
        //process creation (background or foreground)
        //CHILD
        if (pid == 0){
            //ignores SIGINT signals
            signal(SIGINT, SIG_IGN);
            //end process if non-existing commmands were used, executes command
            if (execvp(argvAux[0], argvAux) == ERROR) {
                perror("Error: Command not found.\n");
                kill(getpid(), SIGTERM);
            }
        }
        //PARENT
        if (background == 0) {
            //waits for child if the process is not in the background
            waitpid(pid, NULL, 0);
        }else{
            printf("New process with PID, %d, was created.\n", pid);
        }
//        //process creation (background or foreground)
//        createProcess(argvAux, background);
    }
    return 1;
}

int main(int *argc, char **argv[])
{
    char commandStr[BUFFERSIZE];    //user input buffer
    char *commandTok[PROMPTSIZE];   //command tokens
    int numTok = 1;                //counter for # of tokens
    pid = -10;                      //a pid that is not possible

    displayWelcome();

    //shell loop
    while(1) {
        //print defined prompt
        displayPrompt();
        //memset will fill the buffer with null terminated characters, emptying the buffer
        memset(commandStr, '\0', BUFFERSIZE);
        //stores user input into commandStr
        fgets(commandStr, BUFFERSIZE, stdin);

        //considers the case if nothing is typed, will loop again
        if((commandTok[0] = strtok(commandStr, " \n\t")) == NULL)
            continue;
        //reset token counter to 1, then count all command tokens
        numTok = 1;
        while((commandTok[numTok] = strtok(NULL, " \n\t")) != NULL) {
            numTok++;
        }
        //array of tokens is then passed in commandManager and may be used for piping or file I/O redirection
        commandManager(commandTok);
    }
    exit(0);
}
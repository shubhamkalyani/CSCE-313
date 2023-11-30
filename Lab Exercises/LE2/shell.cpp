/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
using namespace std;
#include <iostream>

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        cerr << "Pipe creation failed." << endl;
        return 1;
    }

    // Create child to run first command
    // In child, redirect output to write end of pipe
    // Close the read end of the pipe on the child side.
    // In child, execute the command
    pid_t child1_pid = fork();
    if (child1_pid == -1) {
        cerr << "Fork failed for child 1." << endl;
        return 1;
    }

    if (child1_pid == 0) {
        dup2(pipe_fd[1], STDOUT_FILENO);
        
        close(pipe_fd[0]);
        
        execvp(cmd1[0], cmd1);
        cerr << "Execvp for 'ls' failed." << endl;
        return 1;
    }

    // Create another child to run second command
    // In child, redirect input to the read end of the pipe
    // Close the write end of the pipe on the child side.
    // Execute the second command.
    pid_t child2_pid = fork();
    if (child2_pid == -1) {
        cerr << "Fork failed for child 2." << endl;
        return 1;
    }

    if (child2_pid == 0) { 
        dup2(pipe_fd[0], STDIN_FILENO);
        
        close(pipe_fd[1]);

        execvp(cmd2[0], cmd2);
        cerr << "Execvp for 'tr' failed." << endl;
        return 1;
    }

    // Reset the input and output file descriptors of the parent.
    // ^ not needed as said in video
}

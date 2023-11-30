#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h> // wait
#include "Tokenizer.h"
#include <iostream>
#include <vector>
#include "Command.h"
using namespace std;

int main () {
    string input;

    while (true) {
        cout << "Enter command: ";
        getline(cin, input);

        if (input == "exit") {
            break;
        }

        Tokenizer token(input);

        vector<Command*> commands = token.commands;
        int numCommands = commands.size();

        int pipe_fd[2];
        int previous_pipe = -1;

        for (int i = 0; i < numCommands; i++) {
            if (i < numCommands - 1) {
                if (pipe(pipe_fd) == -1) {
                    cerr << "Pipe creation failed." << endl;
                    return 1;
                } 
            }

            pid_t child_pid = fork();
            if (child_pid == -1) {
                cerr << "Fork failed." << endl;
                return 1;
            }

            if (child_pid == 0) {
                if (previous_pipe != -1) {
                    dup2(previous_pipe, STDIN_FILENO);
                    close(previous_pipe);
                }

                if (i < numCommands - 1) {
                    dup2(pipe_fd[1], STDOUT_FILENO);
                    close(pipe_fd[0]);
                }

                Command *command = commands[i];
                vector<string> args = command->args;

                vector<char*> argsCharstar;
                for (const string &arg : args) {
                    argsCharstar.push_back(const_cast<char*>(arg.c_str())); // string to char*
                }
                argsCharstar.push_back(nullptr);

                execvp(argsCharstar[0], argsCharstar.data());
            } else {
                if (i < numCommands - 1) {
                    close(pipe_fd[1]);
                }

                wait(NULL);

                if (i < numCommands - 1) {
                    previous_pipe = pipe_fd[0];
                }
            }
        }
    }
}   

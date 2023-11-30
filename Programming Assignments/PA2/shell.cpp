#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {
    for (;;) {
        // need date/time, username, and absolute path to current dir
        // cout << YELLOW << "Shell$" << NC << " ";
        char hostname[1024];
        gethostname(hostname, sizeof(hostname));
        string user = getenv("USER");
        char timestamp[80];
        time_t now = time(nullptr);
        strftime(timestamp, sizeof(timestamp), "%b %d %T", localtime(&now));
        char* cwd = getcwd(nullptr, 0);
        cout << timestamp << " " << user << ":" << hostname << ":" << cwd << "$ " << NC;

        free(cwd);

        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        if (input.find("cd ") == 0) { // cd command handling
            string dir = input.substr(3);
            
            if (dir == "-") {
                char* prevDir = getenv("OLDPWD");
                if (prevDir != nullptr) {
                    chdir(prevDir);
                }
            } else {
                char* prevDir = getcwd(nullptr, 0);
                if (chdir(dir.c_str()) == 0) {
                    setenv("OLDPWD", prevDir, 1);
                } else {
                    cerr << "Error changing directory to " << dir << endl;
                }
                free(prevDir);
            }

            continue;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        vector<Command*> commands = tknr.commands;
        int numCommands = commands.size();

        int pipe_fd[2];
        int previous_pipe = -1;
        int status = 0;

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

                if (command->hasInput()) {
                    int input_fd = open(command->in_file.c_str(), O_RDONLY);
                    if (input_fd == -1) {
                        cerr << "Failed to open input file: " << command->in_file << endl;
                        exit(1);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }

                if (command->hasOutput()) {
                    int output_fd = open(command->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (output_fd == -1) {
                        cerr << "Failed to open output file: " << command->out_file << endl;
                        exit(1);
                    }
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }

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

                waitpid(child_pid, &status, 0);

                if (i < numCommands - 1) {
                    previous_pipe = pipe_fd[0];
                }
            }
        }
        
        if (WIFEXITED(status)) {
            status = WEXITSTATUS(status);
        }
    }
}
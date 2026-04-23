#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

vector<string> path = {"/bin", "/usr/bin"};


void error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}


pid_t executeCommand(vector<string>& args, string outputFile) {
    // find the executable in the search path
    if (args.empty()) {
        error();
        return -1;
    }

    string fullPath = "";
    for (size_t i = 0; i < path.size(); i++) {
        string newPath = path[i] + "/" + args[0];
        if (access(newPath.c_str(), X_OK) == 0) {
            fullPath = newPath;
            break;
        }
    }

    if (fullPath == "") {
        error();
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        error();
        return -1;
    }

    if (pid == 0) {
        // child process 
        // redirection if output file was given
        if (outputFile != "") {
            // open, create or overwrite (and truncate) the file
            int fileDescriptor = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fileDescriptor < 0) {
                error();
                exit(1);
            }
            // must make the STD OUT reroute to the outputFile
            dup2(fileDescriptor, STDOUT_FILENO);
            // must make the STD ERR reroute to the outputFile
            dup2(fileDescriptor, STDERR_FILENO);
            close(fileDescriptor); 
        }

        // ok real redirevtion time, build a char* array for execv -> shud look like ["ls", "-la", "/tmp", NULL]
        vector<char*> cargs;
        for (size_t i = 0; i < args.size(); i++) {
            cargs.push_back(const_cast<char*>(args[i].c_str()));
        }
        cargs.push_back(NULL); //need to have this at the end of the execv array apparently

        execv(fullPath.c_str(), cargs.data());

        //this is only reached here if it failed
        error();
        exit(1);

    } else {
        // Parent process, return the pid
        return pid;
    }
}


void runLine(string line) {

    //split commands by "&"
    vector<string> commands;
    stringstream ss(line);
    string cmdSegment;
    while (getline(ss, cmdSegment, '&')) {
        // get rid of leading/trailing spaces from each cmdSegment
        size_t start = cmdSegment.find_first_not_of(" \t");
        size_t end   = cmdSegment.find_last_not_of(" \t\r");
        if (start == string::npos) continue; // blank cmdSegment, skip
        cmdSegment = cmdSegment.substr(start, end - start + 1); // trim that cmdSegment
        if (!cmdSegment.empty()) commands.push_back(cmdSegment);
    }

    if (commands.empty()) return;

    // for each cmdSegment, parse the args and check if redirection is needed
    vector<pid_t> pids; // collect child pids to wait on later

    for (size_t s = 0; s < commands.size(); s++) {
        string seg = commands[s];

        // check for redirection '>'
        string outputFile = "";
        size_t redirPosition = seg.find('>');
        if (redirPosition != string::npos) {
            string leftPart  = seg.substr(0, redirPosition);
            string rightPart = seg.substr(redirPosition + 1);


            //can't have "ls > " or " > output " 
            if (leftPart.empty() || rightPart.empty()) {
                error();
                continue;
            }
            // error: more than one '>' (e.g. "ls > a > b")
            if (rightPart.find('>') != string::npos) {
                error();
                continue;
            }


            // parse the filename (right of '>')
            istringstream rss(rightPart);
            string filename;
            rss >> filename; // first token
            string extra;
            if (rss >> extra) { //if there's a second file name return error
                error();
                continue;
            }
            if (filename.empty()) { // if file name not parsed correctly
                error();
                continue;
            }

            outputFile = filename;
            seg = leftPart; // the command is just the left part
        }

        // turn the segment that is trimmed by now into args = [cmd, args 1, args 2...]
        istringstream iss(seg);
        vector<string> args;
        string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }

        if (args.empty()) continue;

        // built in moment
        if (args[0] == "exit") {
            if (args.size() != 1) {
                error();
            } else {
                exit(0);
            }
            
        }

        else if (args[0] == "cd") {
            if (args.size() != 2) {
                error();
            } else {
                if (chdir(args[1].c_str()) != 0) {
                    error();
                }
            }
            
        }

        else if (args[0] == "path") {
            path.clear();
            for (size_t i = 1; i < args.size(); i++) {
                path.push_back(args[i]);
            }
            
        }
        else {
            // it's an external commend, so do the child pid thing
            pid_t pid = executeCommand(args, outputFile);
            if (pid > 0) pids.push_back(pid);
        }

    }

    // wait for all child processes to finish and then return to the main process -> this is the parellel part
    for (size_t i = 0; i < pids.size(); i++) {
        waitpid(pids[i], nullptr, 0);
    }
}

int main(int argc, char *argv[]) {

    // more than one argument is an error
    if (argc > 2) {
        error();
        exit(1);
    }

    // figure out where input is coming from
    bool batchMode = false;
    ifstream batchFile;

    if (argc == 2) {
        batchMode = true;
        batchFile.open(argv[1]);
        if (!batchFile.is_open()) {
            error();
            exit(1);
        }

    } 

    string line;
    while (true) {

        // only print the prompt in interactive mode
        if (!batchMode) {
            cout << "wish> " << flush;
        }

        // read a line from stdin (interactive) or the batch file
        if (batchMode) {
            if (!getline(batchFile, line)) exit(0); // EOF
        } else {
            if (!getline(cin, line)) exit(0); // EOF (Ctrl+D)
        }
        runLine(line);
    }

    return 0;
}
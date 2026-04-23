#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// ─── Globals ────────────────────────────────────────────────────────────────

static std::vector<std::string> searchPath = {"/bin"};

static const char ERROR_MSG[] = "An error has occurred\n";
static void printError() {
    write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
}

// ─── Helpers ────────────────────────────────────────────────────────────────

// Split a string by any characters in `delims`, skipping empty tokens.
static std::vector<std::string> split(const std::string& s, const std::string& delims) {
    std::vector<std::string> tokens;
    size_t start = 0;
    while (start < s.size()) {
        size_t pos = s.find_first_of(delims, start);
        if (pos == std::string::npos) pos = s.size();
        if (pos > start)
            tokens.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return tokens;
}

// Trim leading/trailing whitespace (spaces and tabs).
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

// Find the full path of an executable by searching searchPath.
// Returns "" if not found.
static std::string findExecutable(const std::string& cmd) {
    for (const auto& dir : searchPath) {
        std::string fullPath = dir + "/" + cmd;
        if (access(fullPath.c_str(), X_OK) == 0)
            return fullPath;
    }
    return "";
}

// ─── Structs ────────────────────────────────────────────────────────────────

struct Command {
    std::vector<std::string> args;   // args[0] is the program name
    std::string outputFile;          // non-empty => redirect stdout+stderr here
    bool hasRedirect = false;
};

// ─── Parsing ────────────────────────────────────────────────────────────────

// Parse one segment (between '&' separators) into a Command.
// Returns false and prints error if parsing fails.
static bool parseSegment(const std::string& segment, Command& cmd) {
    // Split on '>' to check redirection.
    // We need to be careful: '>' may appear without surrounding spaces.
    std::string seg = trim(segment);
    if (seg.empty()) return false;

    size_t redirPos = seg.find('>');
    if (redirPos != std::string::npos) {
        cmd.hasRedirect = true;
        std::string leftPart  = trim(seg.substr(0, redirPos));
        std::string rightPart = trim(seg.substr(redirPos + 1));

        // Error: another '>' on the right side
        if (rightPart.find('>') != std::string::npos) {
            printError();
            return false;
        }

        // The right side should be exactly one token (the filename).
        std::vector<std::string> rTokens = split(rightPart, " \t");
        if (rTokens.size() != 1) {
            printError();
            return false;
        }
        cmd.outputFile = rTokens[0];

        // The left side is the command + args.
        cmd.args = split(leftPart, " \t");
        if (cmd.args.empty()) {
            printError();
            return false;
        }
    } else {
        // No redirection — entire segment is command + args.
        cmd.args = split(seg, " \t");
        if (cmd.args.empty()) return false;
    }
    return true;
}

// Parse a full input line into one or more Commands (split by '&').
static std::vector<Command> parseLine(const std::string& line) {
    std::vector<Command> commands;

    // Split on '&' to get parallel segments.
    // Note: '&' does NOT require surrounding spaces per the spec.
    std::vector<std::string> segments = split(line, "&");

    for (const auto& seg : segments) {
        std::string trimmed = trim(seg);
        if (trimmed.empty()) continue;
        Command cmd;
        if (!parseSegment(trimmed, cmd)) {
            // parseSegment already printed the error; return empty to skip.
            return {};
        }
        commands.push_back(cmd);
    }
    return commands;
}

// ─── Built-ins ───────────────────────────────────────────────────────────────

// Returns true if the command was a built-in (handled here).
static bool handleBuiltin(const Command& cmd) {
    const std::string& name = cmd.args[0];

    if (name == "exit") {
        if (cmd.args.size() != 1) {
            printError();
            return true;   // still a built-in, just an error
        }
        exit(0);
    }

    if (name == "cd") {
        if (cmd.args.size() != 2) {
            printError();
            return true;
        }
        if (chdir(cmd.args[1].c_str()) != 0)
            printError();
        return true;
    }

    if (name == "path") {
        // Replace the global search path with the provided arguments.
        searchPath.clear();
        for (size_t i = 1; i < cmd.args.size(); i++)
            searchPath.push_back(cmd.args[i]);
        return true;
    }

    return false;
}

// ─── Execution ───────────────────────────────────────────────────────────────

// Fork and exec one command.  Does NOT wait — caller collects children.
static pid_t launchCommand(const Command& cmd) {
    // Find the executable.
    std::string exePath = findExecutable(cmd.args[0]);
    if (exePath.empty()) {
        printError();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printError();
        return -1;
    }

    if (pid == 0) {
        // ── Child process ──────────────────────────────────────────────────

        // Set up redirection if requested.
        if (cmd.hasRedirect) {
            int fd = open(cmd.outputFile.c_str(),
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                printError();
                exit(1);
            }
            // Redirect both stdout and stderr to the file.
            if (dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
                printError();
                exit(1);
            }
            close(fd);
        }

        // Build argv for execv.
        std::vector<char*> argv;
        for (const auto& arg : cmd.args)
            argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);

        execv(exePath.c_str(), argv.data());

        // execv only returns on failure.
        printError();
        exit(1);
    }

    // Parent: return child pid so the caller can wait on it.
    return pid;
}

// Run one parsed line (possibly multiple parallel commands).
static void runLine(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty()) return;

    std::vector<Command> commands = parseLine(trimmed);
    if (commands.empty()) return;

    // ── Built-in shortcut ─────────────────────────────────────────────────
    // If there's exactly one command and it's a built-in, handle inline.
    if (commands.size() == 1 && handleBuiltin(commands[0]))
        return;

    // If ANY command is a built-in in a parallel list, that's still handled
    // inline for this simplified shell (spec says not to test this edge case).

    // ── Launch all commands in parallel ───────────────────────────────────
    std::vector<pid_t> pids;
    for (const auto& cmd : commands) {
        if (handleBuiltin(cmd)) continue;   // built-in: run inline, no fork
        pid_t pid = launchCommand(cmd);
        if (pid > 0) pids.push_back(pid);
    }

    // ── Wait for all children ─────────────────────────────────────────────
    for (pid_t pid : pids)
        waitpid(pid, nullptr, 0);
}

// ─── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc > 2) {
        // More than one argument is an error; exit(1) immediately.
        printError();
        exit(1);
    }

    bool batchMode = (argc == 2);
    std::istream* input = &std::cin;
    std::ifstream batchFile;

    if (batchMode) {
        batchFile.open(argv[1]);
        if (!batchFile.is_open()) {
            printError();
            exit(1);
        }
        input = &batchFile;
    }

    std::string line;
    while (true) {
        // Print prompt only in interactive mode.
        if (!batchMode) {
            std::cout << "wish> " << std::flush;
        }

        if (!std::getline(*input, line)) {
            // EOF — exit gracefully.
            exit(0);
        }

        runLine(line);
    }

    return 0;
}
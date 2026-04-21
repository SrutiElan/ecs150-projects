#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

/*
argv[0] = ./wgrep
argv[1] = searchTerm
argv[2] = file

*/

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        cout << "wgrep: searchterm [file ...]" << endl;
        exit(1);
    }

    string searchTerm = argv[1];

    if (searchTerm == "")
    {             // empty string
        return 0; // match no lines
    }

    // loop over all files passed in

    // open file
    for (int i = 2; i < argc; i++)
    { // for each file
        int fileDescriptor = open(argv[i], O_RDONLY);

        if (fileDescriptor < 0)
        {
            cout << "wgrep: cannot open file" << endl;
            exit(1);
        }

        char buffer;
        int bytesRead;
        string line = "";
        while ((bytesRead = read(fileDescriptor, &buffer, sizeof(buffer))) > 0)
        {
            if (buffer == '\n')
            {
                if (line.find(searchTerm) != string::npos)
                {
                    write(STDOUT_FILENO, line.c_str(), line.length());
                    write(STDOUT_FILENO, "\n", 1);
                }
                line = "";
            }
            else
            {
                line += buffer;
            }
        }
        // file not ending in newline
        if (!line.empty() && line.find(searchTerm) != string::npos)
        {
            write(STDOUT_FILENO, line.c_str(), line.length());
            write(STDOUT_FILENO, "\n", 1);
        }

        close(fileDescriptor);
    }

    // no file provided
    if (argc < 3)
    {
        char buffer;
        int bytesRead;
        string line = "";
        while ((bytesRead = read(STDIN_FILENO, &buffer, sizeof(buffer))) > 0)
        {
            if (buffer == '\n')
            {
                if (line.find(searchTerm) != string::npos)
                {
                    write(STDOUT_FILENO, line.c_str(), line.length());
                    write(STDOUT_FILENO, "\n", 1);
                }
                line = "";
            }
            else
            {
                line += buffer;
            }
        }
        if (!line.empty() && line.find(searchTerm) != string::npos)
        {
            write(STDOUT_FILENO, line.c_str(), line.length());
            write(STDOUT_FILENO, "\n", 1);
        }
    }

    return 0;
}
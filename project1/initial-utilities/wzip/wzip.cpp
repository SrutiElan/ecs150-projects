#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

/*
argv[0] = ./wzip
argv[1] = file
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "wzip: file1 [file2 ...]" << endl;
        exit(1);
    }

    char currChar = '\0';
    int runLen = 0;
    int fileDescriptor;
    for (int i = 1; i < argc; i++)
    { // for each file

        fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor < 0)
        {
            cout << "wzip: cannot open file" << endl;
            exit(1);
        }

        char buffer;
        int bytesRead;
        string line = "";
        while ((bytesRead = read(fileDescriptor, &buffer, sizeof(buffer))) > 0)
        {
            if (buffer == currChar)
            { // same char
                runLen += 1;
            }
            else
            {
                if (runLen > 0)
                {
                    write(STDOUT_FILENO, &runLen, sizeof(int));
                    write(STDOUT_FILENO, &currChar, sizeof(char));
                }

                currChar = buffer;
                runLen = 1;
            }
        }

        close(fileDescriptor);
    }

    // finish off the rest of the doc
    if (runLen > 0)
    {
        write(STDOUT_FILENO, &runLen, sizeof(int));
        write(STDOUT_FILENO, &currChar, sizeof(char));
    }

    return 0;
}

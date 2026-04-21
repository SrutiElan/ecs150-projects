#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

/*
argv[0] = ./wunzip
argv[1] = file
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "wunzip: file1 [file2 ...]" << endl;
        exit(1);
    }

    
    for (int i = 1; i < argc; i++)
    { // for each file

        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor < 0)
        {
            cout << "wunzip: cannot open file" << endl;
            exit(1);
        }

        int bytesRead;
        string line = "";

        int runLen; // 4 bytes
        char currChar;
        // must read 5 bytes: 4 byte is count, last byte is the char
        
        while ((bytesRead = read(fileDescriptor, &runLen, sizeof(int))) > 0)
        {
            read(fileDescriptor, &currChar, sizeof(char));

            for (int j = 0; j < runLen; j++){
                write(STDOUT_FILENO, &currChar, sizeof(char));
            }
        }

        close(fileDescriptor);
    }
    

    return 0;
}

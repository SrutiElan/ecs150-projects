#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;


int main(int argc, char *argv[]) {
    if (argc < 2) { //no files specified (too few arguments)
        return 0;
    }

    int i = 1;
    while (argv[i] != NULL){
        int fileDescriptor = open(argv[i], O_RDONLY);
        if (fileDescriptor < 0) {
            cout << "wcat: cannot open file" << endl;
            exit(1);
        }

        char buffer [100];
        int bytesRead;
        while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0) {
            write(1, buffer, bytesRead);
        }

        close(fileDescriptor);

        i += 1;
    }

    
    return 0;
}
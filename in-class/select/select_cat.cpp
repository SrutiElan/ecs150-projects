#include <iostream>
#include <vector>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[])
{
    vector<int> fdVec; // holds the file descriptors we will be reading from
    fdVec.push_back(STDIN_FILENO);

    for (int idx = 1; idx < argc; idx++)
    {
        // important O_NONBLOCK flag sets up non blocking semantics for the FD
        int fd = open(argv[idx], O_RDONLY | O_NONBLOCK); // O_NONBLOCK tells OS return immediately if there's no data ready - don't freeze.
        fdVec.push_back(fd);
    }

    while (fdVec.size() > 0)
    {                   // keep going until all files are done
        fd_set readSet; // fd_set is a bitmap, row of bits where each bit position corresponds to a file descriptor number. Telling OS which fds we care about
        int maxFd = -1;
        FD_ZERO(&readSet); // clearing the bits from bit map

        // set up the data strucutres needed for select
        for (int idx = 0; idx < fdVec.size(); idx++)
        {
            int fd = fdVec[idx];
            FD_SET(fd, &readSet); // flip on the bit for that fd, meaning watch this one
            if (fd > maxFd)
            {
                maxFd = fd; // select needs to know the highest fd number to know how many bits to scan
            }
        }
        // blocking call (program pauses here) until >=1 fd in readSet has data ready to read.
        // Once it does, select() returnsa nd modifies readSet in place -> clearing the bits for fds that are not ready and leaving the bits set for fds that ARE ready
        select(maxFd + 1, &readSet, NULL, NULL, NULL);

        // find the FD that has data and processs it
        //  imprtant: we can set polciies on priorities
        //  Question: caould you implmemetn a policy that reads files completely before moving on?

        for (int idx = 0; idx < fdVec.size(); idx++)
        {
            int fd = fdVec[idx];
            if (FD_ISSET(fd, &readSet))
            { // check if the bit for this fd is still set, meaning it has data ready to read
                char buffer[4096];
                int ret = read(fd, buffer, sizeof(buffer));
                if (ret == 0)
                {
                    // EOF - we're done with this file
                    close(fd);
                    fdVec.erase(fdVec.begin() + idx); // remove it from our list of fds to watch
                    break;                            // break out of the for loop and go back to the top of the while loop to call select again on the remaining fds
                }
                else
                {
                    write(STDOUT_FILENO, buffer, ret); // print the data to stdout
                }
            }
        }

        cout << "all done!" << endl;

        return 0;
    }
}

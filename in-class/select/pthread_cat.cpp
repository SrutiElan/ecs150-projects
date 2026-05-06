#include <iostream>
#include <vector>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

// we're going to ignore error checking to keep this
// example simple, but make sure that you do full
// error checking in any code that you write for
// projects in this class and in general if you want
// to actually use the software.
void write_file_to_stdout(int fd)
{
    char buffer[4096];
    int ret;

    while ((ret = read(fd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, ret); // Print fd contents to stdout
    }
    close(fd);
}
struct ThreadArgs {
    int fd;
};
void *threadHandler(void *arg){
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)arg;
    int fd = threadArgs -> fd;
    write_file_to_stdout(fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    vector<int> fdVec;
    fdVec.push_back(STDIN_FILENO);

    for (int idx = 1; idx < argc; idx++)
    {
        int fd = open(argv[idx], O_RDONLY);
        fdVec.push_back(fd);
    }

    vector<pthread_t> threads; // holds the thread ids for the threads we will create (this is needed to ensure the main thread waits for all child threads to finish before exiting the program)
    // loop through each fd one-by-one and print out the contents
    for (int idx = 0; idx < fdVec.size(); idx++)
    {
        int fd = fdVec[idx];
        pthread_t threadId;
        /*
        struct ThreadArgs threadArgs; 
        threadArgs.fd = fd;
        
        ^ CRITICAL ERROR. each time loop iterates, variables are established on a stack that gets reset. 
        ThreadArgs will get reset and overwritten before the thread can read the fd value. 
        To fix this, we need to malloc space for each thread's arguments so that they persist until the thread is done with them.
        */
       struct ThreadArgs *threadArgs = new struct ThreadArgs;
       threadArgs -> fd = fd;
        
        pthread_create(&threadId, NULL, threadHandler, (void *)threadArgs);
        threadArgs = NULL; // set pointer to null after passing it to thread to avoid accidentally using it in the main thread after it's been passed to the child thread.
        threads.push_back(threadId);
    }
    for (int idx = 0; idx < threads.size(); idx++)
    {
        pthread_t threadId = threads[idx];
        pthread_join(threadId, NULL); // wait for each thread to finish before exiting the program. 
    }

    cout << "\nall done!" << endl;

    return 0;
}
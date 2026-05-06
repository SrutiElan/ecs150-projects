#include <iostream>
#include <vector>

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

int main(int argc, char *argv[]) {
  vector<int> fdVec;
  fdVec.push_back(STDIN_FILENO);

  for(int idx = 1; idx < argc; idx++) {
    int fd = open(argv[idx], O_RDONLY);
    fdVec.push_back(fd);
  }

  // loop through each fd one-by-one and print out the contents
  for (int idx = 0; idx < fdVec.size(); idx++) {
    if (fork() == 0) { // Fork a child; child sees 0, parent sees child PID
      // child
      int fd = fdVec[idx];
      char buffer[4096];
      int ret;

      while ((ret = read(fd, buffer, sizeof(buffer))) > 0) {
	      write(STDOUT_FILENO, buffer, ret); // Print fd contents to stdout
      }

      close(fd);
      return 0; // Child exits — critical! Without this, child re-enters the loop, then forks itself 
    } else {
      // parent
      wait(NULL); // waits until a child process is done and then continues to execute. 
    }
  }

  for (int idx = 0; idx < fdVec.size(); idx++) { //fdVec.size() is the number of children
    wait(NULL); //basically calls wait() x number of children times to wait for all children to finish before printing "all done!" and exiting the program.
  }
  
  cout << "\ncrazy all done!" << endl;
  
  return 0;
}
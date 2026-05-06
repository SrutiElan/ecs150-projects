#include <iostream>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
  sleep(5); // sleep for 5 seconds to give us time to run "strace -p <pid>" in another terminal
  cout << "Hello ECS 150 world!" << endl;
  return 0;
}
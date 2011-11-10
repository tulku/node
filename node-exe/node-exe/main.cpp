#include <node.h>
#include <pthread.h>

void *run(void *arg);

int main(int argc, char **argv) {
  return node::Start(argc, argv);
}

#include <node.h>

void *run(void *arg);

int main(int argc, char **argv) {
  return node::Start(argc, argv);
}

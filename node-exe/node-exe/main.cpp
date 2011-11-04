#include <node.h>
#include <pthread.h>

void *run(void *arg);

int main(int argc, char **argv) {
  pthread_t t1, t2;
  int result = node::Initialize(argc, argv);
  if(result == 0) {
    pthread_create(&t1, NULL, &run, (void *)"1337");
    pthread_create(&t2, NULL, &run, (void *)"1338");
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    node::Dispose();
  }
  return result;
}

void *run(void *arg) {
  char *arg0 = (char *)"node";
  char *arg1 = (char *)"serv-hello.js";
  char *arg2 = (char *)arg;
  char *arg3 = 0;
  char *argv[] = {arg0, arg1, arg2, arg3};
  node::Isolate *isolate = node::Isolate::New();
  isolate->Start(3, argv);
  isolate->Dispose();
  return NULL;
}

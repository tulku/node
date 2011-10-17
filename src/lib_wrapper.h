#ifndef SRC_LIB_WRAPPER_H_
#define SRC_LIB_WRAPPER_H_

void __exit(int exitval);

#ifdef __cplusplus
extern "C" {
#endif

int libnode_Start(int argc, char *argv[]);
int libnode_Stop(int signum);

#ifdef __cplusplus
}
#endif

#define exit(X) { __exit((X)); }

#endif //SRC_LIB_WRAPPER_H_
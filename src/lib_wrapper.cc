#include "lib_wrapper.h"
#include "node.h"

#include <setjmp.h>

/* buffer to store entry state */
sigjmp_buf __exit_buf;

/* called when exit() is called to abruptly kill the node instance */
void __exit(int exitval) {
	siglongjmp(__exit_buf, exitval<<1 + 1);
}

/* tidy resources in the event of an abrupt exit */
void __tidy() {
	node::AtExit();
}

/* Start the node instance */
int libnode_Start(int argc, char *argv[]) {
	int exitval = sigsetjmp(__exit_buf, 1);
	if(!exitval) {
		/* first entry, so launch node */
		node::Start(argc, argv);
		/* normal return, equivalent to exit(0) */
	}
	__tidy();
	return exitval>>1;
}

/* Stop the node instance */
int libnode_Stop(int signum) {
	node::Stop(signum);
	return 0;
}

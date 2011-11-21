/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h> /* O_CLOEXEC, O_NONBLOCK */
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#else
extern char **environ;
#endif


static void uv__chld(EV_P_ ev_child* watcher, int revents) {
  int status = watcher->rstatus;
  int exit_status = 0;
  int term_signal = 0;
  uv_process_t *process = watcher->data;

  assert(&process->child_watcher == watcher);
  assert(revents & EV_CHILD);

  ev_child_stop(EV_A_ &process->child_watcher);

  if (WIFEXITED(status)) {
    exit_status = WEXITSTATUS(status);
  }

  if (WIFSIGNALED(status)) {
    term_signal = WTERMSIG(status);
  }

  if (process->exit_cb) {
    process->exit_cb((uv_handle_t *)process, exit_status, term_signal);
  }
}


#define UV__F_IPC        (1 << 0)
#define UV__F_NONBLOCK   (1 << 1)

static int uv__make_socketpair(int fds[2], int flags) {
#ifdef SOCK_NONBLOCK
  int fl;

  fl = SOCK_CLOEXEC;

  if (flags & UV__F_NONBLOCK)
    fl |= SOCK_NONBLOCK;

  if (socketpair(AF_UNIX, SOCK_STREAM|fl, 0, fds) == 0)
    return 0;

  if (errno != EINVAL)
    return -1;

  /* errno == EINVAL so maybe the kernel headers lied about
   * the availability of SOCK_NONBLOCK. This can happen if people
   * build libuv against newer kernel headers than the kernel
   * they actually run the software on.
   */
#endif

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds))
    return -1;

  uv__cloexec(fds[0], 1);
  uv__cloexec(fds[1], 1);

  if (flags & UV__F_NONBLOCK) {
    uv__nonblock(fds[0], 1);
    uv__nonblock(fds[1], 1);
  }

  return 0;
}


static int uv__make_pipe(int fds[2], int flags) {
#if HAVE_SYS_PIPE2
  int fl;

  fl = O_CLOEXEC;

  if (flags & UV__F_NONBLOCK)
    fl |= O_NONBLOCK;

  if (sys_pipe2(fds, fl) == 0)
    return 0;

  if (errno != ENOSYS)
    return -1;

  /* errno == ENOSYS so maybe the kernel headers lied about
   * the availability of pipe2(). This can happen if people
   * build libuv against newer kernel headers than the kernel
   * they actually run the software on.
   */
#endif

  if (pipe(fds))
    return -1;

  uv__cloexec(fds[0], 1);
  uv__cloexec(fds[1], 1);

  if (flags & UV__F_NONBLOCK) {
    uv__nonblock(fds[0], 1);
    uv__nonblock(fds[1], 1);
  }

  return 0;
}


/*
 * Used for initializing stdio streams like options.stdin_stream. Returns
 * zero on success.
 */
static int uv__process_init_pipe(uv_pipe_t* handle, int fds[2], int flags) {
  if (handle->type != UV_NAMED_PIPE) {
    errno = EINVAL;
    return -1;
  }

  if (handle->ipc)
    return uv__make_socketpair(fds, flags);
  else
    return uv__make_pipe(fds, flags);
}


#ifndef SPAWN_WAIT_EXEC
# define SPAWN_WAIT_EXEC 1
#endif

int uv_spawn(uv_loop_t* loop, uv_process_t* process,
    uv_process_options_t options) {
  /*
   * Save environ in the case that we get it clobbered
   * by the child process.
   */
  char** save_our_env = environ;
  int stdin_pipe[2] = { -1, -1 };
  int stdout_pipe[2] = { -1, -1 };
  int stderr_pipe[2] = { -1, -1 };
#if SPAWN_WAIT_EXEC
  int signal_pipe[2] = { -1, -1 };
  struct pollfd pfd;
#endif
  int status;
  pid_t pid;
  int flags;

  uv__handle_init(loop, (uv_handle_t*)process, UV_PROCESS);
  loop->counters.process_init++;

  process->exit_cb = options.exit_cb;

  if (options.stdin_stream &&
      uv__process_init_pipe(options.stdin_stream, stdin_pipe, 0)) {
    goto error;
  }

  if (options.stdout_stream &&
      uv__process_init_pipe(options.stdout_stream, stdout_pipe, 0)) {
    goto error;
  }

  if (options.stderr_stream &&
      uv__process_init_pipe(options.stderr_stream, stderr_pipe, 0)) {
    goto error;
  }

  /* This pipe is used by the parent to wait until
   * the child has called `execve()`. We need this
   * to avoid the following race condition:
   *
   *    if ((pid = fork()) > 0) {
   *      kill(pid, SIGTERM);
   *    }
   *    else if (pid == 0) {
   *      execve("/bin/cat", argp, envp);
   *    }
   *
   * The parent sends a signal immediately after forking.
   * Since the child may not have called `execve()` yet,
   * there is no telling what process receives the signal,
   * our fork or /bin/cat.
   *
   * To avoid ambiguity, we create a pipe with both ends
   * marked close-on-exec. Then, after the call to `fork()`,
   * the parent polls the read end until it sees POLLHUP.
   */
#if SPAWN_WAIT_EXEC
  if (uv__make_pipe(signal_pipe, UV__F_NONBLOCK))
    goto error;
#endif

  pid = fork();

  if (pid == -1) {
#if SPAWN_WAIT_EXEC
    uv__close(signal_pipe[0]);
    uv__close(signal_pipe[1]);
#endif
    environ = save_our_env;
    goto error;
  }

  if (pid == 0) {
    if (stdin_pipe[0] >= 0) {
      uv__close(stdin_pipe[1]);
      dup2(stdin_pipe[0],  STDIN_FILENO);
    } else {
      /* Reset flags that might be set by Node */
      uv__cloexec(STDIN_FILENO, 0);
      uv__nonblock(STDIN_FILENO, 0);
    }

    if (stdout_pipe[1] >= 0) {
      uv__close(stdout_pipe[0]);
      dup2(stdout_pipe[1], STDOUT_FILENO);
    } else {
      /* Reset flags that might be set by Node */
      uv__cloexec(STDOUT_FILENO, 0);
      uv__nonblock(STDOUT_FILENO, 0);
    }

    if (stderr_pipe[1] >= 0) {
      uv__close(stderr_pipe[0]);
      dup2(stderr_pipe[1], STDERR_FILENO);
    } else {
      /* Reset flags that might be set by Node */
      uv__cloexec(STDERR_FILENO, 0);
      uv__nonblock(STDERR_FILENO, 0);
    }

    if (options.cwd && chdir(options.cwd)) {
      perror("chdir()");
      _exit(127);
    }

    environ = options.env;

    execvp(options.file, options.args);
    perror("execvp()");
    _exit(127);
    /* Execution never reaches here. */
  }

  /* Parent. */

  /* Restore environment. */
  environ = save_our_env;

#if SPAWN_WAIT_EXEC
  /* POLLHUP signals child has exited or execve()'d. */
  uv__close(signal_pipe[1]);
  do {
    pfd.fd = signal_pipe[0];
    pfd.events = POLLIN|POLLHUP;
    pfd.revents = 0;
    errno = 0, status = poll(&pfd, 1, -1);
  }
  while (status == -1 && (errno == EINTR || errno == ENOMEM));

  assert((status == 1) && "poll() on pipe read end failed");
  uv__close(signal_pipe[0]);
#endif

  process->pid = pid;

  ev_child_init(&process->child_watcher, uv__chld, pid, 0);
  ev_child_start(process->loop->ev, &process->child_watcher);
  process->child_watcher.data = process;

  if (stdin_pipe[1] >= 0) {
    assert(options.stdin_stream);
    assert(stdin_pipe[0] >= 0);
    uv__close(stdin_pipe[0]);
    uv__nonblock(stdin_pipe[1], 1);
    flags = UV_WRITABLE | (options.stdin_stream->ipc ? UV_READABLE : 0);
    uv__stream_open((uv_stream_t*)options.stdin_stream, stdin_pipe[1],
        flags);
  }

  if (stdout_pipe[0] >= 0) {
    assert(options.stdout_stream);
    assert(stdout_pipe[1] >= 0);
    uv__close(stdout_pipe[1]);
    uv__nonblock(stdout_pipe[0], 1);
    flags = UV_READABLE | (options.stdout_stream->ipc ? UV_WRITABLE : 0);
    uv__stream_open((uv_stream_t*)options.stdout_stream, stdout_pipe[0],
        flags);
  }

  if (stderr_pipe[0] >= 0) {
    assert(options.stderr_stream);
    assert(stderr_pipe[1] >= 0);
    uv__close(stderr_pipe[1]);
    uv__nonblock(stderr_pipe[0], 1);
    flags = UV_READABLE | (options.stderr_stream->ipc ? UV_WRITABLE : 0);
    uv__stream_open((uv_stream_t*)options.stderr_stream, stderr_pipe[0],
        flags);
  }

  return 0;

error:
  uv__set_sys_error(process->loop, errno);
  uv__close(stdin_pipe[0]);
  uv__close(stdin_pipe[1]);
  uv__close(stdout_pipe[0]);
  uv__close(stdout_pipe[1]);
  uv__close(stderr_pipe[0]);
  uv__close(stderr_pipe[1]);
  return -1;
}

/* retrieves the shared handle, synchronously calling a user callback 
 * callback contains shared handle pointer, or NULL if thread has exited */
int uv_thread_get_shared(uv_thread_t *thread, int (*cb)(uv_thread_shared_t *, void *), void *data) {
  uv_thread_shared_t *hnd = thread->thread_shared;
  pthread_mutex_lock(&hnd->mtx);
  int r = cb(hnd->thread_id ? hnd : 0, data); 
  pthread_mutex_unlock(&hnd->mtx);
  return r;
}

/* deletes the shared handle */
void uv__thread_shared_delete(uv_thread_shared_t *hnd) {
  int i;
  if (hnd->args) {
    for (i = 0; hnd->args[i]; i++) free(hnd->args[i]);
    free(hnd->args);
  }
  if (hnd->env) {
    for (i = 0; hnd->env[i]; i++) free(hnd->env[i]);
    free(hnd->env);
  }
  free(hnd);
}

/* called by the client when the corresponding uv_thread_t handle
 * is closed */
void uv_thread_close(uv_thread_t *thread) {
  /* close the watcher */
  ev_async_stop(thread->loop->ev, (ev_async *)&thread->thread_watcher);

  /* synchronously clear the reference to this from the shared handle */
  uv_thread_shared_t *hnd = thread->thread_shared;
  pthread_mutex_lock(&hnd->mtx);
  hnd->thread_handle = 0;
  pthread_t thread_still_exists = hnd->thread_id;
  pthread_mutex_unlock(&hnd->mtx);

  /* if the thread has exited already, delete shared handle */
 if(!thread_still_exists)
   uv__thread_shared_delete(hnd);
}

/* the actual thread entrypoint passed to pthread_create() */
static void *uv__thread_run(void *arg) {
  /* synchronously get the entrypoint and call it */
  uv_thread_shared_t *hnd = (uv_thread_shared_t *)arg;
  pthread_mutex_lock(&hnd->mtx);
  uv_thread_run thread_run = hnd->options->thread_run;
  void *thread_arg = hnd->options->thread_arg;
  /* ... any other processing depending on thread or options here ... */
  pthread_cond_signal(&hnd->cond);
  pthread_mutex_unlock(&hnd->mtx);  
  void *result = (*thread_run)(hnd, thread_arg);
  
  /* close any pipes created */
  if(hnd->stdin_fd != -1) uv__close(hnd->stdin_fd);
  if(hnd->stdout_fd != -1) uv__close(hnd->stdout_fd);
  if(hnd->stderr_fd != -1) uv__close(hnd->stderr_fd);

  /* synchronously notify exit to the client event loop */
  pthread_mutex_lock(&hnd->mtx);
  uv_thread_t *thread = hnd->thread_handle;
  if(thread) {
    /* copy these before they disappear ... */
    thread->exit_status = hnd->exit_status;
    thread->term_signal = hnd->term_signal;
    ev_async_send(thread->loop->ev, &thread->thread_watcher);
  }
  hnd->thread_id = 0;
  pthread_mutex_unlock(&hnd->mtx);

  /* if the handle had gone already, delete shared handle */
  if(!thread)
    uv__thread_shared_delete(hnd);
  
  return result;
}

/* the watcher callback signifying thread exit. Called in the context
 * of the event loop that created the thread */
static void uv__thread_exit(EV_P_ ev_async* watcher, int revents) {  
  uv_thread_t *thread = watcher->data;
  
  assert(&thread->thread_watcher == watcher);
  assert(revents & EV_ASYNC);
  
  ev_async_stop(EV_A_ &thread->thread_watcher);
  
  if (thread->exit_cb) {
    thread->exit_cb((uv_handle_t *)thread, thread->exit_status, thread->term_signal);
  }
}

/* creates and starts a thread and associated watcher */
int uv_thread_create(uv_loop_t *loop, uv_thread_t *thread, uv_process_options_t options) {

  /* initialise the thread handle */
  uv__handle_init(loop, (uv_handle_t*)thread, UV_THREAD);
  loop->counters.thread_init++;
  thread->exit_cb = options.exit_cb;
  
  /* initialise a watcher for this thread */
  ev_async_init((ev_async *)&thread->thread_watcher, uv__thread_exit);
  ev_async_start(loop->ev, (ev_async *)&thread->thread_watcher);
  thread->thread_watcher.data = thread;
  
  /* initialise the shared handle */
  uv_thread_shared_t *hnd = (uv_thread_shared_t *)calloc(sizeof(uv_thread_shared_t), 1);
  if(!hnd)
    goto error;
  
  thread->thread_shared = hnd;
  pthread_mutex_init(&hnd->mtx, NULL);
  pthread_cond_init(&hnd->cond, NULL);
  hnd->thread_handle = thread;
  hnd->options = &options;
  hnd->thread_arg = options.thread_arg;
  hnd->stdin_fd = hnd->stdout_fd = hnd->stderr_fd = -1;
  /* FIXME: take ownership of args, but better way sought */
  hnd->args = options.args;
  hnd->env = options.env;
  options.args = 0;
  options.env = 0;

  /* set up pipes to caller where requested */
  int r = 0, flags;
  int stdin_pipe[2] = { -1, -1 };
  int stdout_pipe[2] = { -1, -1 };
  int stderr_pipe[2] = { -1, -1 };
  if(options.stdin_stream) {
    r = uv__process_init_pipe(options.stdin_stream, stdin_pipe, 0);
    if(r)
      goto error;
    if(stdin_pipe[0] >= 0) {
      hnd->stdin_fd = stdin_pipe[0];
      uv__nonblock(stdin_pipe[1], 1);
      flags = UV_WRITABLE | (options.stdin_stream->ipc ? UV_READABLE : 0);
      uv__stream_open((uv_stream_t*)options.stdin_stream, stdin_pipe[1],
                      flags);
    }
  }
  if(options.stdout_stream) {
    r = uv__process_init_pipe(options.stdout_stream, stdout_pipe, 0);
    if(r)
      goto error;
    if(stdout_pipe[1] >= 0) {
      hnd->stdout_fd = stdout_pipe[1];
      uv__nonblock(stdout_pipe[0], 1);
      flags = UV_READABLE | (options.stdout_stream->ipc ? UV_WRITABLE : 0);
      uv__stream_open((uv_stream_t*)options.stdout_stream, stdout_pipe[0],
                      flags);
    }
  }
  if(options.stderr_stream) {
    r = uv__process_init_pipe(options.stderr_stream, stderr_pipe, 0);
    if(r)
      goto error;
    if(stderr_pipe[1] >= 0) {
      hnd->stderr_fd = stderr_pipe[1];
      uv__nonblock(stderr_pipe[0], 1);
      flags = UV_READABLE | (options.stderr_stream->ipc ? UV_WRITABLE : 0);
      uv__stream_open((uv_stream_t*)options.stderr_stream, stderr_pipe[0],
                      flags);
    }
  }
  
  /* synchronously create the thread */
  pthread_mutex_lock(&hnd->mtx);
  pthread_create(&hnd->thread_id, 0, uv__thread_run, hnd);
  pthread_cond_wait(&hnd->cond, &hnd->mtx);
  pthread_mutex_unlock(&hnd->mtx);

  return 0;

error:
  uv__set_sys_error(loop, errno);
  uv__close(stdin_pipe[0]);
  uv__close(stdin_pipe[1]);
  uv__close(stdout_pipe[0]);
  uv__close(stdout_pipe[1]);
  uv__close(stderr_pipe[0]);
  uv__close(stderr_pipe[1]);
  return -1;
}

void uv_thread_exit(uv_loop_t *loop, uv_thread_t *thread, int exit_status, int term_signal) {
  thread->exit_status = exit_status;
  thread->term_signal = term_signal;
  ev_async_send(loop->ev, &thread->thread_watcher);
}

int uv_process_kill(uv_process_t* process, int signum) {
  int r = kill(process->pid, signum);

  if (r) {
    uv__set_sys_error(process->loop, errno);
    return -1;
  } else {
    return 0;
  }
}


uv_err_t uv_kill(int pid, int signum) {
  int r = kill(pid, signum);

  if (r) {
    return uv__new_sys_error(errno);
  } else {
    return uv_ok_;
  }
}

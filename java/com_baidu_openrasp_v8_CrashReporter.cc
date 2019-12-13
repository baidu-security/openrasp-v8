/*
 * Copyright 2017-2019 Baidu Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "com_baidu_openrasp_v8_CrashReporter.h"
#include "header.h"

#if defined(__APPLE__) || defined(__linux__)

#include <execinfo.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

static char url[PATH_MAX] = {0};
static char appid_header[PATH_MAX + 32] = "X-OpenRASP-AppID: ";
static char appsecret_header[PATH_MAX + 32] = "X-OpenRASP-AppSecret: ";
static char raspid_form[PATH_MAX + 32] = "rasp_id=";
static char filename[PATH_MAX] = {0};
static char hostname_form[PATH_MAX + 32] = "hostname=";
static char crash_log_form[PATH_MAX + 32] = "crash_log=@";
static bool raised = false;

int fork_and_exec(const char* path, char* const* argv) {
  pid_t pid = fork();

  if (pid < 0) {
    // fork failed
    return -1;
  } else if (pid == 0) {
    // child process

    execvp(path, argv);

    // execve failed
    _exit(-1);
  } else {
    // copied from J2SE ..._waitForProcessExit() in UNIXProcess_md.c; we don't
    // care about the actual exit code, for now.

    int status;

    // Wait for the child process to exit.  This returns immediately if
    // the child has already exited. */
    while (waitpid(pid, &status, 0) < 0) {
      switch (errno) {
        case ECHILD:
          return 0;
        case EINTR:
          break;
        default:
          return -1;
      }
    }

    if (WIFEXITED(status)) {
      // The child exited normally; get its exit code.
      return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      // The child exited because of a signal
      // The best value to return is 0x80 + signal number,
      // because that is what all Unix shells do, and because
      // it allows callers to distinguish between process exit and
      // process death by signal.
      return 0x80 + WTERMSIG(status);
    } else {
      // Unknown exit code; pass it through
      return status;
    }
  }
}

void abort_handler(int sig) {
  if (access(filename, F_OK) == -1) {
    if (raised == false) {
      raised = true;
      raise(SIGSEGV);
      return;
    } else {
      strcpy(crash_log_form, "crash_log=not_found");
    }
  }
  signal(sig, SIG_DFL);
  // clang-format off
  const char* const argv[] = {"curl",
                              url,
                              "--connect-timeout",
                              "5",
                              "-H",
                              appid_header,
                              "-H",
                              appsecret_header,
                              "-F",
                              "job=crash",
                              "-F",
                              "language=java",
                              "-F",
                              raspid_form,
                              "-F",
                              hostname_form,
                              "-F",
                              crash_log_form,
                              nullptr};
  // clang-format on
  if (fork_and_exec("curl", (char* const*)argv) != 0) {
    perror("");
    std::cout << "\n#\n# Failed to report crash log\n#\n" << std::endl;
    // std::cout << url << std::endl;
    // std::cout << hostname_form << std::endl;
    // std::cout << crash_log_form << std::endl;
  }
}

/*
 * Class:     com_baidu_openrasp_v8_CrashReporter
 * Method:    install
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
ALIGN_FUNCTION JNIEXPORT void JNICALL Java_com_baidu_openrasp_v8_CrashReporter_install(JNIEnv* env,
                                                                                       jclass cls,
                                                                                       jstring jurl,
                                                                                       jstring jappid,
                                                                                       jstring jappsecret,
                                                                                       jstring jraspid) {
  strncpy(url, Jstring2String(env, jurl).c_str(), PATH_MAX);
  strncpy(appid_header + strlen(appid_header), Jstring2String(env, jappid).c_str(), PATH_MAX);
  strncpy(appsecret_header + strlen(appsecret_header), Jstring2String(env, jappsecret).c_str(), PATH_MAX);
  strncpy(raspid_form + strlen(raspid_form), Jstring2String(env, jraspid).c_str(), PATH_MAX);
  if (getcwd(filename, PATH_MAX) == nullptr) {
    return;
  }
  snprintf(filename + strlen(filename), PATH_MAX, "/hs_err_pid%d.log", getpid());
  if (gethostname(hostname_form + strlen(hostname_form), PATH_MAX) == -1) {
    return;
  }
  strncpy(crash_log_form + strlen(crash_log_form), filename, PATH_MAX);
  signal(SIGABRT, abort_handler);
}

#else

/*
 * Class:     com_baidu_openrasp_v8_CrashReporter
 * Method:    install
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
ALIGN_FUNCTION JNIEXPORT void JNICALL Java_com_baidu_openrasp_v8_CrashReporter_install(JNIEnv* env,
                                                                                       jclass cls,
                                                                                       jstring u,
                                                                                       jstring appid,
                                                                                       jstring appSecret,
                                                                                       jstring raspid) {
  return;
}

#endif
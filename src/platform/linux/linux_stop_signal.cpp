#include <pthread.h>
#include <signal.h>

namespace {

sigset_t makeStopSignalSet() {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGHUP);
  return set;
}

}  // namespace

void installStopHandler() {
  const sigset_t set{makeStopSignalSet()};
  pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

void waitForStopSignal() {
  const sigset_t set{makeStopSignalSet()};

  int signalNumber{0};
  sigwait(&set, &signalNumber);
}

void signalShutdownComplete() {};
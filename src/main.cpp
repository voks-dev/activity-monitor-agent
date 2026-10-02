#include "core/application.h"
#include "core/interface/stop_signal.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
  installStopHandler();

  int exitCode{EXIT_SUCCESS};

  try {

    Application app;
    app.run();

  } catch (const std::exception& ex) {
    std::cerr << "[main] exception: " << ex.what() << " - exiting\n";
    exitCode = EXIT_FAILURE;

  } catch (...) {
    std::cerr << "[main] exception: unknown exception - exiting\n";
    exitCode = EXIT_FAILURE;
  }

  signalShutdownComplete();

  return exitCode;
}
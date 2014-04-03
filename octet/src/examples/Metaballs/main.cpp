////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Text overlay
//

#include <time.h>
#include <math.h>
#include "../../octet.h"
#include "Metaballs.h"

/// Create a box with octet
int main(int argc, char **argv) {
  // path from bin\Debug to octet directory
  octet::app_utils::prefix("../../../");

  // set up the platform.
  octet::app::init_all(argc, argv);

  // our application.
  octet::Metaballs app(argc, argv);
  app.init();

  // open windows
  octet::app::run_all_apps();
}
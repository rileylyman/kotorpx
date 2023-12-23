#include "raylib.h"
#include "scene.h"
#include <stdio.h>

int main() {
  struct scene_Scene scene = scene_new("scenes/test.scn");
  I("main exiting\n");
  return 0;
}
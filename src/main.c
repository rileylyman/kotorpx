#include <stdio.h>

#include "raylib.h"
#include "scene.h"

int main() {
    struct scene_Scene scene = scene_new("scenes/test.scn");
    I("main exiting\n");
    return 0;
}
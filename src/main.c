#include "log.h"
#include "raylib.h"
#include "scene.h"

int main() {
    struct scene_Scene scene;
    if (!scene_load(&scene, "scenes/test.scn")) {
        return 1;
    }
    I("main exiting\n");
    return 0;
}
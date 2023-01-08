#ifndef CAMERA_H_
#define CAMERA_H_

#include "esp_camera.h"
#include <Arduino.h>
#include <PinDefines.h>

class Camera {
public:
    Camera();
    ~Camera();

    bool initialize();
    void camera_fb_return(camera_fb_t *fb);
    camera_fb_t* camera_fb_get();

};

#endif
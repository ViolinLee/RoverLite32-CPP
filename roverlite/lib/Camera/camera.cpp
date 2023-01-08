#include "camera.h"

bool Camera::initialize()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    // config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    // config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    // config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 1;

	if (psramFound())
	{
		config.jpeg_quality = 10;
        config.frame_size = FRAMESIZE_VGA;
		config.fb_count = 1;
        // config.grab_mode = CAMERA_GRAB_LATEST;
	}
	else
	{
		config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
	}

    if (psramFound())
    {
        heap_caps_malloc_extmem_enable(20000);  
        Serial.printf("PSRAM initialized. malloc to take memory from psram above this size");    
    } 

	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK)
	{
		Serial.printf("Camera init failed with error 0x%x", err);
		return false;
	} else {
        Serial.printf("Camera init succeeded!");
        return true;
    }

}

void Camera::camera_fb_return(camera_fb_t *fb)
{
    esp_camera_fb_return(fb); 
}

camera_fb_t* Camera::camera_fb_get()
{
    return esp_camera_fb_get();
}

Camera::Camera()
{
}

Camera::~Camera()
{
}
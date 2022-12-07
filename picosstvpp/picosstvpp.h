#ifndef PICO_SSTV_PP_H

#define PICO_SSTV_PP_H

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
//#include <gd.h>
#include <time.h>
#include <math.h>
//#include <tgmath.h>
//#include <magic.h>
//#include <unistd.h>

#ifdef ESP32
#else
#include "LittleFS.h"
#endif

#include "hardware/pwm.h" 
#include <TJpg_Decoder.h>
#include "pico/stdlib.h"   // stdlib 

//uint8_t  filetype       (char *filename) ;
void     playtone       (uint16_t tonefreq , double tonedur) ;
void     addvisheader   (void) ;
void     addvistrailer  (void) ;

//uint16_t toneval_rgb        (uint8_t colorval) ;
uint16_t toneval_yuv        (uint8_t colorval) ;

void     buildaudio_m     (double pixeltime) ;
void     buildaudio_s     (double pixeltime) ;
void     buildaudio_r36     (void) ;

#ifdef AUDIO_AIFF
void     writefile_aiff (void) ;
#endif
#ifdef AUDIO_WAV
void     writefile_wav  (void) ;
#endif

void show_dir4();
//void load_files();
void play_pwm_file(int dds_pwm_pin) ;

void picosstvpp_begin(int pin);
void picosstvpp();
void jpeg_decode(char* filename, char* fileout, bool debug);
void sstv_end();

// ================
// macros/defines
// ================

#define RATE   22000 // 11025 
#define WRAP 5
#define MAXRATE   22050
#define BITS   16
#define CHANS  1 
#define VOLPCT 20 
// ^-- 90% max
#define MAXSAMPLES 61000  // 10000  // (300 * MAXRATE)

// uncomment only one of these
//#define AUDIO_WAV
//#define AUDIO_AIFF
#define SSTV_PWM  // 8 level PWM to cam.pwm file

#define MAGIC_PNG ("PNG image data,")
#define MAGIC_JPG ("JPEG image data")
#define MAGIC_CNT 15

#define FILETYPE_ERR 0
#define FILETYPE_PNG 1
#define FILETYPE_JPG 2


#endif

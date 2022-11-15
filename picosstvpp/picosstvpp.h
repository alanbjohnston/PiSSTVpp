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
#include "LittleFS.h"
#include "hardware/pwm.h" 

//uint8_t  filetype       (char *filename) ;
void     playtone       (uint16_t tonefreq , double tonedur) ;
void     addvisheader   (void) ;
void     addvistrailer  (void) ;

uint16_t toneval_rgb        (uint8_t colorval) ;
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

void show_dir();
void load_files();
void play_pwm_file();

void picosstvpp_begin(int pin);

#endif

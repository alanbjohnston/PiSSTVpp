// PiSSTVpp

// 2013 Robert Marshall KI4MCW
// 2014 Gerrit Polder, PA3BYA fixed header. 
// 2014 Don Gi Min, KM4EQR, more protocols and option handling

// Ported to Raspberry Pi Pico and turned into a library by Alan Johnston, KU2Y

#include "picosstvpp.h"

// =========
// globals
// =========
#ifdef SSTV_PWM
uint16_t   g_audio[2];
#else
uint16_t   g_audio[MAXSAMPLES] ;
#endif
uint32_t   g_scale, g_samples ;
double     g_twopioverrate , g_uspersample ; 
double     g_theta, g_fudge ; 
uint8_t    g_protocol; //VIS ID's for SSTV protocols
long	avg_time = 0;

FILE *     g_imgfp ;
FILE *     g_outfp ;
//gdImagePtr g_imgp ;
uint16_t   g_rate;

 File input_file;
 File output_file;
// File inFile;
  File outFile;
  byte sstv_pwm_pin;
  bool sstv_stop = false;
  bool sstv_stop_write = false;
  unsigned long sstv_micro_timer;
  int sstv_period;
  long prompt_count_max;
  long prompt_count;	

void picosstvpp_begin(int pin) {
	
//	     Serial.println(micros() - sstv_micro_timer);	
  sstv_pwm_pin = pin;	
//  delay(10000);	
  Serial.begin(115200);	
  Serial.println("picosstvpp v0.5 starting");	
//  show_dir4();	
//  load_files();
  LittleFS.remove("/cam.pwm");
  LittleFS.remove("/sstv_image_1_320_x_240.jpg");
  LittleFS.remove("/sstv_image_2_320_x_240.jpg");
//  LittleFS.remove("/cam2.bin");
  LittleFS.remove("/cam.bin");
	
  Serial.println("Deleted .bin and .pwm files");	
  show_dir4();
		

}

// ================
//   main
// ================

// int main(int argc, char *argv[]) {
void picosstvpp() {
	
    g_rate = WRAP * 4400; //RATE;	
  float multiplier;
  int wrap = WRAP;  // was 10; // 5;
  int dds_pin_slice;
  pwm_config dds_pwm_config;
  sstv_period = 1E6 / g_rate;  // clock
  char octet;
  byte lower;
  byte upper;
  Serial.println("Playing PWM file");	

	 
  int  dds_pwm_pin = 26;
   
//    multiplier = 133E6 / (clock * (wrap + 1));
    multiplier = 133E6 / (g_rate * (wrap + 1));
//    multiplier = 125E6 / (clock * (wrap + 1));
	
//    isr_period = (int) ( 1E6 / clock + 0.5);
    
    Serial.printf("Pico PWM Playback v0.5 begin\nClock: %d Wrap: %d Multiplier: %4.1f Period: %d\n", g_rate, wrap, multiplier, sstv_period);

    gpio_set_function(dds_pwm_pin, GPIO_FUNC_PWM);
    dds_pin_slice = pwm_gpio_to_slice_num(dds_pwm_pin);

    dds_pwm_config = pwm_get_default_config();
    pwm_config_set_clkdiv(&dds_pwm_config, multiplier); // was 100.0 50 75 25.0); // 33.333);  // 1.0f
    pwm_config_set_wrap(&dds_pwm_config, wrap); // 3 
    pwm_init(dds_pin_slice, &dds_pwm_config, true);
    pwm_set_gpio_level(dds_pwm_pin, 0); // (dds_pwm_config.top + 1) * 0.5);
  
//    Serial.printf("PWM config.top: %d\n", dds_pwm_config.top);
	
//  delay(1000);	 
	
// while (true) {	
	
//  output_file = LittleFS.open("/cam.pwm", "r");
	
  prompt_count_max = 4 * 1E6 / sstv_period;
  prompt_count = 0;	
	
  sstv_micro_timer = micros();	
	
    char *protocol; 
    int option;
    sstv_stop = false;
    sstv_stop_write = false;

    g_protocol = 56; //Scottie 2
/*	
    while ((option = getopt(argc, argv, "r:p:")) != -1) {
        switch (option) {
            case 'r':
                g_rate = (atoi(optarg));
                break;
            case 'p':
                protocol = optarg;
                break;
        }
    }

    //Set VIS codes
    if ( strcmp(protocol, "m1") == 0 ) {
        g_protocol = 44; //Martin 1
    }
    else if ( strcmp(protocol,"m2") == 0) {
        g_protocol = 40; //Martin 2
    }
    else if ( strcmp(protocol,"s1") == 0) {
        g_protocol = 60; //Scottie 1
    }
    else if ( strcmp(protocol,"s2") == 0) {
        g_protocol = 56; //Scottie 2
    }
    else if ( strcmp(protocol,"sdx") == 0) {
        g_protocol = 76; //Scottie DX
    }
    else if ( strcmp(protocol,"r36") == 0) {
        g_protocol = 8; //Robot 36
    }
    else {
        printf("Unrecognized protocol option %s, defaulting to Martin 1...\n\n", protocol);
        g_protocol = 44;
    }
*/

    // locals
    uint32_t starttime = millis();	 // time(NULL) ;	
    uint8_t ft ; 
    char inputfile[255], outputfile[255] ;
    
    // string hygeine
    memset( inputfile  , 0 , 255 ) ;
    memset( outputfile , 0 , 255 ) ;
    
    // assign values to globals

    double temp1, temp2, temp3 ;
    temp1 = (double)( 1 << (BITS - 1) ) ;
    temp2 = VOLPCT / 100.0 ;
    temp3 = temp1 * temp2 ;
    g_scale = (uint32_t)temp3 ;
    
    g_twopioverrate = 2.0 * M_PI / g_rate ;
    g_uspersample = 1000000.0 / (double)g_rate ;

    g_theta = 0.0 ;
    g_samples = 0.0 ;
    g_fudge = 0.0 ;

    Serial.printf( "Constants check:\n" ) ;
    Serial.printf( "      rate = %d\n" , g_rate ) ;
    Serial.printf( "  VIS code = %d\n" , g_protocol);
#ifndef SSTV_PWM		
    Serial.printf( "      BITS = %d\n" , BITS ) ;
    Serial.printf( "    VOLPCT = %d\n" , VOLPCT ) ; 
    Serial.printf( "     scale = %d\n" , g_scale ) ;
#endif	
    Serial.printf( "   us/samp = %f\n" , g_uspersample ) ;
    Serial.printf( "   2p/rate = %f\n\n" , g_twopioverrate ) ;
/*    
    // set filenames    
    strncpy( inputfile , argv[optind] , strlen( argv[optind] ) ) ;
    ft = filetype( inputfile ) ;
    if ( ft == FILETYPE_ERR ) 
    {
        printf( "Exiting.\n" ) ;
        return 2 ;
    }
    
    strncpy( outputfile, inputfile , strlen( inputfile ) ) ;
    
#ifdef AUDIO_AIFF    
    strcat( outputfile , ".aiff" ) ;
#endif
#ifdef AUDIO_WAV    
    strcat( outputfile , ".wav" ) ;
#endif
    
    printf( "Input  file is [%s].\n" , inputfile ) ;
    printf( "Output file is [%s].\n" , outputfile ) ;
    
    // prep
    
    g_imgfp = fopen( inputfile , "r" ) ;
    g_outfp = fopen( outputfile , "w" ) ;
    printf( "FILE ptrs opened.\n" ) ;
    
    if ( ft == FILETYPE_JPG ) 
    { g_imgp = gdImageCreateFromJpeg( g_imgfp ) ; }
    else if ( ft == FILETYPE_PNG ) 
    { g_imgp = gdImageCreateFromPng( g_imgfp ) ; }
    else {
        printf( "Some weird error!\n" ) ;
        return 3 ;
    }    
*/    
    input_file = LittleFS.open("/cam.bin", "r");
#ifdef WAV	
    output_file = LittleFS.open("/cam.wav", "w+");	
#endif	
#ifdef SSTV_PWM	
//    output_file = LittleFS.open("/cam.pwm", "w+");	
#endif	
/*	
    if (output_file)
      Serial.printf( "Output file opened.\n" );
    else
      Serial.printf( "Error opening output file.\n" );	    
*/
    // go!

    addvisheader() ;
/**/    
    //Selects audio format mode
    switch (g_protocol) {
        case 44: //Martin 1
            buildaudio_m(457.6);
            break;
        case 40: //Martin 2
            buildaudio_m(228.8);
            break;
        case 60: //Scottie 1
            buildaudio_s(432.0);
            break;
        case 56: //Scottie 2
//            buildaudio_s(275.2);
             buildaudio_s(275.1);
		    
//	     buildaudio_s(274.0);  // was 274.4 no stripe (vertical band), 274.7 - 1 stripe instead of 3 for 275.2, 274.9 2 stripes, 275.5 auto ML280
            break;
        case 76: //Scottie DX
            buildaudio_s(1080.0);
            break;
        case 8: //Robot 36
            buildaudio_r36();
            break;
        default:
            Serial.printf("Something went horribly wrong: unknown protocol while building audio!\n");
 //           exit(2);
            break;
    }

    if (!sstv_stop)
      addvistrailer() ;
/**/	
    
#ifdef AUDIO_AIFF    
    writefile_aiff() ;
#endif
#ifdef AUDIO_WAV
    writefile_wav();
#endif    

    // cleanup

//    fclose( g_imgfp ) ;
//    fclose( g_outfp ) ;
	
    input_file.close();
    output_file.close();	
    // brag
    
    uint32_t endtime = millis();	// time(NULL) ;
    Serial.printf( "Created output file in %d milliseconds.\n" , ( endtime - starttime ) ) ;
	
//    show_dir4();
//    delay(1000);	
//    play_pwm_file();
//    delay(10000);	
//    return 0 ;
}


// =====================
//  subs 
// =====================    


// filetype -- Check to see if input file is in one of our
//             supported formats (currently jus JPEG and PNG).
//             Uses libmagic.

/*
uint8_t filetype( char *filename ) {
    magic_t m ;
    char m_str[ MAGIC_CNT + 2 ] ;
    uint8_t retval ;
    
    printf( "  Checking filetype for file [%s]\n" , filename ) ;
    
    retval = FILETYPE_ERR ;
    
    m = magic_open( MAGIC_NONE ) ;
    if ( m && ( magic_load(m, NULL) == 0 ) )
    {
        strncpy(m_str, magic_file(m, filename), MAGIC_CNT+1) ;
        
        if ( strncmp(m_str, MAGIC_JPG, MAGIC_CNT) == 0 )
        { 
            printf( "  File is a JPEG image.\n" ) ;
            retval = FILETYPE_JPG ; 
        }
        
        else if ( strncmp(m_str, MAGIC_PNG, MAGIC_CNT) == 0 )
        { 
            printf( "  File is a PNG image.\n" ) ;
            retval = FILETYPE_PNG ; 
        }
        
        else
        {
            printf( "  This file format is not supported!\n" ) ;
            printf( "  Please use a JPEG or PNG file instead.\n" ) ;
        }    
    }
    
    if ( m ) { magic_close(m) ; }
    
    return retval ;
}
*/

// playtone -- Add waveform info to audio data. New waveform data is 
//             added in a phase-continuous manner according to the 
//             audio frequency and duration provided. Note that the
//             audio is still in a purely hypothetical state - the
//             format of the output file is not determined until 
//             the file is written, at the end of the process.
//             Also, yes, a nod to Tom Hanks.

void playtone( uint16_t tonefreq , double tonedur ) {
#ifdef SSTV_PWM
    int voltage;
    tonedur *= 0.97;
    tonefreq *= 1.014;	
#else
    uint16_t voltage;	
#endif	
    uint16_t tonesamples, i ;
    double   deltatheta ;
    
//    tonefreq += 20;  // increase frequency by 20 Hz
//    tonefreq *= 1.01333;  // increase frequency by scale.

	
    tonedur += g_fudge ;
    tonesamples = ( tonedur / g_uspersample ) + 0.5 ;
/*	
    if (tonesamples > MAXSAMPLES) {
	Serial.printf("Tonesamples overflow: %d \n", tonesamples); 
	tonesamples = MAXSAMPLES - 1;    
    }
*/	
    deltatheta = g_twopioverrate * tonefreq ;
    
//    for ( i=1 ; (i<=tonesamples && !sstv_stop_write && !sstv_stop && !Serial.available() && !BOOTSEL && digitalRead(10)); i++ ) {
    for ( i=1 ; (i<=tonesamples && !sstv_stop_write && !sstv_stop); i++ ) {
#ifdef AUDIO_AIFF        
        g_samples++ ;
        
        if ( tonefreq == 0 ) { g_audio[i] = 32768 ; }
        else {

            voltage = 32768 + (int)( sin( g_theta ) * g_scale ) ;
            g_audio[i] = voltage ;
            g_theta += deltatheta ;

        }
#endif
#ifdef AUDIO_WAV
        g_samples++ ;
        
        if ( tonefreq == 0 ) { g_audio[i] = 32768 ; }
        else {

            voltage =     0 + (int)( sin( g_theta ) * g_scale ) ;
            g_audio[i] = voltage ;
            g_theta += deltatheta ;

        }
#endif    
#ifdef SSTV_PWM
//	for(int j = 0; j < 2; j++) {	
          g_samples++ ;
 
//          if ( tonefreq == 0 ) { g_audio[j] = 32768 ; }		
          if ( tonefreq == 0 ) 
//	      g_audio[j] = (WRAP + 1)/2; 
	      voltage = (WRAP + 1)/2; 
          else {

//            voltage =     3 + (int)( sin( g_theta ) * 4.0 ) ;  // wrap 5+1
//            voltage =     (WRAP + 1)/2 + (int)( sin( g_theta ) * (float)((WRAP + 1)/2 + 0)) ; //   range is 1 to wrap - 1
            voltage =     (WRAP + 1)/2 + (int)( sin( g_theta ) * (float)((WRAP + 1)/2 + 1)) ; //   range is 1 to wrap - 1
	    if (voltage < 0)
	      voltage = 0;	    
/*		  
           int sine =  sin( g_theta ) * (float)((WRAP + 1)/2 + 1);
	   if (sine < ((-1)*((WRAP + 1)/2)))
	     sine = (WRAP + 1)/2;	   
           voltage = (WRAP + 1)/2 + sine ; 
*/		  
		  
//	    Serial.println(voltage);	
//	    g_audio[j] = voltage ;
            g_theta += deltatheta ;	

          }
// play it	
	 while ((micros() - sstv_micro_timer) < sstv_period)    { }
	 busy_wait_at_least_cycles(56);	// 454 ns    
///         avg_time += (micros() - sstv_micro_timer);
//    	 pwm_set_gpio_level(dds_pwm_pin, voltage);
    	 pwm_set_gpio_level(sstv_pwm_pin, voltage);
    	 sstv_micro_timer = micros();  
	    
//	 Serial.print(voltage);
//	 Serial.print(",");   
/*	  

	    prompt_count++;
	    if (prompt_count > prompt_count_max) {
		prompt_count = 0;
	//	Serial.println("Prompt!\n");   
		if (Serial.available() || BOOTSEL || !digitalRead(10))
//		if (BOOTSEL || !digitalRead(10))
		  sstv_stop = true;  
//		Serial.printf("a: %f4.1 ", (float)avg_time/(float)prompt_count_max); 
//		avg_time = 0;    
	    }
	    
*/	    
//	}
/*	    
	byte octet = (g_audio[0] & 0xf) + (((g_audio[1] & 0xf)) << 4);    
	int result = output_file.write(octet);
	if (result < 1) {
	  sstv_stop_write = true;
	  Serial.println("Output file write error");	
	}
*/	    
//	i++;   	    
#endif  		

    } // end for i        
    
    g_fudge = tonedur - ( tonesamples * g_uspersample ) ;
/*	
    for ( i=1 ; i<=tonesamples ; i++ ) {
#ifdef SSTV_PWM
 
#endif
//	output_file.write(g_audio[i] & 0xff00);  
    }
*/	
}  // end playtone    

/*
uint16_t toneval_rgb ( uint8_t colorval ) {
    return ( ( 800 * colorval ) / 256 ) + 1500 ;
}
*/
#define toneval_rgb(colorval) ((int)((( 800.0 * (float)colorval) / 256.0) + 0.5) + 1500) 

uint16_t toneval_yuv ( uint8_t colorval ) {
    return ( (float)colorval * 3.1372549 ) + 1500.0 ;
}    

// addvisheader -- Add the specific audio tones that make up the 
//                 Martin 1 VIS header to the audio data. Basically,
//                 this just means lots of calls to playtone(). 

void addvisheader() {
    printf( "Adding VIS header to audio data.\n" ) ;
    
    // bit of silence

/*	
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
    playtone(1500, 900000);
	
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);
    playtone(2000, 900000);	
	
//  sstv_stop_write = true;
	
//    return;	
*/	
    playtone(    0 , 500000 ) ;   
	
    // attention tones
    playtone( 1900 , 100000 ) ; // you forgot this one
    playtone( 1500 , 100000 ) ;
    playtone( 1900 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
                    
    // VIS lead, break, mid, start
    playtone( 1900 , 300000 ) ;
    playtone( 1200 ,  10000 ) ;
    playtone( 1900 , 300000 ) ;
    playtone( 1200 ,  30000 ) ;

    int i;
    int parity = 0;

    //write VIS data tones
    for(i = 1; i <= 64; i = i<<1) {
        if(i & g_protocol) { //digit is a 1
            playtone(1100, 30000);
            parity = !parity;
        }
        else {
            playtone(1300, 30000); //digit is a 0
        }
    }

    //parity bit
    if(parity) //odd parity
        playtone(1100, 30000);
    else       //even parity
        playtone(1300, 30000);

    // VIS stop
    playtone( 1200 ,  30000 ) ; 
    Serial.printf( "Done adding VIS header to output file.\n" ) ;
        
} // end addvisheader   

//Builds audio scan data for the Martin series of specifications.
//Applicable to Martin 1 and Martin 2 SSTV modes
//Each pixel is scanned for pixeltime microseconds
void buildaudio_m (double pixeltime) {
    uint16_t x , y , k ;
    uint32_t pixel ;
    uint8_t r[320], g[320], b[320] ;
    
    char buff[3];
	
     for ( y=0 ; y<256 ; y++ ) {
	        
        // read image data
        for ( x=0 ; x<320 ; x++ ) {
//            pixel = gdImageGetTrueColorPixel( g_imgp, x, y ) ;
		
//	 input_file.readBytes(buff, 3);
          
          r[x] =  buff[0];
          g[x] =  buff[1];
          b[x] =  buff[2];	
            //printf( "Got pixel.\n" ) ;
            
            // get color data
//            r[x] = gdTrueColorGetRed( pixel ) ;
//            g[x] = gdTrueColorGetGreen( pixel ) ;
//            b[x] = gdTrueColorGetBlue( pixel ) ;
        }
        
        // add row markers to audio
        // sync
        playtone( 1200 , 4862 ) ;
        // porch 
        playtone( 1500 ,  572 ) ;
        
        // each pixel is 457.6us long in Martin 1
        
        // add audio for green channel for this row
        for ( k=0 ; k<320 ; k++ )
        { playtone( toneval_rgb( g[k] ) , pixeltime ) ; }
        
        // separator tone 
        playtone( 1500 , 572 ) ;
        
        // bloo channel
        for ( k=0 ; k<320 ; k++ )
        { playtone( toneval_rgb( b[k] ) , pixeltime ) ; }

        playtone( 1500 , 572 ) ;

        // red channel
        for ( k=0 ; k<320 ; k++ )
        { playtone( toneval_rgb( r[k] ) , pixeltime ) ; }

        playtone( 1500 , 572 ) ;
        
    }  // end for y
    
    printf( "Done adding image to audio data.\n" ) ;    
    
}  // end buildaudio_m   

//Builds audio scan data for the Scottie series of specifications.
//Applicable to Scottie 1, Scottie 2, Scottie  SSTV modes
//Each pixel is scanned for pixeltime microseconds
void buildaudio_s (double pixeltime) {
    uint16_t x , y , k ;
    uint32_t pixel ;
    uint8_t r[320], g[320], b[320]; 
    char buff_row[320 * 2] ;
    
    Serial.printf( "Adding image to output file.\n" ) ;
	
//    char buff[3];
    char buff[2];
    uint16_t pixel_value;	
	    
    //add starting sync pulse
    playtone( 1200 , 9000);

//    for ( y=0 ; y<256 ; y++ ) {
//    for ( y=0 ; y<100 ; y++ ) {
    for ( y=0 ; ((y<240) && !sstv_stop_write  && !sstv_stop) ; y++ ) {
        // read image data
//	Serial.println("Starting row");    
	input_file.readBytes(buff_row, sizeof(buff_row));    
        for ( x=0 ; ((x<320) && !sstv_stop_write && !sstv_stop) ; x++ ) {
/*
	if ( x < 100) {
		r[x] = 0xff;
		g[x] = 0;		
		b[x] = 0;	
	}
	else if ( x < 200) {
		r[x] = 0;
		g[x] = 0xff;		
		b[x] = 0;	
	}
	else {	
		r[x] = 0;
		g[x] = 0;		
		b[x] = 0xff;	
	}
*/		
		/**/
//	    input_file.readBytes(buff, 3);
///	    input_file.readBytes(buff, 2);
		
//	    pixel_value = buff[0] + (buff[1] << 8);  // back		
	    pixel_value = buff_row[2 * x] + (buff_row[(2 * x) + 1] << 8);  
/*		
           Serial.print(pixel_value, HEX);
  	   Serial.print(" ^ ");
		
           Serial.print(buff[1], HEX);
	  Serial.print(" ");
           Serial.print(buff[0], HEX);
	  Serial.print(" | ");
*/		
            byte red_raw = (pixel_value & 0b1111100000000000) >> 11;
            byte green_raw = (pixel_value & 0b0000011111100000) >> 5;         
            byte blue_raw = (pixel_value & 0b0000000000011111);   
/*		
	    Serial.print(red_raw, HEX);
	    Serial.print(" ");
	    Serial.print(green_raw, HEX);
	    Serial.print(" ");
	    Serial.print(blue_raw, HEX);
	    Serial.print(" / ");			
*/		
            r[x] = (float)(red_raw) * 255.0/31.0;
            g[x] = (float)(green_raw) * 255.0/63.0;
            b[x] = (float)(blue_raw) * 255.0/31.0;    	
/*		
	    Serial.print(r[x], HEX);
	    Serial.print(" ");
	    Serial.print(g[x], HEX);
	    Serial.print(" ");
	    Serial.print(b[x], HEX);
	    Serial.print(" + ");	
*/		
/**/          
//          r[x] =  buff[0];
//          g[x] =  buff[1];
//          b[x] =  buff[2];	
		
//            pixel = gdImageGetTrueColorPixel( g_imgp, x, y ) ;
            
            // get color data
//            r[x] = gdTrueColorGetRed( pixel ) ;
//            g[x] = gdTrueColorGetGreen( pixel ) ;
//            b[x] = gdTrueColorGetBlue( pixel ) ;
        }
        //seperator pulse
        playtone(1500, 1500);
        
        // add audio for green channel for this row
        for ( k=0 ; ((k<320) && !sstv_stop_write && !sstv_stop)  ; k++ )
//	    if (k >8)	
//	    if ((k >8) && (k < 300))	
//	    if (true)	 
	    if ((k < 305))			
              playtone( toneval_rgb( g[k] ) , pixeltime ) ;
	    else
              playtone( toneval_rgb(0) , pixeltime ); 		    	
		

        // separator tone 
        playtone(1500, 1500) ;
	
	int kk;    

        // blue channel
        for ( k=0 ; ((k<320) && !sstv_stop_write && !sstv_stop) ; k++ )
	{
	    if (k > 5) kk = k - 5; else kk = 2;  // k	
//	    if ((kk >8) && (kk < 300))	
//	    if (true)	 
	    if ((kk < 305))		    
              playtone( toneval_rgb( b[kk] ) , pixeltime ) ; 
	    else
              playtone( toneval_rgb(0) , pixeltime ); 		    
	}

        //sync pulse
        playtone(1200, 9000);

        //sync porch
        playtone(1500 , 1500) ;

        // red channel
        for ( k=0 ; ((k<320) && !sstv_stop_write && !sstv_stop)  ; k++ )
	{
	    if (k > 10) kk = k - 10; else kk = 2;	 //k
//	    if ((kk >8) && (kk < 300))	
//	    if (true)	   
	    if ((kk < 305))	
		    
              playtone( toneval_rgb( r[kk] ) , pixeltime ) ;
	    else
              playtone( toneval_rgb(0) , pixeltime ); 			
		        
	}
//       Serial.println("Ending row");    
    }  // end for y
    
    Serial.printf( "Done adding image to audio data.\n" ) ;    
    
}  // end buildaudio_s


//Builds audio scan data for the Robot 36.
//Applicable to only Robot 36.
void buildaudio_r36 () {

    uint16_t x , y , k ;
    uint32_t pixel1, pixel2;
    uint8_t r1, g1, b1, r2, g2, b2, avgr, avgg, avgb;
    uint8_t y1[320], y2[320], ry[320], by[320];

    Serial.printf( "Adding image to audio data.\n" ) ;
    
    for ( y=0 ; y<240 ; y+=2 ) {
    
        // read image data
        for ( x=0 ; x<320 ; x++ ) {
/*
            //even line pixel
            pixel1 = gdImageGetTrueColorPixel( g_imgp, x, y ) ;
            //odd line pixel 
            pixel2 = gdImageGetTrueColorPixel( g_imgp, x, y+1) ;
            
            // get color data
            r1 = gdTrueColorGetRed( pixel1 ); //first line (even) of red intensities
            r2 = gdTrueColorGetRed( pixel2 ); //second line (odd) of red intensities
            g1 = gdTrueColorGetGreen( pixel1 );
            g2 = gdTrueColorGetGreen( pixel2 );
            b1 = gdTrueColorGetBlue( pixel1 );
            b2 = gdTrueColorGetBlue( pixel2 );
*/
            avgr = (uint8_t)( ((uint16_t)r1 + (uint16_t)r2) / 2 );

            avgg = (uint8_t)( ((uint16_t)g1 + (uint16_t)g2) / 2 );

            avgb = (uint8_t)( ((uint16_t)b1 + (uint16_t)b2) / 2 );

            //Y value of even lines 
            y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + (129.057 * (float)g1) + (25.064 * (float)b1)));
            //Y value of odd lines
            y2[x] = 16.0 + (0.003906 * ((65.738 * (float)r2) + (129.057 * (float)g2) + (25.064 * (float)b2)));
            //R-Y value of the average of the two lines, to be transmitted even scans
            ry[x] = 128.0 + (0.003906 * ((112.439 * (float)avgr) + (-94.154 * (float)avgg) + (-18.285 * (float)avgb)));
            //B-Y value of the average of the two lines, to be transmitted odd scans
            by[x] = 128.0 + (0.003906 * ((-37.945 * (float)avgr) + (-74.494 * (float)avgg) + (112.439 * (float)avgb)));

        }
        
        //begin robot 36 code

        //even lines    
        //sync
        playtone( 1200 , 9000 ) ;
        //porch 
        playtone( 1500 , 3000 ) ;
        
        //y scan, even, 88ms total, 320 points, 275us per pixel
        for ( k = 0; k < 320; k++ ) {
            playtone( toneval_yuv( y1[k] ) , 275 ) ;
        }
        
        //even line seperator
        playtone( 1500 , 4500 );
        //porch
        playtone( 1900 , 1500 );

        //R-Y scan, 44ms total, 320 points, 137.5us per pixel
        for ( k = 0; k < 320; k++ ) {
            playtone( toneval_yuv( ry[k] ) , 137.5 );
        }

        //odd lines
        // sync
        playtone( 1200 , 9000 ) ;
        // porch 
        playtone( 1500 , 3000 ) ;

        //y scan, odd, 88ms total, 320 points, 275us per pixel
        for ( k = 0; k < 320; k++ ) {
                playtone( toneval_yuv( y2[k] ) , 275 ) ;
        }

        //odd line seperator
        playtone( 2300 , 4500 );
        //porch
        playtone( 1900 , 1500 );

        //B-Y scan, 44ms total, 320 points, 137.5us per pixel
        for ( k = 0; k < 320; k++) {
            playtone( toneval_yuv( by[k] ) , 137.5);
        }
  
    }  // end for y
    
    Serial.printf( "Done adding image to audio data.\n" ) ;    
    
}  // end buildaudio_r36

// addvistrailer -- Add tones for VIS trailer to audio stream.
//                  More calls to playtone(). 

void addvistrailer () {
    printf( "Adding VIS trailer to audio data.\n" ) ;
    
    playtone( 2300 , 300000 ) ;
    playtone( 1200 ,  10000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1200 ,  30000 ) ;
    
    // bit of silence
    playtone(    0 , 500000 ) ;
    
    printf( "Done adding VIS trailer to audio data.\n" ) ;    
}

// writefile_aiff -- Save audio data to an AIFF file. Playback for
//                   AIFF format files is tricky. This worked on 
//                   ARM Linux:
//                     aplay -r 11025 -c 1 -f U16_BE file.aiff
//                   The WAV output is much easier and more portable, 
//                   but who knows - this code might be useful for 
//                   something. 

#ifdef AUDIO_AIFF
void writefile_aiff () {
    uint32_t totalsize , audiosize , i ;
    audiosize = 8 + ( 2 * g_samples ) ;      // header + 2bytes/samp
    totalsize = 4 + 8 + 18 + 8 + audiosize ;

    Serial.printf( "Writing audio data to file.\n" ) ;
    Serial.printf( "Got a total of [%d] samples.\n" , g_samples ) ;
    
    // "form" chunk
    fputs( "FORM" , g_outfp ) ;
    fputc( (totalsize & 0xff000000) >> 24 , g_outfp ) ;
    fputc( (totalsize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (totalsize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (totalsize & 0x000000ff)       , g_outfp ) ;    
    fputs( "AIFF" , g_outfp ) ;
    
    // "common" chunk
    fputs( "COMM" , g_outfp ) ;
    fputc(    0 , g_outfp ) ;   // size
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(   18 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;   // channels = 1
    fputc(    1 , g_outfp ) ;
    fputc( (g_samples & 0xff000000) >> 24 , g_outfp ) ;   // size
    fputc( (g_samples & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (g_samples & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (g_samples & 0x000000ff)       , g_outfp ) ;    
    fputc(    0 , g_outfp ) ;  // bits/sample
    fputc(   16 , g_outfp ) ;
    fputc( 0x40 , g_outfp ) ;  // 10 byte sample rate (??)
    fputc( 0x0c , g_outfp ) ;  // <--- 11025
    fputc( 0xac , g_outfp ) ;
    fputc( 0x44 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    // audio data chunk
    fputs( "SSND" , g_outfp ) ;
    fputc( (audiosize & 0xff000000) >> 24 , g_outfp ) ;
    fputc( (audiosize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (audiosize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (audiosize & 0x000000ff)       , g_outfp ) ;    
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    // FINALLY, the audio data itself
    for ( i=0 ; i<=g_samples ; i++ )
    {
        fputc( ( g_audio[i] & 0xff00 ) >> 8 , g_outfp ) ;
        fputc( ( g_audio[i] & 0x00ff )      , g_outfp ) ;
    }
    
    Serial.printf( "Done writing to audio file.\n" ) ;
}
#endif 

// writefile_wav -- Write audio data to a WAV file. Once the file
//                  is written, any audio player in the world ought
//                  to be able to play the file without any funky
//                  command-line params.

#ifdef AUDIO_WAV
void writefile_wav () {
    uint32_t totalsize , audiosize , byterate , blockalign ;
    uint32_t i ;
    
    audiosize  = g_samples * CHANS * (BITS / 8) ; // bytes of audio
    totalsize  = 4 + (8 + 16) + (8 + audiosize) ; // audio + some headers  
    byterate   = g_rate * CHANS * BITS / 8 ;        // audio bytes / sec
    blockalign = CHANS * BITS / 8 ;               // total bytes / sample
    
    Serial.printf( "Writing audio data to file.\n" ) ;
    Serial.printf( "Got a total of [%d] samples.\n" , g_samples ) ;
/*    
    // RIFF header 
    fputs( "RIFF" , g_outfp ) ;
    
    // total size, audio plus some headers (LE!!)
    fputc( (totalsize & 0x000000ff)       , g_outfp ) ;    
    fputc( (totalsize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (totalsize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (totalsize & 0xff000000) >> 24 , g_outfp ) ;
    fputs( "WAVE" , g_outfp ) ;
    
    // sub chunk 1 (format spec)
    
    fputs( "fmt " , g_outfp ) ;  // with a space!
    
    fputc(   16 , g_outfp ) ;   // size of chunk (LE!!)
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    fputc(    1 , g_outfp ) ;   // format = 1 (PCM) (LE)
    fputc(    0 , g_outfp ) ;
    
    fputc(    1 , g_outfp ) ;   // channels = 1 (LE)
    fputc(    0 , g_outfp ) ;
    
    // samples / channel / sec (LE!!)
    fputc( (g_rate & 0x000000ff)       , g_outfp ) ;
    fputc( (g_rate & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (g_rate & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (g_rate & 0xff000000) >> 24 , g_outfp ) ;

    // bytes total / sec (LE!!)
    fputc( (byterate & 0x000000ff)       , g_outfp ) ;    
    fputc( (byterate & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (byterate & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (byterate & 0xff000000) >> 24 , g_outfp ) ;

    // block alignment (LE!!)
    fputc( (blockalign & 0x00ff)       , g_outfp ) ;    
    fputc( (blockalign & 0xff00) >>  8 , g_outfp ) ;    
    
    fputc( (BITS & 0x00ff)       , g_outfp ) ;   // bits/sample (LE)
    fputc( (BITS & 0xff00) >>  8 , g_outfp ) ; 

    // sub chunk 2
    // header
    fputs( "data" , g_outfp ) ;
    
    // audio bytes total (LE!!)
    fputc( (audiosize & 0x000000ff)       , g_outfp ) ;    
    fputc( (audiosize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (audiosize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (audiosize & 0xff000000) >> 24 , g_outfp ) ;
*/    
    // FINALLY, the audio data itself (LE!!)
    for ( i=0 ; i<=g_samples ; i++ ) {
//        fputc( ( g_audio[i] & 0x00ff )      , g_outfp ) ;
//        fputc( ( g_audio[i] & 0xff00 ) >> 8 , g_outfp ) ;
	output_file.write(g_audio[i] & 0x00ff);  
	output_file.write(g_audio[i] & 0xff00);  
    }

    // no trailer    
    Serial.printf( "Done writing to audio file.\n" ) ;
}
#endif
        
// end

void show_dir4() {
  LittleFS.begin();
  Dir dir = LittleFS.openDir("/");
// or Dir dir = LittleFS.openDir("/data");
  Serial.println("FS directory:");
  while (dir.next()) {
    Serial.print(dir.fileName());
    if(dir.fileSize()) {
        File f = dir.openFile("r");
        Serial.print(" ");
        Serial.println(f.size());
    } else
        Serial.println(" 0");	    
  }
  Serial.println(">");
}
/*
void load_files() {
  LittleFS.begin();
  File f;
	
  f = LittleFS.open("sstv_image_1_320_x_240.jpg", "r");
  if (f) {	
    Serial.println("Image sstv_image_1_320_x_240.jpg already in FS");
    f.close();
  } else {
    Serial.println("Loading image sstv_image_1_320_x_240.jpg into FS");
    f = LittleFS.open("sstv_image_1_320_x_240.jpg", "w+");
    if (f.write(sstv_image_1_320_x_240, sizeof(sstv_image_1_320_x_240)) < sizeof(sstv_image_1_320_x_240)) {
       Serial.println("Loading image failed. Is Flash Size (FS) set to 512kB?");	     
       delay(2000);
    }
    f.close();
  }

  f = LittleFS.open("sstv_image_2_320_x_240.jpg", "r");
  if (f) {	
    Serial.println("Image sstv_image_2_320_x_240.jpg already in FS");
    f.close();
  } else {
    Serial.println("Loading image sstv_image_2_320_x_240.jpg into FS");
    f = LittleFS.open("sstv_image_2_320_x_240.jpg", "w+");
    if (f.write(sstv_image_2_320_x_240, sizeof(sstv_image_2_320_x_240)) < sizeof(sstv_image_2_320_x_240)) {
       Serial.println("Loading image failed. Is Flash Size (FS) set to 512kB?");
       delay(2000);
    }
    f.close();
  }
  show_dir();
}
*/

void play_pwm_file(int dds_pwm_pin) {
	
//  set_sys_clock_khz(133000, true);
	
//  #define DDS_PWM_PIN 26
//   = DDS_PWM_PIN;	
//  int clock = RATE; // 11.025E3; // was 22E3 50E3;
  float multiplier;
  int wrap = WRAP;  // was 10; // 5;
  int dds_pin_slice;
  pwm_config dds_pwm_config;
  int period = 1E6 / g_rate;  // clock;
  char octet;
  byte lower;
  byte upper;
  Serial.println("Playing PWM file");	
  unsigned long sstv_micro_timer;
	 
    dds_pwm_pin = 26;
   
//    multiplier = 133E6 / (clock * (wrap + 1));
    multiplier = 133E6 / (g_rate * (wrap + 1));
//    multiplier = 125E6 / (clock * (wrap + 1));
	
//    isr_period = (int) ( 1E6 / clock + 0.5);
    
    Serial.printf("Pico PWM Playback v0.3 begin\nClock: %d Wrap: %d Multiplier: %4.1f Period: %d\n", g_rate, wrap, multiplier, period);

    gpio_set_function(dds_pwm_pin, GPIO_FUNC_PWM);
    dds_pin_slice = pwm_gpio_to_slice_num(dds_pwm_pin);

    dds_pwm_config = pwm_get_default_config();
    pwm_config_set_clkdiv(&dds_pwm_config, multiplier); // was 100.0 50 75 25.0); // 33.333);  // 1.0f
    pwm_config_set_wrap(&dds_pwm_config, wrap); // 3 
    pwm_init(dds_pin_slice, &dds_pwm_config, true);
    pwm_set_gpio_level(dds_pwm_pin, 0); // (dds_pwm_config.top + 1) * 0.5);
  
//    Serial.printf("PWM config.top: %d\n", dds_pwm_config.top);
	
//  delay(1000);	 
	
// while (true) {	
	
  output_file = LittleFS.open("/cam.pwm", "r");
	
  long prompt_count_max = 1E6 / period;
  long prompt_count = 0;	
	
  sstv_micro_timer = micros();		
  while (output_file.available() && !sstv_stop) {	
//  while (!sstv_stop) {	  
	  
    output_file.readBytes(&octet, 1);
    lower = octet & 0x0f;
    upper = (octet & 0xf0) >> 4;
//    Serial.printf("%d\n%d\n", lower, upper);	
	  
    while ((micros() - sstv_micro_timer) < period)	{ }   	  
    pwm_set_gpio_level(dds_pwm_pin, lower);
    sstv_micro_timer = micros();	
	  
    while ((micros() - sstv_micro_timer) < period)    { }   	
    pwm_set_gpio_level(dds_pwm_pin, upper);
    sstv_micro_timer = micros();
	  
    prompt_count++;
    if (prompt_count > prompt_count_max) {
	prompt_count = 0;
//	Serial.println("Prompt!\n");   
	if (Serial.available() || BOOTSEL || !digitalRead(10))
	  sstv_stop = true;  	 
    }
	  
  }
	
  Serial.println("End");
  output_file.close();	

//  delay(5000);	 	 
// }	 
}

int JpegDec_i;
int JpegDec_j;
int JpegDec_height = 240;
int JpegDec_width = 320;
//byte  JpegDec_sortBuf[15360]; //320(px)*16(lines)*3(bytes) // Header buffer
byte  JpegDec_sortBuf[320 * 16 * 2]; //320(px)*16(lines)*2(bytes) // Header buffer
int JpegDec_pxSkip;
uint8_t *JpegDec_pImg;
int JpegDec_x, JpegDec_y, JpegDec_bx, JpegDec_by;
int JpegDec_comps = 3;
  
bool get_block(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
#ifdef DEBUG
  Serial.println("\nBlock callback");
  Serial.println(x);
  Serial.println(y);
  Serial.println(w);
  Serial.println(h);
#endif
  int JpegDec_MCUx = x;
  int JpegDec_MCUy = y;
  int JpegDec_MCUHeight = h;
  int JpegDec_MCUWidth = w;

//  JpegDec_pImg = bitmap;
  
//    JpegDec_pImg = JpegDec_pImage;
    for(JpegDec_by=0; JpegDec_by<JpegDec_MCUHeight; JpegDec_by++){
      for(JpegDec_bx=0; JpegDec_bx<JpegDec_MCUWidth; JpegDec_bx++){
//        JpegDec_x = JpegDec_MCUx * JpegDec_MCUWidth + JpegDec_bx;
//        JpegDec_y = JpegDec_MCUy * JpegDec_MCUHeight + JpegDec_by;
        JpegDec_x = JpegDec_MCUx + JpegDec_bx;
        JpegDec_y = JpegDec_MCUy + JpegDec_by;
        if(JpegDec_x<JpegDec_width && JpegDec_y<JpegDec_height){
//          if(JpegDec_comps == 1){ // Grayscale
//            //sprintf(str,"%u", pImg[0]);
//            outFile.write(JpegDec_pImg, 1);
//          }else
        { // RGB
            // When saving to the SD, write 16 lines on one time
            // First we write on the array 16 lines and then we save to SD
            JpegDec_pxSkip = ((JpegDec_y - (16 * JpegDec_j)) * 320) + JpegDec_x;
          
//            int pixel_value = *bitmap;
            uint16_t pixel_value = *bitmap;
/*	
	        if (y < 100) 
		    pixel_value = 0xf800;
		else if (y < 200)
		    pixel_value = 0x7E0;
		else
		    pixel_value = 0x1f;
*/			
                      
//            byte red_raw = (pixel_value & 0b1111100000000000) >> 11;
//            byte green_raw = (pixel_value & 0b0000011111100000) >> 5;         
//            byte blue_raw = (pixel_value & 0b0000000000011111);        
		
/*
            byte red = (float)((pixel_value & 0b1111100000000000) >> 11) * 255.0/31.0;
            byte green = (float)((pixel_value & 0b0000011111100000) >> 5) * 255.0/63.0;
            byte blue = (float)(pixel_value & 0b0000000000011111) * 255.0/31.0;  
		
	    Serial.print(red, HEX);
	    Serial.print(" ");
	    Serial.print(green, HEX);
	    Serial.print(" ");
	    Serial.print(blue, HEX);
	    Serial.print(" ");	
*/	 
		
//            JpegDec_sortBuf[(3 * JpegDec_pxSkip) + 0] = red;  // JpegDec_pImg[0];
//            JpegDec_sortBuf[(3 * JpegDec_pxSkip) + 1] = green; // JpegDec_pImg[1];
//            JpegDec_sortBuf[(3 * JpegDec_pxSkip) + 2] = blue; // JpegDec_pImg[2];

            JpegDec_sortBuf[(2 * JpegDec_pxSkip) + 0] = pixel_value & 0xff;  // JpegDec_pImg[0];
            JpegDec_sortBuf[(2 * JpegDec_pxSkip) + 1] = (pixel_value & 0xff00) >> 8; // JpegDec_pImg[1];
/*
	    Serial.print((pixel_value & 0xff00) >> 8, HEX);		
	    Serial.print(pixel_value & 0xff, HEX);
//	    Serial.print(" ");
	    Serial.print(" ");			
		
*/		
#ifdef DEBUG          
            Serial.print("sortBuf index = ");
//            Serial.println((3 * JpegDec_pxSkip));
            Serial.println((2 * JpegDec_pxSkip));

#endif
            JpegDec_i++;
            if(JpegDec_i == 5120){ //320(px)x16(lines)
#ifdef DEBUG
              Serial.println("Writing lines!");
#endif
              //              for(k = 0; k < 15360; k++){
//                imgFile.write(sortBuf[k]);
//              }
              outFile.write(JpegDec_sortBuf, sizeof(JpegDec_sortBuf));
              JpegDec_i = 0;
              JpegDec_j++; //15(sections)
            }
          }
        }
//        JpegDec_pImg += JpegDec_comps ;
        bitmap++;
      }
    }
#ifdef DEBUG  
  Serial.println("Block processed!");
#endif
  return 1;
}

bool jpeg_decode(char* filename, char* fileout, bool debug){
  uint8_t *pImg;
//  uint16_t *pImg;
  int x,y, bx,by;
//  byte sortBuf[15360]; //320(px)*16(lines)*3(bytes) // Header buffer
  int i,j,k;
  int pxSkip;
  
  // Open the file for writing
//  File imgFile = SD.open(fileout, FILE_WRITE);
  outFile = LittleFS.open(fileout, "w+");
  
  if (outFile) {
//    if (debug)
      Serial.printf("Output opened %s\n", fileout);
  }
  else {
    Serial.println("Failed to open output");
    return false;
  }
/*  
  for(i = 0; i < 15360; i++){ // Cleaning Header Buffer array
    sortBuf[i] = 0xFF;
  }
*/
  // Decoding start
  
  if (debug)
    Serial.println("Starting jpeg decode");
  
  JpegDec_i = 0;
  JpegDec_j = 0;
  
  uint16_t w = 0, h = 0;
  // TJpgDec.getFsJpgSize(&w, &h, "/cam.jpg", LittleFS); // Note name preceded with "/"
  TJpgDec.getFsJpgSize(&w, &h, filename, LittleFS); // Note name preceded with "/"
  
  if (debug) {
    Serial.print("Width = "); 
    Serial.print(w); 
    Serial.print(", height = "); 
    Serial.println(h);
  }
  
  if ((w == 0) && (h == 0)) {
    Serial.println("Failed to open jpeg input");
    return false;
  }
//  counter = 0;
//  write_complete = false;
  
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(false);    // was true
  TJpgDec.setCallback(get_block);  
  //TJpgDec.drawFsJpg(0, 0, "/cam.jpg", LittleFS);
  TJpgDec.drawFsJpg(0, 0, filename, LittleFS);
  
  if (debug)
    Serial.println("Draw complete");
  
//  while (!write_complete) { Serial.println("Waiting..."); delay(500);}

/*  
  JpegDec.decodeFile(filename);
  // Image Information
  Serial.print("Width     :");
  Serial.println(JpegDec.width);
  Serial.print("Height    :");
  Serial.println(JpegDec.height);
  Serial.print("Components:");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height:");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("");
*/
  if (debug)
    Serial.println("Writing bin to FS");

//  imgFile.write(JpegDec.pImage, sizeof(JpegDec.pImage));
  

  if (debug)
    Serial.println("Bin has been written to FS");
  outFile.close();
  return true;	
}

void sstv_end() {
  sstv_stop = true;
}

/*******************************************************************************
 * @title: GrippyBot Remote Control
 * @version: 0.0.1
 * @status: alpha
 * @description: Remote control for the grippybot using SDL 1.2 Library to 
 * manage input from a GAMEPAD (playstation type). Sorry but for the moment you 
 * have to  customize the buttons for your gamepad in the SDL_JOYBUTTONDOWN and 
 * SDL_JOYAXISMOTION events. (code based on the SDL1.2 sample)
 *
 * In this version the 4 analogic axis are bound to the 4 servomotors of the arm.
 * button 0 and 1 are bound to open and close the hand (or finger):
 * panoramic servo 	: axis 0
 * arm1				: axis 1
 * arm2				: axis 3
 * arm3				: axis 4
 * hand open		: button 0
 * hand close		: button 1
 * This program needs the serial port to which your grippybot (the arduino mega)
 * is connected, usually "/dev/ttyACM0".
 * example: ./grippybot_remote_control /dev/ttyACM0
 * @date: 2016.11.21
 * @author: David Venancio de Campos
 * @email: dv@3DLibre.com
 * @website: https://3DLibre.com (offline atm)
 * ****************************************************************************/
#include <stdio.h>    // Standard input/output definitions
#include <stdlib.h>
#include <string.h>   // String function definitions
#include <unistd.h>   // for usleep()
#include <SDL/SDL.h>
#include "arduino-serial-lib.h"
#include "config.h"

char szSerialPort[256];
int fd = -1;
int pan_angle = PAN_ANGLE_START,
	arm1_angle = ARM_1_ANGLE_START,
	arm2_angle = ARM_2_ANGLE_START,
	arm3_angle = ARM_3_ANGLE_START,
	fingers_angle = HAND_ANGLE_START,
	prev_pan_angle = PAN_ANGLE_START,
	prev_arm1_angle = ARM_1_ANGLE_START,
	prev_arm2_angle = ARM_2_ANGLE_START,
	prev_arm3_angle = ARM_3_ANGLE_START,
	prev_fingers_angle = HAND_ANGLE_START,
	pan_increment = 0,
	arm1_increment = 0,
	arm2_increment = 0,
	arm3_increment = 0,
	fingers_increment = 0;
char szBot_Command[32];

//main function
int main ( int argc, char** argv )
{
	if (argc<=1){
		printf("GrippyBot Remote Control ~ Version 0.01\r\n*********************************************************\r\n");
		printf("Usage: grippybot_remote_control /path/serial/port\r\n\r\n");
		printf("Controls:\r\n");
		printf("axis 0 = move panoramic\r\n");
		printf("axis 1 = move arm1\r\n");
		printf("axis 2 = move arm2\r\n");
		printf("axis 3 = move arm3\r\n");
		printf("button 0 = open hand\r\n");
		printf("button 1 = close hand\r\n\r\n");
		printf("Example: grippybot_remote_control /dev/ttyACM0\r\n");
		return 1;		
	}else{
		//lire les arguments
		sprintf(szSerialPort, "%s", argv[1]);
		printf("opening serial port: %s at %d\r\n", szSerialPort, ARDUINO_BAUDRATE);
		
		fd = serialport_init(szSerialPort, ARDUINO_BAUDRATE);
		if( fd==-1 ){
				printf("couldn't open port\n");
				return 1;
		}
        printf("opened port %s\n", szSerialPort);
        serialport_flush(fd);
	}
	
    SDL_Joystick *joy;
    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // Check for joystick
    if(SDL_NumJoysticks()>0){
      // Open joystick
      joy=SDL_JoystickOpen(0);

      if(joy){
        printf("Opened Joystick 0\n");
        printf("Name: %s\n", SDL_JoystickName(0));
        printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
        printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
        printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
      }else{
        printf("Couldn't open Joystick 0\n");
	  }
    }else{
        printf("No joystick detected ! T_T\n");
        return 1;
    }

    // create a new window
    SDL_Surface* screen = SDL_SetVideoMode(640, 250, 16, SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    // load an image
    SDL_Surface* bmp = SDL_LoadBMP("logo.bmp");
    if (!bmp){
        printf("Unable to load bitmap: %s\n", SDL_GetError());
        return 1;
    }

    // centre the bitmap on screen
    SDL_Rect dstrect;
    dstrect.x = (screen->w - bmp->w) / 2;
    dstrect.y = (screen->h - bmp->h) / 2;


    // program main loop
    bool done = false;
    while (!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
                {
                    // exit if ESCAPE is pressed
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        done = true;
                    if (event.key.keysym.sym == SDLK_s)
                        sprintf(szBot_Command, "go start\n");
                    break;
                }
            case SDL_JOYAXISMOTION: {
                    printf("Joystick motion : which=%u axis=%u, value=%d\n", 
						event.jaxis.which, event.jaxis.axis, event.jaxis.value);
								
					if(event.jaxis.which==0){
						switch(event.jaxis.axis){
							case 0: if(event.jaxis.value < -JOYSTICK_TRESHHOLD){
										pan_increment = SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value);
									}else if(event.jaxis.value > JOYSTICK_TRESHHOLD){
										pan_increment = (SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value));
									}else if(event.jaxis.value == 0){
										pan_increment = 0;
									}  
									break;
							case 1: if(event.jaxis.value < -JOYSTICK_TRESHHOLD){
										arm1_increment = (SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value));
									}else if(event.jaxis.value > JOYSTICK_TRESHHOLD){
										arm1_increment = SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value);
									}else if(event.jaxis.value == 0){
										arm1_increment = 0;
									}
									break;
							case 2: if(event.jaxis.value < -JOYSTICK_TRESHHOLD){
										arm3_increment = (SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value));	
									}else if(event.jaxis.value > JOYSTICK_TRESHHOLD){
										arm3_increment = SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value);
									}else if(event.jaxis.value == 0){
										arm3_increment = 0;
									}
									break;
							case 3: if(event.jaxis.value < -JOYSTICK_TRESHHOLD){
										arm2_increment = (SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value));
									}else if(event.jaxis.value > JOYSTICK_TRESHHOLD){
										arm2_increment = (SERVO_MOVE_INCREMENT / (JOY_MAX/event.jaxis.value));
									}else if(event.jaxis.value == 0){
										arm2_increment = 0;
									}
									break;
						}
					}
					
                    break;
                }

            case SDL_JOYBALLMOTION: {
                    //OnJoyBall(Event->jball.which,Event->jball.ball,Event->jball.xrel,Event->jball.yrel);
                    break;
                }

            case SDL_JOYHATMOTION: {
                    //OnJoyHat(Event->jhat.which,Event->jhat.hat,Event->jhat.value);
                    break;
                }
            case SDL_JOYBUTTONDOWN: {
                    printf("joystick button down = %d\n",event.jbutton.button);
					switch(event.jbutton.button){
						case 0: fingers_increment = 2; //open
								break;
						case 1: fingers_increment = -2; //close		(ask for movement,sent later)
								break;
						case 2: sprintf(szBot_Command, "go park\n"); //write directly a command to the buffer
								break;
						case 3: sprintf(szBot_Command, "wait\n");
								break;
						case 4: sprintf(szBot_Command, "get_distance\n");
								break;
						case 5: sprintf(szBot_Command, "laser\n");
								break;
						case 6: //sprintf(szBot_Command, "laser\n");
								break;
						case 7: //sprintf(szBot_Command, "laser\n");
								break;
						case 8: sprintf(szBot_Command, "play im\n"); //select
								break;
						case 9: sprintf(szBot_Command, "go start\n");
								pan_angle=PAN_ANGLE_START;		//init the angles
								arm1_angle=ARM_1_ANGLE_START;
								arm2_angle=ARM_2_ANGLE_START;
								arm3_angle=ARM_3_ANGLE_START;
								fingers_angle=HAND_ANGLE_START;
								break;
								
					}
                    break;
                }

            case SDL_JOYBUTTONUP: {
                    printf("joystick button up = %d\n",event.jbutton.button);
                    switch(event.jbutton.button){
						case 0: fingers_increment = 0;
								break;
								
						case 1: fingers_increment = 0;
								break;
								
					}
					break;
                }
            } // end switch
        } // end of message processing

		//input and movement management 

		//pan management
		//limit the movements to max and min angles, defined in the config.h file
		if((pan_angle > PAN_ANGLE_MIN || pan_increment>=0) && (pan_angle < PAN_ANGLE_MAX || pan_increment<=0))
			pan_angle = pan_angle + pan_increment;
			
		if(prev_pan_angle != pan_angle){
			prev_pan_angle = pan_angle;
			sprintf(szBot_Command, "pan %d\n", pan_angle);
			printf(">>> sending : %s", szBot_Command);
			serialport_write(fd, szBot_Command);
			sprintf(szBot_Command," ");
		}
		
		//arm1 management
		if((arm1_angle > ARM_1_ANGLE_MIN || arm1_increment>=0) && (arm1_angle < ARM_1_ANGLE_MAX || arm1_increment<=0))
			arm1_angle = arm1_angle + arm1_increment;
			
		if(prev_arm1_angle != arm1_angle){
			prev_arm1_angle = arm1_angle;
			sprintf(szBot_Command, "arm1 %d\n", arm1_angle);
			printf(">>> sending : %s", szBot_Command);
			serialport_write(fd, szBot_Command);
			sprintf(szBot_Command," ");
		}
		//arm2 management
		if((arm2_angle > ARM_2_ANGLE_MIN || arm2_increment>=0) && (arm2_angle < ARM_2_ANGLE_MAX || arm2_increment<=0))
			arm2_angle = arm2_angle + arm2_increment;
		if(prev_arm2_angle != arm2_angle){
			prev_arm2_angle = arm2_angle;
			sprintf(szBot_Command, "arm2 %d\n", arm2_angle);
			printf(">>> sending : %s", szBot_Command);
			serialport_write(fd, szBot_Command);
			sprintf(szBot_Command," ");
		}
		//arm3 management
		if((arm3_angle > ARM_3_ANGLE_MIN || arm3_increment>=0) && (arm3_angle < ARM_3_ANGLE_MAX || arm3_increment<=0))
			arm3_angle = arm3_angle + arm3_increment;
		if(prev_arm3_angle != arm3_angle){
			prev_arm3_angle = arm3_angle;
			sprintf(szBot_Command, "arm3 %d\n", arm3_angle);
			printf(">>> sending : %s", szBot_Command);
			serialport_write(fd, szBot_Command);
			sprintf(szBot_Command," ");
		}
		//hand management
		if((fingers_angle > HAND_ANGLE_MIN || fingers_increment>=0) && (fingers_angle < HAND_ANGLE_MAX || fingers_increment<=0))
			fingers_angle = fingers_angle + fingers_increment;
		if(prev_fingers_angle != fingers_angle){
			prev_fingers_angle = fingers_angle;
			sprintf(szBot_Command, "fingers %d\n", fingers_angle);
			printf(">>> sending : %s", szBot_Command);
			serialport_write(fd, szBot_Command);
			sprintf(szBot_Command," ");
		}
		// if the command buffer has a command, send it by the serial port
		if(szBot_Command[0]!=' '){
			printf(">>> sending : %s", szBot_Command);
			serialport_write(fd, szBot_Command);
			sprintf(szBot_Command," ");
		}
		
        // DRAWING STARTS HERE // to be completed (show joystick input)
		// clear screen
        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
		// draw bitmap
        SDL_BlitSurface(bmp, 0, screen, &dstrect);
        // DRAWING ENDS HERE
        // finally, update the screen :)
        SDL_Flip(screen);
    } // end main loop

    // free loaded bitmap
    SDL_FreeSurface(bmp);
	//close the joystick
    if(SDL_JoystickOpened(0))
        SDL_JoystickClose(joy);
    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}

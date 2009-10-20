#include "vrpn_Sound.h"
#include <string.h>
#include <math.h>
#include <conio.h>
#include <cstdlib>

#define FASTSPEED 500
#define CIRCLERADIUS 5.0F


vrpn_Sound_Client *soundClient;
float X,Y,Z,adj;
int numconnections;
int numSounds = 0;
vrpn_Connection connection;

void MoveListener()
{
	vrpn_float64 pos[3], ori[4];
	char kbchar;

	ori[0]=0;
	ori[1]=0;
	ori[2]=1;
	ori[3]=1;

	pos[0]=0;
	pos[1]=0;
	pos[2]=0;

	printf("		Move Listener\n");
	printf("*******************************\n");
	printf("             I\n");
	printf("           J   K\n");
	printf("             M\n");
	printf("*******************************\n");
	printf(" hit Q to return to main menu\n");

	while(!(_kbhit() && ((_getch()=='q') || (_getch()=='Q')))){

		kbchar = _getch();

		if((kbchar=='i') || (kbchar=='I')){
		
			pos[1] += .1;
			(void)soundClient->setListenerPose(pos,ori);
			soundClient->mainloop();
			connection.mainloop();
		}
		
		if((kbchar=='j') || (kbchar=='J')){
		
			pos[0] -= .1;
			(void)soundClient->setListenerPose(pos,ori);
			soundClient->mainloop();
			connection.mainloop();
		}
		
		if((kbchar=='k') || (kbchar=='K')){
		
			pos[0] += .1;
			(void)soundClient->setListenerPose(pos,ori);
			soundClient->mainloop();
			connection.mainloop();
		}
		
		if((kbchar=='m') || (kbchar=='M')){
		
			pos[1] -= .1;
			(void)soundClient->setListenerPose(pos,ori);
			soundClient->mainloop();
			connection.mainloop();
		}
		
		if((kbchar=='q') || (kbchar=='Q')){
			break;
		}
		
		kbchar = ' ';
	}
}


static void init_sample_values()
{
	Y=Z=0.0F;
	X=5.0F;
	adj=0.0F;
}


static void move_sample_values()
{
	// handle the circle movement
	adj+=(2.0F*3.14159265358979F/(((FASTSPEED+1-50.0F)*1.6F)+20.0F));
	X=(float)(CIRCLERADIUS*cos(adj));
	Y=(float)(CIRCLERADIUS*sin(adj));
}


void loopSound(vrpn_SoundID id)
{
	float sA = 0;
	vrpn_float64 position[3], orientation[4];
	
	while(1)
	{
		
		//save old settings
		position[0] = X; position[1] = Y; position[2] = Z;
		orientation[0] = -X; orientation[1] = -Y; orientation[2] = Z; orientation[3] = 1;
		(void)soundClient->setSoundPose(id,position, orientation);
		
		printf(" %f %f %f \r",position[0], position[1], position[2]);

		// move the sample values
		move_sample_values();
		
		/*
		
		  // calculate the delta vector
		  velocity[0]=X-sX;
		  velocity[1]=0;
		  velocity[2]=Z-sZ;
		  velocity[3]=((50.0F/300.0F)+1.0F)/1500.0F;
		  
			// restore the values to original
			//X=sX;
			//Z=sZ;
			//adj=sA;
		*/
		//	 (void)soundClient->setSoundVelocity(id,velocity);
		soundClient->mainloop();
		if (X == 25.0F)
			break;
		vrpn_SleepMsecs(1);
	}
}

void shutdown(){
	
	printf("\nSound Client shutting down.\n");
	soundClient = NULL;
}

int getOnlyDigits(){

	char temp[80];
	int id;
	fgets(temp, sizeof(temp)-1, stdin);
	for(int x=0; x<80; x++)
		if(isalpha(temp[x])) return -1;
	id=strtol(temp,0,10);
	return id;
}





int main(int argc, char** argv)
{
	char dummy[80];
	vrpn_SoundID ids[100], id;
	char files[100][80];
	int curID = 0;
	int command;
	int loop = 1, i;
	vrpn_int32 repeat, volume;
	vrpn_float64 position[3], orientation[4], velocity[4];
	vrpn_float64 pos[3], ori[4], Lvelocity[4];
	numconnections = 0;
	int flag = 0;
	
	position[0] = 0; position[1] = 0; position[2] = 0;
	orientation[0] = 0; orientation[1] = 0; orientation[2] = -1; orientation[3] = 1;
	velocity[0] = 0; velocity[1] = 0; velocity[2] = 0; velocity[3] = 0;
	
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = 0;
	ori[0] = 0;
	ori[1] = 0;
	ori[2] = 0;
	ori[3] = 1;
/*	
	printf("Please enter the server you wish to connect to.\n");
	scanf("%s", server);
	printf("Please enter the sound device name you wish to connect to.\n");
	scanf("%s", device);
*/	
	vrpn_Connection connection("localhost");
	soundClient = new vrpn_Sound_Client("sound", &connection);
	
	for(i = 0; i < 100; i++)
		ids[i] = -1;
	

	/*******************************************
	*
	*	Client mainloop
	*
	*******************************************/
	while(loop)
	{
		vrpn_SleepMsecs(1);
		
		soundClient->mainloop();
		connection.mainloop();
		
		if (numconnections==0 && connection.connected())
			numconnections++;
		
		if (((numconnections!=0) && (!connection.connected())) || !connection.doing_okay())  {
			shutdown();	
			numconnections=0;
			break;
		}
		
		printf("\nCurrent sounds loaded***************\n");
		for(i = 0; i < numSounds; i++)
			printf("Sound# %d: %s\n", ids[i], files[i]);
		printf("************************************\n");

		printf("\nOptions\n");
		printf("1) Load Sound\n");
		printf("2) Unload Sound\n");
		printf("3) Play Sound\n");
		printf("4) Stop Sound\n");
		printf("5) Change Sound Volume\n");
		printf("6) Change Sound Position\n");
		printf("7) Change Sound Orientation\n");
		printf("8) Change Sound Velocity\n");
		printf("9) Change Listener Position\n");
		printf("10) Change Listener Orientation\n");
		printf("11) Change Listener Velocity\n");
		printf("12) Loop sound around head\n");
		printf("13) Move Listener\n");
		printf("14) Quit\n");
		printf("Choose option ");
		
		
		do{
			command = getOnlyDigits();
		}while(command == -1);	


	
		switch(command)
		{
		case 1:
			printf("Enter path and file to load\n");
			scanf("%s", dummy);
			vrpn_SoundDef SoundDef;
			
			// 1 meter in front of listener
			SoundDef.pose.position[0] = 0;
			SoundDef.pose.position[1] = 0;
			SoundDef.pose.position[2] = 0;

			// Looking back at listener
			SoundDef.pose.orientation[0] = 0;
			SoundDef.pose.orientation[1] = 0;
			SoundDef.pose.orientation[2] = 0;
			SoundDef.pose.orientation[3] = 1;

			// DirectX default for both is 360
			SoundDef.cone_inner_angle = 360;
			SoundDef.cone_outer_angle = 360;

			// No attenuation
			SoundDef.cone_gain = -60;

			SoundDef.min_front_dist = .1;
			SoundDef.max_front_dist = 50;

			SoundDef.velocity[0] = 0;
			SoundDef.velocity[1] = 0;
			SoundDef.velocity[2] = 0;

			ids[curID] = soundClient->loadSound(dummy, curID, SoundDef);
			strcpy(files[curID++], dummy);
			numSounds++;
			soundClient->mainloop();
			break;
		case 2:
			do{
				printf("Enter id of sound to unload:");
				id = getOnlyDigits();
			}while(id == -1);	

			
			(void)soundClient->unloadSound(id);
			for(i = id; i < numSounds; i++){
					strcpy(files[i], files[i+1]);
			}
			numSounds--;
			curID--;
			soundClient->mainloop();
			break;
		case 3:
			do{
				printf("Enter id of sound to play:");
				id = getOnlyDigits();
			}while(id == -1);	

			do{
				printf("Enter number of times to repeat.  (0 = continuous) ");
				repeat = getOnlyDigits();
			}while(repeat == -1);	

			(void)soundClient->playSound(id, repeat);
			soundClient->mainloop();
			break;
		case 4:
			do{
				printf("Enter ID of sound to stop ");
				id = getOnlyDigits();
			}while(id == -1);	
		
			(void)soundClient->stopSound(id);
			soundClient->mainloop();
			break;
		case 5:
			do{
				printf("Enter ID of sound to change ");
				id = getOnlyDigits();
			}while(id == -1);	
			
			do{
				printf("Enter value to change volume to ");
				volume = getOnlyDigits();
			}while(volume == -1);	

			(void)soundClient->setSoundVolume(id, volume);
			soundClient->mainloop();
			break;
		case 6:
			do{
				printf("Enter ID of sound to change ");
				id = getOnlyDigits();
			}while(id == -1);	

			printf("Enter the new X,Y, and Z position coordinates for the sound\n");
			scanf("%lf %lf %lf", &position[0], &position[1], &position[2]);
			(void)soundClient->setSoundPose(id, position, orientation);
			soundClient->mainloop();
			break;
		case 7:
			do{
				printf("Enter ID of sound to change ");
				id = getOnlyDigits();
			}while(id == -1);	

			printf("Enter the new X,Y, Z, and W orientation coordinates for the sound\n");
			scanf("%lf %lf %lf %lf", &orientation[0], &orientation[1], &orientation[2], &orientation[3]);
			(void)soundClient->setSoundPose(id, position, orientation);
			soundClient->mainloop();
			break;
		case 8:
			do{
				printf("Enter ID of sound to change ");
				id = getOnlyDigits();
			}while(id == -1);	

			printf("Enter the new X,Y, and Z velocity coordinates for the sound and magnitude\n");
			scanf("%lf %lf %lf %lf", &velocity[0], &velocity[1], &velocity[2], &velocity[3]);
			(void)soundClient->setSoundVelocity(id,velocity);
			soundClient->mainloop();
			break;
		case 9:
			printf("Enter the new X,Y, and Z position coordinates for the listener\n");
			scanf("%lf %lf %lf", &pos[0], &pos[1], &pos[2]);
			(void)soundClient->setListenerPose(pos,ori);
			soundClient->mainloop();
			break;
		case 10:
			printf("Enter the new orientation quaternion for the listener\n");
			scanf("%lf %lf %lf %lf", &ori[0], &ori[1], &ori[2], &ori[3]);
		
			(void)soundClient->setListenerPose(pos, ori);
			soundClient->mainloop();
			break;
		case 11:
			printf("Enter the new X,Y, and Z velocity coordinates for the listener and magnitude\n");
			scanf("%lf %lf %lf %lf", &Lvelocity[0], &Lvelocity[1], &Lvelocity[2], &Lvelocity[3]);
			(void)soundClient->setListenerVelocity(Lvelocity);
			soundClient->mainloop();
			break;
		case 12:
			do{
				printf("Enter ID of sound to loop");
				id = getOnlyDigits();
			}while(id == -1);
			
			init_sample_values();
			loopSound(id);
			break;
		case 13:
			MoveListener();
			break;
		case 14:
			loop = 0;
			shutdown();
			break;
		default:
			break;
		}		
	}
	
	return 0;
}

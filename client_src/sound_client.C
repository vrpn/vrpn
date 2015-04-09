#include "vrpn_Sound.h"
#include <string.h>
#include <math.h>

#define FASTSPEED 100
#define CIRCLERADIUS 25.0F

vrpn_Sound_Client *soundClient;
float X,Y,Z,adj;



static void init_sample_values()
{
  Y=Z=0.0F;

  X=25.0F;
  adj=0.0F;
}

static void move_sample_values()
{
  // handle the circle movement
  adj+=(2.0F*3.14159265358979F/(((FASTSPEED+1-50.0F)*1.6F)+20.0F));
  X=(float)(CIRCLERADIUS*cos(adj));
  Z=(float)(CIRCLERADIUS*sin(adj));
}

void loopSound(vrpn_SoundID id)
{
  vrpn_float64 position[3], orientation[4], velocity[4];

  while(1)
  {
	  //save old settings
	  position[0] = X; position[1] = Y; position[2] = Z;
	  orientation[0] = -X; orientation[1] = -Y; orientation[2] = -Z; orientation[3] = 1;
	  (void)soundClient->setSoundPose(id, position, orientation);

      // move the sample values
	  move_sample_values();

	  // calculate the delta vector
	 velocity[0]=X-position[0];
     velocity[1]=0-position[1];
     velocity[2]=Z-position[2];
	 velocity[3]=((50.0F/300.0F)+1.0F)/1500.0F;

	 (void)soundClient->setSoundVelocity(id,velocity);
	 soundClient->mainloop();
     if (X == 25.0F)
   	   break;
	 vrpn_SleepMsecs(1);
  }
}

int main(int, char**)
{
	char server[80], device[80], dummy[80];
	vrpn_SoundID ids[100], id;
	char files[100][80];
	int curID = 0;
	int command;
	int loop = 1, i;
	vrpn_int32 repeat, volume;
	vrpn_float64 position[3], orientation[4], velocity[4];
	vrpn_float64 Lposition[3], Lorientation[4], Lvelocity[4];

	position[0] = 0; position[1] = 0; position[2] = 0;
	orientation[0] = 0; orientation[1] = 0; orientation[2] = 1; orientation[3] = 0;
	velocity[0] = 0; velocity[1] = 0; velocity[2] = 0; velocity[3] = 0;

	Lposition[0] = 0; Lposition[1] = 0; Lposition[2] = 0;
	Lorientation[0] = 0; Lorientation[1] = 0; Lorientation[2] = -1; Lorientation[3] = 1;
	Lvelocity[0] = 0; Lvelocity[1] = 0; Lvelocity[2] = 0; Lvelocity[3] = 0;

	printf("Please enter the server you wish to connect to.\n");
	scanf("%s", server);
	printf("Please enter the sound device name you wish to connect to.\n");
	scanf("%s", device);
	
	vrpn_Connection *connection = vrpn_get_connection_by_name(server);
	soundClient = new vrpn_Sound_Client(device, connection);

	for(i = 0; i < 100; i++)
		ids[i] = -1;

	while(loop)
	{
		printf("Current sounds loaded\n");
		for(i = 0; i < 100; i++)
		{
			if (ids[i] == -1)
				continue;

			if (i % 3 == 2)
				printf("\n");
			else
				printf("\t");
			printf("%d %s", ids[i], files[i]);
		}
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
		printf("13) Quit\n");
		printf("Choose option ");
		scanf("%d", &command);

		switch(command)
		{
		case 1: {
				printf("Enter path and file to load\n");
				scanf("%s", dummy);
				vrpn_SoundDef SoundDef;
				ids[curID] = soundClient->loadSound(dummy, curID, SoundDef);
				strcpy(files[curID++], dummy);
				soundClient->mainloop();
			}
			break;
		case 2:
			printf("Enter ID of sound to unload ");
			scanf("%d", &id);
			(void)soundClient->unloadSound(id);
			for(i = 0; i < 100; i++)
				if (ids[i] == id) ids[i] = -1;
			soundClient->mainloop();
			break;
		case 3:
			printf("Enter ID of sound to play ");
			scanf("%d", &id);
			printf("Enter number of times to repeat.  (0 = continuous) ");
			scanf("%d", &repeat);
			(void)soundClient->playSound(id, repeat);
			soundClient->mainloop();
			break;
		case 4:
			printf("Enter ID of sound to stop ");
			scanf("%d", &id);
			(void)soundClient->stopSound(id);
			soundClient->mainloop();
			break;
		case 5:
			printf("Enter ID of sound to change ");
			scanf("%d", &id);
			printf("Enter value to change volume to ");
			scanf("%d", &volume);
			(void)soundClient->setSoundVolume(id, volume);
			soundClient->mainloop();
			break;
		case 6:
			printf("Enter ID of sound to change ");
			scanf("%d", &id);
			printf("Enter the new X,Y, and Z position coordinates for the sound\n");
			scanf("%lf %lf %lf", &position[0], &position[1], &position[2]);
			(void)soundClient->setSoundPose(id, position, orientation);
			soundClient->mainloop();
			break;
		case 7:
			printf("Enter ID of sound to change ");
			scanf("%d", &id);
			printf("Enter the new X,Y, Z, and W orientation coordinates for the sound\n");
			scanf("%lf %lf %lf %lf", &orientation[0], &orientation[1], &orientation[2], &orientation[3]);
			(void)soundClient->setSoundPose(id, position, orientation);
			soundClient->mainloop();
			break;
		case 8:
			printf("Enter ID of sound to change ");
			scanf("%d", &id);
			printf("Enter the new X,Y, and Z velocity coordinates for the sound and magnitude\n");
			scanf("%lf %lf %lf %lf", &velocity[0], &velocity[1], &velocity[2], &velocity[3]);
			(void)soundClient->setSoundVelocity(id,velocity);
			soundClient->mainloop();
			break;
		case 9:
			printf("Enter the new X,Y, and Z position coordinates for the listener\n");
			scanf("%lf %lf %lf", &Lposition[0], &Lposition[1], &Lposition[2]);
			(void)soundClient->setListenerPose(Lposition, Lorientation);
			soundClient->mainloop();
			break;
		case 10:
			printf("Enter the new X,Y, Z, and W orientation coordinates for the listener\n");
			scanf("%lf %lf %lf %lf", &Lorientation[0], &Lorientation[1], &Lorientation[2], &Lorientation[3]);
			(void)soundClient->setListenerPose(Lposition, Lorientation);
			soundClient->mainloop();
			break;
		case 11:
			printf("Enter the new X,Y, and Z velocity coordinates for the listener and magnitude\n");
			scanf("%lf %lf %lf %lf", &Lvelocity[0], &Lvelocity[1], &Lvelocity[2], &Lvelocity[3]);
			(void)soundClient->setListenerVelocity(Lvelocity);
			soundClient->mainloop();
			break;
		case 12:
			printf("Enter ID of sound to loop");
			scanf("%d", &id);
			init_sample_values();
			loopSound(id);
			break;
		case 13:
			loop = 0;
			break;
		default:
			break;
		}


	}
	

	return 0;
}

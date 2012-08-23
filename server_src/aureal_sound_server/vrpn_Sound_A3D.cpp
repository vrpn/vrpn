#include "vrpn_Sound_A3D.h"

#define NUM_SPIN 20000
const int CHAR_BUF_SIZE =1024;


vrpn_Sound_Server_A3D::vrpn_Sound_Server_A3D(const char      * name, 
											 vrpn_Connection * c, 
											 HWND              hWin) 
                       : vrpn_Sound_Server(name, c){

  try {

	// initialize the sound goodies
  	a3droot     = NULL;
    a3dgeom     = NULL;
    a3dlis      = NULL;

    lasterror = A3dCreate(NULL, (void**)&a3droot, NULL, A3D_1ST_REFLECTIONS | A3D_OCCLUSIONS | A3D_DISABLE_SPLASHSCREEN);
		if(FAILED(lasterror))
		{
			throw "Failed to retrieve the A3D COM interface";
		}
    		
    // need to set cooperative level
 		lasterror = a3droot->SetCooperativeLevel(hWin, A3D_CL_EXCLUSIVE);
		if(FAILED(lasterror))
		{
			throw "Failed to Set the Cooperative Level";
		}

	  lasterror = a3droot->QueryInterface(IID_IA3dListener, (void **)&a3dlis);

    if (FAILED(lasterror)) {
		  throw "Couldnt get Listener in constructor\n";
    }

	  lasterror = a3droot->QueryInterface(IID_IA3dGeom, (void **)&a3dgeom);
	  
    if (FAILED(lasterror)) {
		  throw "Couldnt get Geometry in constructor";
    }

    // check on status of requested features:

    if (a3droot->IsFeatureAvailable(A3D_1ST_REFLECTIONS)) 
      printf("Reflections available\n");
    else printf("Reflections NOT available\n");
    
    if (a3droot->IsFeatureAvailable(A3D_OCCLUSIONS)) 
      printf("Occlusions available\n");
    else printf("Occlusions NOT available\n");

	  // set some global default values
	  // default DistanceModelScale is 1.0 which means gain is reducded 6dB for doubling in distance from listener
	  // default DopplerScale is 1.0 (340m/s)
	  // default Equalization is 1.0
	  // default MaxReflectionDelayTime is .3 seconds which, according to
	  //   the A3D document, is adequate for models smaller than a football stadium
	  // default NumFallBackSources is 12 (more than 12 sources and we go to software)
	  // default RMPriorityBias is .5 means we equally weigh audibility and priority when deciding whether to play source
	  // default UnitsPerMeter is 1.0 (we use meters)

	  // default coordinate system is RIGHT HANDED

	  lasterror = a3droot->SetCoordinateSystem(A3D_RIGHT_HANDED_CS);

	  lasterror = a3droot->SetOutputGain(1.0);  // max gain
	
	  // default to using headphones
	  lasterror = a3droot->SetOutputMode(OUTPUT_HEADPHONES,OUTPUT_HEADPHONES,OUTPUT_MODE_STEREO);

	  // set up our playlist
	  maxSounds = 15;
	  numSounds = -1;
    numMaterials = 0;
    maxMaterials = 0;
	
    a3dsamples = (IA3dSource2**) new IA3dSource2* [maxSounds];

	  // initialize playlist
	  for (int i(0); i < maxSounds; i++) {
	  	a3dsamples[i] = NULL;
      soundMap[i]=-1;
    }

	  a3droot->Clear();
	}

	catch(char* szError) {
		// Display the message.
		printf("AudioInit() - %s.\n", szError);
    send_message("Error initializing sound card.. sound server did not start.",vrpn_TEXT_ERROR,0);
		exit(0);
	}

}
					
vrpn_Sound_Server_A3D::~vrpn_Sound_Server_A3D() {

	printf("Before freeing... \n");
	//release the interfaces..
  
	a3droot->Shutdown();
	// free up COM
	CoUninitialize();
	printf("After free... \n");
}

	
void vrpn_Sound_Server_A3D::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef) {
    
	vrpn_int32 myid = soundMap[id];
	if (myid != -1) {
	  if (repeat == 0)
	    lasterror = a3dsamples[myid]->Play(A3D_LOOPED);
	  else 
		lasterror = a3dsamples[myid]->Play(A3D_SINGLE);
	  printf("Playing sound %d\n", myid);
    send_message("Playing sound",vrpn_TEXT_NORMAL,0);
	}
  else send_message("Error: playSound (Sound not loaded)",vrpn_TEXT_WARNING,0);
}

void vrpn_Sound_Server_A3D::stopSound(vrpn_SoundID id) {

	vrpn_int32 myid = soundMap[id];
  if (myid!=-1) {
	  printf("Stopping sound %d\n",myid);
	  lasterror = a3dsamples[myid]->Stop();
    send_message("Stopping sound",vrpn_TEXT_NORMAL,0);
  } else send_message("Invalid sound id",vrpn_TEXT_ERROR,0);
}

void vrpn_Sound_Server_A3D::loadSoundLocal(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef) {
  
  char tempbuf[1024];

  try {
   numSounds++;
   if (numSounds == maxSounds) {
		maxSounds = 2 * maxSounds;
		IA3dSource2 ** temp = new (IA3dSource2*);

		for (int i(0); i < maxSounds; i++) {
			if (i <= numSounds)
			  temp[i] = a3dsamples[i];
			else temp[i] = NULL;
		}
		delete [] a3dsamples;
		a3dsamples = temp;
	}
  filename = strlwr(filename);  // string to lower case

  lasterror = a3droot->NewSource( A3DSOURCE_TYPEDEFAULT | A3DSOURCE_INITIAL_RENDERMODE_A3D , &a3dsamples[numSounds]);

  if (FAILED(lasterror)) {
    throw "Error making new source";
  }

  if (strstr(filename, "mp3")==NULL) // assume WAVE format
     lasterror = a3dsamples[numSounds]->LoadFile(filename, A3DSOURCE_FORMAT_WAVE);
  else  // otherwise we assume mp3
     lasterror = a3dsamples[numSounds]->LoadFile(filename, A3DSOURCE_FORMAT_MP3); 
  
  if (FAILED(lasterror)) {
    throw "Error loading file";
  }

  // initialize sound params

  lasterror=a3dsamples[numSounds]->SetCone(0,0,1);
  // these values are defaults:
  // default ModelScale of 6dB for each doubling of distance
  // default DopplerScale of 1.0
  // default Equalization of 1.0
  // default pitch is 1.0 (unaltered pitch)
  // default priority of .5 on a scale from 0 to 1
  // default ReflectionDelayScale is 1.0 (normal delay in reflections [.3 seconds])
  // default RefelectionGainScale is 1.0 
  // default RenderMode uses occlusion, reflection, a3d (??)
  // default TransformMode is relative to origin
   
  lasterror=a3dsamples[numSounds]->SetMinMaxDistance((float) soundDef.min_front_dist, (float)soundDef.max_front_dist, A3D_MUTE);
  lasterror=a3dsamples[numSounds]->SetGain(soundDef.volume);

  // default position is at the origin
  lasterror=a3dsamples[numSounds]->SetPosition3f((float)soundDef.pose.position[0],(float)soundDef.pose.position[1],(float)soundDef.pose.position[2]);
   
qogl_matrix_type eye_f_world_mat;
  qogl_matrix_type eye_axis_vectors_in_world;

  q_to_ogl_matrix (eye_f_world_mat, soundDef.pose.orientation);
  qogl_matrix_type eye_axis_vectors=// we put the up and forward
	// as the first two columns in the matrix.
	//NOTE: these columns in the matrix
	//appear as rows here since the number starts in the upper left corner
	//as goes down!
  { 0,0,-1,1, //the forward normal vector in eye space
	  0,1,0,1, //the up normal vector in eye space
	  0,0,0,0, //unused
	  0,0,0,0 } ; //unused
      
  qogl_matrix_mult(eye_axis_vectors_in_world,eye_axis_vectors,eye_f_world_mat);
        
    
  // takes front vector then up vector
  lasterror = a3dlis->SetOrientation6f((float)eye_axis_vectors_in_world[0], (float)eye_axis_vectors_in_world[1],
                         	             (float)eye_axis_vectors_in_world[2], (float)eye_axis_vectors_in_world[4], (float)eye_axis_vectors_in_world[5],
	                     			           (float)eye_axis_vectors_in_world[6]);    

  soundMap[id] = numSounds;
  }
  catch(char* szError)
	{
		// Display the message.
   
    sprintf(tempbuf,"Error: loadSoundLocal (%s) [file: %s]",szError,filename);
		printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
	return;
	}

    sprintf(tempbuf,"Loading sound #%d: %s \n",numSounds,filename);
		printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
    return;

}

// not supported
void vrpn_Sound_Server_A3D::loadSoundRemote(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef) {

  send_message("loadSoundRemote not supported",vrpn_TEXT_WARNING,0);
}

void vrpn_Sound_Server_A3D::unloadSound(vrpn_SoundID id) {

  vrpn_int32 myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: unloadSound(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  a3dsamples[myid]->Release();

  for (int i(myid); i < numSounds; i++) {
	a3dsamples[i] = a3dsamples[i+1];
  }

  sprintf(tempbuf,"Unloading sound: %d ",id);
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
}

void vrpn_Sound_Server_A3D::setListenerPose(vrpn_PoseDef pose) {
	 
qogl_matrix_type eye_f_world_mat;
  qogl_matrix_type eye_axis_vectors_in_world;

  q_to_ogl_matrix (eye_f_world_mat, pose.orientation);
  qogl_matrix_type eye_axis_vectors=// we put the up and forward
	// as the first two columns in the matrix.
	//NOTE: these columns in the matrix
	//appear as rows here since the number starts in the upper left corner
	//as goes down!
  { 0,0,-1,1, //the forward normal vector in eye space
	  0,1,0,1, //the up normal vector in eye space
	  0,0,0,0, //unused
	  0,0,0,0 } ; //unused
      
  qogl_matrix_mult(eye_axis_vectors_in_world,eye_axis_vectors,eye_f_world_mat);
        
    
  // takes front vector then up vector
  lasterror = a3dlis->SetOrientation6f((float)eye_axis_vectors_in_world[0], (float)eye_axis_vectors_in_world[1],
                         	             (float)eye_axis_vectors_in_world[2], (float)eye_axis_vectors_in_world[4], (float)eye_axis_vectors_in_world[5],
	                     			           (float)eye_axis_vectors_in_world[6]);

  lasterror = a3dlis->SetPosition3f((float)pose.position[0],(float)pose.position[1],(float)pose.position[2]);

  // dont send back a message here because there are a bunch of them
}

void vrpn_Sound_Server_A3D::setListenerVelocity(vrpn_float64 velocity[4]) {
  q_vec_type velocity_vec;

  q_to_vec(velocity_vec, velocity);

  lasterror = a3dlis->SetVelocity3fv((A3DVAL*) velocity_vec);

  char tempbuf[1024];
  sprintf(tempbuf,"Setting listener velocity ");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}

void vrpn_Sound_Server_A3D::changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundStatus(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }

  lasterror = a3dsamples[id]->SetPosition3f((float)soundDef.pose.position[0],(float)soundDef.pose.position[1],(float)soundDef.pose.position[2]);

  qogl_matrix_type eye_f_world_mat;
  qogl_matrix_type eye_axis_vectors_in_world;

  q_to_ogl_matrix (eye_f_world_mat, soundDef.pose.orientation);
  qogl_matrix_type eye_axis_vectors=// we put the up and forward
	// as the first two columns in the matrix.
	//NOTE: these columns in the matrix
	//appear as rows here since the number starts in the upper left corner
	//as goes down!
  { 0,0,-1,1, //the forward normal vector in eye space
	  0,1,0,1, //the up normal vector in eye space
	  0,0,0,0, //unused
	  0,0,0,0 } ; //unused
      
  qogl_matrix_mult(eye_axis_vectors_in_world,eye_axis_vectors,eye_f_world_mat);
        
    
  // takes front vector then up vector
  lasterror = a3dsamples[id]->SetOrientation6f((float)eye_axis_vectors_in_world[0], (float)eye_axis_vectors_in_world[1],
                         	             (float)eye_axis_vectors_in_world[2], (float)eye_axis_vectors_in_world[4], (float)eye_axis_vectors_in_world[5],
	                     			           (float)eye_axis_vectors_in_world[6]);
  q_vec_type velocity_vec;

  q_to_vec(velocity_vec, soundDef.velocity);
  lasterror = a3dsamples[myid]->SetVelocity3fv((A3DVAL*) velocity_vec);
  
  // we only use front_min and front_back
  lasterror = a3dsamples[myid]->SetMinMaxDistance((float)soundDef.min_front_dist, (float)soundDef.max_front_dist, A3D_AUDIBLE);
  lasterror = a3dsamples[myid]->SetCone((float)soundDef.cone_inner_angle, (float)soundDef.cone_outer_angle, (float)soundDef.cone_gain);
  lasterror = a3dsamples[myid]->SetDopplerScale((float)soundDef.dopler_scale);
  lasterror = a3dsamples[myid]->SetEq((float)soundDef.equalization_val);
  lasterror = a3dsamples[myid]->SetPitch((float)soundDef.pitch);
  lasterror = a3dsamples[myid]->SetGain((float)soundDef.volume);

  sprintf(tempbuf,"Setting sound definition ");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

  return;
}
						
void vrpn_Sound_Server_A3D::setSoundPose(vrpn_SoundID id, vrpn_PoseDef pose) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundPose(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
    
  lasterror = a3dsamples[id]->SetPosition3f((float)pose.position[0],(float)pose.position[1],(float)pose.position[2]);

  qogl_matrix_type eye_f_world_mat;
  qogl_matrix_type eye_axis_vectors_in_world;

  q_to_ogl_matrix (eye_f_world_mat, pose.orientation);
  qogl_matrix_type eye_axis_vectors=// we put the up and forward
	// as the first two columns in the matrix.
	//NOTE: these columns in the matrix
	//appear as rows here since the number starts in the upper left corner
	//as goes down!
  { 0,0,-1,1, //the forward normal vector in eye space
	  0,1,0,1, //the up normal vector in eye space
	  0,0,0,0, //unused
	  0,0,0,0 } ; //unused
      
  qogl_matrix_mult(eye_axis_vectors_in_world,eye_axis_vectors,eye_f_world_mat);
        
    
  // takes front vector then up vector
  lasterror = a3dsamples[id]->SetOrientation6f((float)eye_axis_vectors_in_world[0], (float)eye_axis_vectors_in_world[1],
                         	             (float)eye_axis_vectors_in_world[2], (float)eye_axis_vectors_in_world[4], (float)eye_axis_vectors_in_world[5],
	                     			           (float)eye_axis_vectors_in_world[6]);  
  sprintf(tempbuf,"Setting sound pose ");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
}

void vrpn_Sound_Server_A3D::setSoundVelocity(vrpn_SoundID id, vrpn_float64 *velocity) {
  q_vec_type velocity_vec;
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundVelocity(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }

  q_to_vec(velocity_vec, velocity);

  lasterror = a3dsamples[myid]->SetVelocity3fv((A3DVAL*) velocity_vec);
  
  sprintf(tempbuf,"Setting sound velocity ");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}

void vrpn_Sound_Server_A3D::setSoundDistInfo(vrpn_SoundID id, vrpn_float64 *distinfo) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundDistInfo(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  
  lasterror = a3dsamples[myid]->SetMinMaxDistance((float)distinfo[2], (float)distinfo[3], A3D_AUDIBLE);
  
  sprintf(tempbuf,"Setting distance information for sound %d to min: %f; max: %f",id,distinfo[2],distinfo[3]);
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}
void vrpn_Sound_Server_A3D::setSoundConeInfo(vrpn_SoundID id, vrpn_float64 *coneinfo) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundConeInfo(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  coneinfo[0] = coneinfo[0] * 180.0/Q_PI;
  coneinfo[1] = coneinfo[1] * 180.0/Q_PI;

  lasterror = a3dsamples[myid]->SetCone((float)coneinfo[0], (float)coneinfo[1], (float)coneinfo[2]);
  
  sprintf(tempbuf,"Setting sound cone information for sound %d to inner: %f; outer: %f; gain: %f",coneinfo[0], coneinfo[1], coneinfo[2]);
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}


void vrpn_Sound_Server_A3D::setSoundDoplerFactor(vrpn_SoundID id, vrpn_float64 doplerfactor) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundDoplerFactor(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  
  lasterror = a3dsamples[myid]->SetDopplerScale((float)doplerfactor);
  
  sprintf(tempbuf,"Setting sound dopler scale");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}
void vrpn_Sound_Server_A3D::setSoundEqValue(vrpn_SoundID id, vrpn_float64 eqvalue) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundEqValue(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  
  lasterror = a3dsamples[myid]->SetEq((float)eqvalue);
  
  sprintf(tempbuf,"Setting sound equalization value");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}
void vrpn_Sound_Server_A3D::setSoundPitch(vrpn_SoundID id, vrpn_float64 pitch) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundPitch(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  
  lasterror = a3dsamples[myid]->SetPitch((float)pitch);
  sprintf(tempbuf,"Setting sound pitch");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}
void vrpn_Sound_Server_A3D::setSoundVolume(vrpn_SoundID id, vrpn_float64 volume) {
  vrpn_int32     myid = soundMap[id];
  char tempbuf[1024];
   
  if (myid==-1) {
    sprintf(tempbuf,"Error: setSoundVolume(Invalid id)");
    printf("%s\n", tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
    return;
  }
  lasterror = a3dsamples[myid]->SetGain((float)volume);
    
  sprintf(tempbuf,"Setting sound volume");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  
  return;
}

void vrpn_Sound_Server_A3D::loadModelLocal(const char * filename) {
  FILE  * model_file;
  float   x,y,z;
  int     num = 0;
  char    tempbuf[1024], tempbuf2[1024];
  
  int       valid_num = 0;
  int       mynum = 0;
  int       curnum = 0;
  char      material_name[MAX_MATERIAL_NAME_LENGTH];
  
  float     trans1, trans2, refl1, refl2;
  float		  subopeningval;
  // open the file
		
  try
  {
    sprintf(tempbuf, "Working from model file: %s", filename);
    printf("%s\n",tempbuf);
    send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
   
    if ( (model_file = fopen(filename, "r")) == NULL) {
      throw "Cannot open model file";
    }
  
  char	line[512];	// Line read from the input file
  char  * pch;
	char    scrap[512];
	
	// Read lines from the file until we run out
	while ( fgets(line, sizeof(line), model_file) != NULL ) 
		{
			
			// Make sure the line wasn't too long
    if (strlen(line) >= sizeof(line)-1) {
        sprintf(tempbuf, "Line too long in config file: %s",line);
				throw tempbuf;
			}
			
			if ((strlen(line)<3)||(line[0]=='#')) {
				continue;
			}
			
			// copy for strtok work
			strncpy(scrap, line, sizeof(line) - 1);
			
#define isit(s) !strcmp(pch=strtok(scrap," \t"),s)
#define next() pch += strlen(pch) + 1
			
			// for now just have 10 materials
			
			if(isit("MATERIAL"))
			{	
			// material name should be first followed by trans and reflect values
				next();
				
				if (sscanf(pch,"%511s\t%f\t%f\t%f\t%f\n",material_name, &trans1, &trans2, &refl1, &refl2) != 5) {
					sprintf(tempbuf, "PROBLEM in material list with material: %s", material_name);
          throw tempbuf;
				}
				else {
					// make sure name is not already in the list
					for (int i(0); i < MAX_NUMBER_MATERIALS; i++) {
						if (strcmp(material_name, mat_names[i]) == 0)
						{
							sprintf(tempbuf, "Material %s already exists in the list.. replacing with new definition", material_name);
              send_message((const char *) tempbuf,vrpn_TEXT_WARNING,0);
						}
					}

					// copy the name to the name array
					strcpy(mat_names[curnum], material_name);
					a3dgeom->NewMaterial(&materials[curnum]);
					materials[curnum]->SetTransmittance(trans1, trans2);
					materials[curnum]->SetReflectance(refl1, refl2);
					curnum++;
          sprintf(tempbuf, "Material %d is %s", curnum-1, mat_names[curnum-1]);
          send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
					valid_num = curnum;
				}
			}
	
			else if(isit("A3D_QUAD"))
			{

				next();

				if (sscanf(pch,"%d\t%512s",&num, material_name) != 2) {
 							sprintf(tempbuf, "PROBLEM in A3D_QUAD %d\n", num);
              throw tempbuf;
				}
				else {
					// should look up the materials
					mynum = -1;
					
					for (int i(0); i < valid_num; i++) {
					
						if (strcmp(material_name, mat_names[i]) == 0) {
								mynum = i;
						}
					}
					
					if (mynum >= 0)
					  a3dgeom->BindMaterial(materials[mynum]);
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_QUADS);
				a3dgeom->Tag(num);

				// read in 4 vertices
				for (int i(0); i<4; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
 							sprintf(tempbuf, "Quad (#%d) line too long in model file: %s",num, line);
              throw tempbuf;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  a3dgeom->Vertex3f(x,y,z);
				  }
				 
					}
					else return;
				}
				a3dgeom->End();
				a3dgeom->PopMatrix();
				
			}

			else if(isit("A3D_SUB_QUAD"))
			{

				next();

				if (sscanf(pch,"%d\t%511s\t%f",&num, material_name,&subopeningval) != 3) {
					throw "PROBLEM in A3D_QUAD";
				}
				else {
					// should look up the materials
					mynum = -1;
					for (int i(0); i < valid_num; i++) {
						if (strcmp(material_name, mat_names[i]) == 0)
							mynum = i;
					}

					if (mynum >= 0)
					  a3dgeom->BindMaterial(materials[mynum]); 
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_SUB_QUADS);
				a3dgeom->Tag(num);
				a3dgeom->SetOpeningFactorf(subopeningval); // make sure to call this before specifying vertices

				// read in 4 vertices
				for (int i(0); i<4; i++) {
				  if (fgets(line, sizeof(line), model_file) != NULL) {
				    if (strlen(line) >= sizeof(line)-1) {
 							sprintf(tempbuf, "subQuad (#%d) line too long in model file: %s",num, line);
              throw tempbuf;
					}
			        if ((strlen(line)<3)||(line[0]=='#')) 
			   	    continue; // skip this line

				    // read in the vertices
				    if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
			 		  return;
				    else {
					    a3dgeom->Vertex3f(x,y,z);
					}
				  }
				 else return;
				}
				a3dgeom->End();
				a3dgeom->PopMatrix();
				
			}

			else if(isit("A3D_TRIANGLE"))
			{

				next();

				if (sscanf(pch,"%d\t%511s\t%f",&num, material_name) != 2) {
					throw "PROBLEM in A3D_TRI";
				}
				else {
					// should look up the materials
					mynum = -1;
					for (int i(0); i < valid_num; i++) {
						if (strcmp(material_name, mat_names[i]) == 0)
							mynum = i;
					}
					if (mynum >= 0)
					  a3dgeom->BindMaterial(materials[mynum]); 				
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_TRIANGLES);
				a3dgeom->Tag(num);

				// read in 3 vertices
				for (int i(0); i<3; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
             	sprintf(tempbuf, "Tri (#%d) line too long in model file: %s",num, line);
              throw tempbuf;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  a3dgeom->Vertex3f(x,y,z);
				  }
				  
					}
					else return;
				}
				a3dgeom->End();
				a3dgeom->PopMatrix();
			}

			else if(isit("A3D_SUB_TRIANGLE"))
			{
				next();
				if (sscanf(pch,"%d\t%511s\t%f",&num, material_name,&subopeningval) != 3) {
					throw "PROBLEM in A3D_SUB_TRI";
				}
				else {
					// should look up the materials
					mynum = -1;
					for (int i(0); i < valid_num; i++) {
						if (strcmp(material_name, mat_names[i]) == 0)
							mynum = i;
					}
					if (mynum > 0)
					  a3dgeom->BindMaterial(materials[mynum]); 					
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_SUB_TRIANGLES);
				a3dgeom->Tag(num);
				a3dgeom->SetOpeningFactorf(subopeningval); // make sure to call this before specifying vertices

				// read in 3 vertices
				for (int i(0); i<3; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
 							sprintf(tempbuf, "subTri (#%d) line too long in model file: %s",num, line);
              throw tempbuf;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  a3dgeom->Vertex3f(x,y,z);
				  }
				  					}
					else return;
				}
				a3dgeom->End();
				a3dgeom->PopMatrix();
			}
	}

	fclose(model_file);
}
catch (char* szError) {
  sprintf(tempbuf2,"Error: Loading model file (%s)", szError);
  printf("%s\n", tempbuf2);
  send_message((const char *) tempbuf2,vrpn_TEXT_ERROR,0);
}
return;
}


void vrpn_Sound_Server_A3D::loadModelRemote(){}	// not supported

void vrpn_Sound_Server_A3D::loadPolyQuad(vrpn_QuadDef * quad) {
  
  int mynum = -1;
	for (int i(0); i < maxMaterials; i++) {
	  if (strcmp(quad->material_name, mat_names[i]) == 0)
			mynum = i;
	}

  lasterror = a3dgeom->BindMaterial(materials[mynum]);
  lasterror = a3dgeom->PushMatrix();
  if (quad->subQuad) {
    lasterror = a3dgeom->Begin(A3D_SUB_QUADS);
    lasterror = a3dgeom->SetOpeningFactorf((float)quad->openingFactor); 
  }
  else
    lasterror = a3dgeom->Begin(A3D_QUADS);
  
  lasterror = a3dgeom->Tag(quad->tag);
  for (i=0; i<4; i++)
    lasterror = a3dgeom->Vertex3f((float)quad->vertices[i][0],(float)quad->vertices[i][1],(float)quad->vertices[i][2]);
  lasterror = a3dgeom->End();
  lasterror = a3dgeom->PopMatrix();

  char tempbuf[1024];
  sprintf(tempbuf,"Adding quad");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}

void vrpn_Sound_Server_A3D::loadPolyTri(vrpn_TriDef * tri) {
  int mynum = -1;
	for (int i(0); i < maxMaterials; i++) {
	  if (strcmp(tri->material_name, mat_names[i]) == 0)
			mynum = i;
	}
	
  lasterror = a3dgeom->BindMaterial(materials[mynum]);
  lasterror = a3dgeom->PushMatrix();
  if (tri->subTri) {
    lasterror = a3dgeom->Begin(A3D_SUB_TRIANGLES);
    lasterror = a3dgeom->SetOpeningFactorf((float)tri->openingFactor); 
  }
  else
    lasterror = a3dgeom->Begin(A3D_TRIANGLES);
  
  lasterror = a3dgeom->Tag(tri->tag);
  for (i=0; i<3; i++)
    lasterror = a3dgeom->Vertex3f((float)tri->vertices[i][0],(float)tri->vertices[i][1],(float)tri->vertices[i][2]);
  lasterror = a3dgeom->End();
  lasterror = a3dgeom->PopMatrix();

  char tempbuf[1024];
  sprintf(tempbuf,"Adding tri");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}

void vrpn_Sound_Server_A3D::loadMaterial(vrpn_MaterialDef * material, vrpn_int32 id) {
  char tempbuf[1024];
  for (int i(0); i < maxMaterials; i++) {
		if (strcmp(material->material_name, mat_names[i]) == 0)	{
      sprintf(tempbuf,"Material %s already exists in the list\n", material->material_name);
      printf("%s\n", tempbuf);
      send_message((const char *) tempbuf,vrpn_TEXT_WARNING,0);
			return;
		}
	}

	// copy the name to the name array
	strcpy(mat_names[numMaterials], material->material_name);
	a3dgeom->NewMaterial(&materials[numMaterials]);
	materials[numMaterials]->SetTransmittance((float)material->transmittance_highfreq, (float)material->transmittance_gain);
	materials[numMaterials]->SetReflectance((float)material->reflectance_highfreq, (float)material->reflectance_gain);
	numMaterials++;
	printf("Material %d is %s\n", numMaterials-1, mat_names[numMaterials-1]);
	maxMaterials = numMaterials;
  
  sprintf(tempbuf,"Adding material");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
  return;
}

// moving vertices not supported yet because there is the issue of having to redraw the entire scene.. maybe use lists
void vrpn_Sound_Server_A3D::setPolyQuadVertices(vrpn_float64 vertices[4][3], const vrpn_int32 id) {
  char tempbuf[1024];
  sprintf(tempbuf,"Setting quad vertices not implemented");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
}

void vrpn_Sound_Server_A3D::setPolyTriVertices(vrpn_float64 vertices[3][3], const vrpn_int32 id) {
  char tempbuf[1024];
  sprintf(tempbuf,"Setting tri vertices not implemented");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
}

// need to look up the correct polygon somehow
void vrpn_Sound_Server_A3D::setPolyOF(vrpn_float64 OF, vrpn_int32 tag) {
  char tempbuf[1024];
  sprintf(tempbuf,"Setting polygon opening factor not implemented");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
}

void vrpn_Sound_Server_A3D::setPolyMaterial(const char * material, vrpn_int32 tag) {
  char tempbuf[1024];
  sprintf(tempbuf,"Setting polygon material not implemented");
  printf("%s\n", tempbuf);
  send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);
}

	
void vrpn_Sound_Server_A3D::mainloop() {
  vrpn_Text_Sender::mainloop();
  vrpn_Sound::d_connection->mainloop();
	lasterror = a3droot->Flush();
  server_mainloop();
}

bool vrpn_Sound_Server_A3D::noSounds(void) {return 0;}

void vrpn_Sound_Server_A3D::stopAllSounds() {
  for (int i(0); i<numSounds;i++)
	  a3dsamples[i]->Stop();
}

// re-initialize all sounds and materials etc
void vrpn_Sound_Server_A3D::shutDown() {

  // reinitialize playlist
  printf("Begin cleanup\n");
	for (int i(0); i < maxSounds; i++) {
    if (i<=numSounds)
      a3dsamples[i]->FreeAudioData();

		a3dsamples[i] = NULL;
    soundMap[i]=-1;

	}

  for (int j(0); j<maxMaterials;j++) {
    if (j<=numMaterials)
      delete materials[j];
    materialMap[j]=-1;
    strcpy(mat_names[j],"");
  }

  maxSounds = 15;
	numSounds = -1;
  numMaterials = 0;
  maxMaterials = 0;
  
  a3droot->Clear();
  printf("End cleanup\n");
}

vrpn_float64  vrpn_Sound_Server_A3D::GetCurrentVolume(const vrpn_int32 CurrentSoundId) {
	float      val;
	vrpn_int32 myid = soundMap[CurrentSoundId];
	lasterror = a3dsamples[myid]->GetGain(&val);
	return val;
}

void vrpn_Sound_Server_A3D::GetLastError(char *buf) {
	strcpy(buf,"");
  if (FAILED(lasterror)) {
    sprintf(buf,"ERROR: %d",lasterror);
    send_message((const char *) buf,vrpn_TEXT_ERROR,0);
  }
	return;
}

vrpn_int32  vrpn_Sound_Server_A3D::GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId) {
 vrpn_int32 myid = soundMap[CurrentSoundId];
 return 1;
}

void vrpn_Sound_Server_A3D::GetCurrentPosition(const vrpn_int32 CurrentSoundId, float* X_val, float* Y_val, float* Z_val) {
	vrpn_int32 myid = soundMap[CurrentSoundId];
	lasterror =  a3dsamples[myid]->GetPosition3f(X_val, Y_val, Z_val);
	return;
}

void vrpn_Sound_Server_A3D::GetListenerPosition(float* X_val, float* Y_val, float* Z_val) {
    lasterror =  a3dlis->GetPosition3f(X_val, Y_val, Z_val);
	return;
}

void vrpn_Sound_Server_A3D::GetCurrentDistances(const vrpn_int32 CurrentSoundId, float* FMin, float* FMax) {
	vrpn_int32 myid = soundMap[CurrentSoundId];
	lasterror =  a3dsamples[myid]->GetMinMaxDistance(FMax, FMin,NULL);
	return;
}

void vrpn_Sound_Server_A3D::GetListenerOrientation(float* y_val, float *p_val, float *r_val) {
	// z y x is what we expect to get back..  these arent named right but deal for now
	lasterror =  a3dlis->GetOrientationAngles3f(y_val, p_val, r_val);
	return;
}

void vrpn_Sound_Server_A3D::GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *y_val, float *p_val, float *r_val) {
	
	vrpn_int32 myid = soundMap[CurrentSoundId];
	lasterror =  a3dsamples[myid]->GetOrientationAngles3f(y_val, p_val, r_val);
	return;
}
	
HWND GetConsoleHwnd(void) {
	// Get the current window title.
	char pszOldWindowTitle[CHAR_BUF_SIZE];
	GetConsoleTitle(pszOldWindowTitle, CHAR_BUF_SIZE);

	// Format a unique New Window Title.
	char pszNewWindowTitle[CHAR_BUF_SIZE];
	wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

	// Change the current window title.
	SetConsoleTitle(pszNewWindowTitle);

	// Ensure window title has changed.
	Sleep(40);

	// Look for the new window.
	HWND hWnd = FindWindow(NULL, pszNewWindowTitle);

	// Restore orignal name
	SetConsoleTitle(pszOldWindowTitle);

	return hWnd;
}


void main(int argc, char **argv) {

HWND hWin;
vrpn_Sound_Server_A3D * soundServer = NULL;
vrpn_Tracker_Remote   * tracker_connection;
char                    tracker_device[512];
char                    tracker_name[512];
vrpn_Connection       * connection;
vrpn_Connection       * trackerCon;
int                     got_report;

int USE_TRACKER;

	char	* config_file_name = "vrpn.cfg";
	FILE	* config_file;
	char 	* client_name   = NULL;
	int	    client_port   = 4150;
	int	    bail_on_error = 1;
	int	    verbose       = 1;
	int	    auto_quit     = 0;
	int	    realparams    = 0;
	int 	  loop          = 0;
	int	    port          = vrpn_DEFAULT_LISTEN_PORT_NO;

	connection = new vrpn_Synchronized_Connection (port);
	
	// Open the configuration file
	if (verbose) printf("Reading from config file %s\n", config_file_name);
	
	if ( (config_file = fopen(config_file_name, "r")) == NULL) 
	{
		perror("Cannot open config file");
		printf("  (filename %s)\n", config_file_name);
		return;
	}
		
	// Read the configuration file, creating a device for each entry.
	// Each entry is on one line, which starts with the name of the
	//   class of the object that is to be created.
	// If we fail to open a certain device, print a message and decide
	//  whether we should bail.
	{	
		char	line[512];	// Line read from the input file
		char *pch;
		char    scrap[512], s2[512];
	
		// Read lines from the file until we run out
		while ( fgets(line, sizeof(line), config_file) != NULL ) 
		{
			
			// Make sure the line wasn't too long
			if (strlen(line) >= sizeof(line)-1) 
			{
				printf("Line too long in config file: %s\n",line);
				if (bail_on_error) { return; }
				else { continue; }	// Skip this line
			}
			
			if ((strlen(line)<3)||(line[0]=='#')) 
			{
				// comment or empty line -- ignore
				continue;
			}
			
			// copy for strtok work
			strncpy(scrap, line, sizeof(line) - 1);
			// Figure out the device from the name and handle appropriately
			
			// WARNING: SUBSTRINGS WILL MATCH THE EARLIER STRING, SO 
			// ADD AN EMPTY SPACE TO THE END OF STATIC STRINGS!!!!
			
			//	  #define isit(s) !strncmp(line,s,strlen(s))
#define isit(s) !strcmp(pch=strtok(scrap," \t"),s)
#define next() pch += strlen(pch) + 1
	
			#ifdef _WIN32

			if(isit("vrpn_Sound_Server"))
			{
				printf("%s\n",pch); 
				next();
				printf("%s\n",pch);
				if (sscanf(pch,"%511s\t%d\t%511s\t%511s",s2,&USE_TRACKER,tracker_name, tracker_device) != 4) 
				{
					printf("Bad vrpn_Server_Sound line: %s\n",line);
					if (bail_on_error) 
					{ 
						return; 
					}
					else 
					{ 
						continue; 
					}	// Skip this line
				}

				hWin = GetConsoleHwnd();

				printf("Begin initializing A3D Sound Server\n");	
				soundServer = NULL;
				soundServer = new vrpn_Sound_Server_A3D(s2, connection,hWin);
				if (soundServer == NULL) 
					printf("Can't create sound server\n");
        printf("End A3D Sound Server initialization\n");	
				
			}
#endif
		}
	}

	fclose(config_file);

	// Open remote tracker if we are to use one

	if (USE_TRACKER) {
		
		char newname[1024];
		sprintf(newname,"%s@%s",(const char*)tracker_device, (const char*)tracker_name);
		printf("Using tracker: %s\n",newname);
		trackerCon = vrpn_get_connection_by_name(tracker_name);
		tracker_connection = new vrpn_Tracker_Remote((const char *) newname);
		// SET UP TRACKER HANDLER
		if (trackerCon->doing_okay()) {
			printf( "TC OK.\n");
		} else {
			printf( "TC Not OK.\n");
		}
	}
	else printf("Not using tracker\n");

	loop = 0;
	
	if (client_name) 
	{
		printf( "vrpn_serv: connecting to client: %s:%d\n",
			client_name, client_port);
		if (connection->connect_to_client(client_name, client_port))
		{
			printf( "server: could not connect to client %s:%d\n", client_name, client_port);
		}
	}
	
		
// ********************************************************************
// **                                                                **
// **                MAIN LOOP                                       **
// **                                                                **
// ********************************************************************
float fPrevTime = 0.0f;
float fFrameTime;
float fTime;
int counter = 0;
int stopNow = 0;
int numconnections = 0;
char buf[1024];

	printf("Begin main loop\n");

	while (!stopNow && 	!_kbhit()) {

    soundServer->GetLastError(buf);

    if (!strncmp(buf,"ERROR",5)) {
		  printf("%s", buf);
    }
      counter++;

  	  // record time since last frame 
      if (counter==NUM_SPIN) {
	    fTime = (float)timeGetTime();
	    counter = 0;

	    fFrameTime = (fTime - fPrevTime) * 0.001f;
	  
	    printf("Running at %4.2f Hz\n", (float) NUM_SPIN/fFrameTime);

        fPrevTime = fTime;
      }

	  soundServer->mainloop();
						
		// ensure we get a new report!
		if (USE_TRACKER) {
		  tracker_connection->mainloop();
		  got_report = 0;
		  if (trackerCon->doing_okay())
		    while (!got_report) 
			  tracker_connection->mainloop(); 
		}

		// Send and receive all messages
		connection->mainloop();
		if (numconnections==0 && connection->connected())
           numconnections++;
	
		if (((numconnections!=0) & (!connection->connected())) | !connection->doing_okay())  {
			soundServer->shutDown();
		  numconnections=0;
		}
	}

	printf("about to shutdown\n");
//	delete connection;
   delete soundServer;
}

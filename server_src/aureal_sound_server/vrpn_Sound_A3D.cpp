#include "vrpn_Sound_A3D.h"

#define NUM_SPIN 10000
const int CHAR_BUF_SIZE =1024;

char * PrintError(int code) {
	switch (code) {
		case A3DERROR_MEMORY_ALLOCATION : return "A3DERROR_MEMORY_ALLOCATION"; break;
		case A3DERROR_FAILED_CREATE_PRIMARY_BUFFER : return "A3DERROR_FAILED_CREATE_PRIMARY_BUFFER";break;
		case A3DERROR_FAILED_CREATE_SECONDARY_BUFFER : return "A3DERROR_FAILED_CREATE_SECONDARY_BUFFER"; break;
		case A3DERROR_FAILED_INIT_A3D_DRIVER : return "A3DERROR_FAILED_INIT_A3D_DRIVER"; break;
		case A3DERROR_FAILED_QUERY_DIRECTSOUND : return "A3DERROR_FAILED_QUERY_DIRECTSOUND"; break;
		case A3DERROR_FAILED_QUERY_A3D3 : return "A3DERROR_FAILED_QUERY_A3D3"; break; 
		case A3DERROR_FAILED_INIT_A3D3 : return "A3DERROR_FAILED_INIT_A3D3"; break;
		case A3DERROR_FAILED_QUERY_A3D2: return "A3DERROR_FAILED_QUERY_A3D2"; break;
		case A3DERROR_FAILED_FILE_OPEN: return "A3DERROR_FAILED_FILE_OPEN"; break;
		case A3DERROR_FAILED_CREATE_SOUNDBUFFER: return "A3DERROR_FAILED_CREATE_SOUNDBUFFER"; break;
		case A3DERROR_FAILED_QUERY_3DINTERFACE: return "A3DERROR_FAILED_QUERY_3DINTERFACE"; break;
		case A3DERROR_FAILED_LOCK_BUFFER: return "A3DERROR_FAILED_LOCK_BUFFER"; break;
		case A3DERROR_FAILED_UNLOCK_BUFFER: return "A3DERROR_FAILED_UNLOCK_BUFFER"; break;
		case A3DERROR_UNRECOGNIZED_FORMAT: return "A3DERROR_UNRECOGNIZED_FORMAT"; break;
		case A3DERROR_NO_WAVE_DATA: return "A3DERROR_NO_WAVE_DATA"; break;
		case A3DERROR_UNKNOWN_PLAYMODE: return "A3DERROR_UNKNOWN_PLAYMODE"; break;
		case A3DERROR_FAILED_PLAY: return "A3DERROR_FAILED_PLAY"; break;
		case A3DERROR_FAILED_STOP: return "A3DERROR_FAILED_STOP"; break;
		case A3DERROR_NEEDS_FORMAT_INFORMATION: return "A3DERROR_NEEDS_FORMAT_INFORMATION"; break;
		case A3DERROR_FAILED_ALLOCATE_WAVEDATA: return "A3DERROR_FAILED_ALLOCATE_WAVEDATA"; break;
		case A3DERROR_NOT_VALID_SOURCE: return "A3DERROR_NOT_VALID_SOURCE"; break;
		case A3DERROR_FAILED_DUPLICATION: return "A3DERROR_FAILED_DUPLICATION"; break;
		case A3DERROR_FAILED_INIT: return "A3DERROR_FAILED_INIT"; break;
		case A3DERROR_FAILED_SETCOOPERATIVE_LEVEL: return "A3DERROR_FAILED_SETCOOPERATIVE_LEVEL"; break;
		case A3DERROR_FAILED_INIT_QUERIED_INTERFACE: return "A3DERROR_FAILED_INIT_QUERIED_INTERFACE"; break;
		case A3DERROR_GEOMETRY_INPUT_OUTSIDE_BEGIN_END_BLOCK: return "A3DERROR_GEOMETRY_INPUT_OUTSIDE_BEGIN_END_BLOCK"; break;
		case A3DERROR_INVALID_NORMAL: return "A3DERROR_INVALID_NORMAL"; break;
		case A3DERROR_END_BEFORE_VALID_BEGIN_BLOCK: return "A3DERROR_END_BEFORE_VALID_BEGIN_BLOCK"; break;
		case A3DERROR_INVALID_BEGIN_MODE: return "A3DERROR_INVALID_BEGIN_MODE"; break;				
		case A3DERROR_INVALID_ARGUMENT: return "A3DERROR_INVALID_ARGUMENT"; break;
		case A3DERROR_INVALID_INDEX: return "A3DERROR_INVALID_INDEX"; break;
		case A3DERROR_INVALID_VERTEX_INDEX: return "A3DERROR_INVALID_VERTEX_INDEX"; break;
		case A3DERROR_INVALID_PRIMITIVE_INDEX: return "A3DERROR_INVALID_PRIMITIVE_INDEX"; break;
		case A3DERROR_MIXING_2D_AND_3D_MODES: return "A3DERROR_MIXING_2D_AND_3D_MODES"; break;
		case A3DERROR_2DWALL_REQUIRES_EXACTLY_ONE_LINE: return "A3DERROR_2DWALL_REQUIRES_EXACTLY_ONE_LINE"; break;
		case A3DERROR_NO_PRIMITIVES_DEFINED: return "A3DERROR_NO_PRIMITIVES_DEFINED"; break;
		case A3DERROR_PRIMITIVES_NON_PLANAR: return "A3DERROR_PRIMITIVES_NON_PLANAR"; break;
		case A3DERROR_PRIMITIVES_OVERLAPPING: return "A3DERROR_PRIMITIVES_OVERLAPPING"; break;
		case A3DERROR_PRIMITIVES_NOT_ADJACENT: return "A3DERROR_PRIMITIVES_NOT_ADJACENT"; break;
		case A3DERROR_OBJECT_NOT_FOUND: return "A3DERROR_OBJECT_NOT_FOUND"; break;		
		case A3DERROR_ROOM_HAS_NO_SHELL_WALLS: return "A3DERROR_ROOM_HAS_NO_SHELL_WALLS"; break;
		case A3DERROR_WALLS_DO_NOT_ENCLOSE_ROOM: return "A3DERROR_WALLS_DO_NOT_ENCLOSE_ROOM"; break;
		case A3DERROR_INVALID_WALL: return "A3DERROR_INVALID_WALL"; break;					
		case A3DERROR_ROOM_HAS_LESS_THAN_4SHELL_WALLS: return "A3DERROR_ROOM_HAS_LESS_THAN_4SHELL_WALLS"; break;
		case A3DERROR_ROOM_HAS_LESS_THAN_3UNIQUE_NORMALS: return "A3DERROR_ROOM_HAS_LESS_THAN_3UNIQUE_NORMALS"; break;
		case A3DERROR_INTERSECTING_WALL_EDGES: return "A3DERROR_INTERSECTING_WALL_EDGES"; break;		
		case A3DERROR_INVALID_ROOM: return "A3DERROR_INVALID_ROOM"; break;					
		case A3DERROR_SCENE_HAS_ROOMS_INSIDE_ANOTHER_ROOMS: return "A3DERROR_SCENE_HAS_ROOMS_INSIDE_ANOTHER_ROOMS"; break;
		case A3DERROR_SCENE_HAS_OVERLAPPING_STATIC_ROOMS	: return "A3DERROR_SCENE_HAS_OVERLAPPING_STATIC_ROOMS"; break;
		case A3DERROR_DYNAMIC_OBJ_UNSUPPORTED: return "A3DERROR_DYNAMIC_OBJ_UNSUPPORTED"; break;		
		case A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR: return "A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR"; break;
		case A3DERROR_INVALID_ROOM_INDEX: return "A3DERROR_INVALID_ROOM_INDEX"; break;
		case A3DERROR_INVALID_WALL_INDEX: return "A3DERROR_INVALID_WALL_INDEX"; break;
		case A3DERROR_SCENE_INVALID: return "A3DERROR_SCENE_INVALID"; break;
		case A3DERROR_UNIMPLEMENTED_FUNCTION: return "A3DERROR_UNIMPLEMENTED_FUNCTION"; break;
		case A3DERROR_NO_ROOMS_IN_SCENE: return "A3DERROR_NO_ROOMS_IN_SCENE"; break;
		case A3DERROR_2D_GEOMETRY_UNIMPLEMENTED: return "A3DERROR_2D_GEOMETRY_UNIMPLEMENTED"; break;
		case A3DERROR_OPENING_NOT_WALL_COPLANAR: return "A3DERROR_OPENING_NOT_WALL_COPLANAR"; break;
		case A3DERROR_OPENING_NOT_VALID: return "A3DERROR_OPENING_NOT_VALID"; break;						
		case A3DERROR_INVALID_OPENING_INDEX: return "A3DERROR_INVALID_OPENING_INDEX"; break;
		case A3DERROR_FEATURE_NOT_REQUESTED: return "A3DERROR_FEATURE_NOT_REQUESTED"; break;
		case A3DERROR_FEATURE_NOT_SUPPORTED: return "A3DERROR_FEATURE_NOT_SUPPORTED"; break;
		case A3DERROR_FUNCTION_NOT_VALID_BEFORE_INIT: return "A3DERROR_FUNCTION_NOT_VALID_BEFORE_INIT"; break;
		case A3DERROR_INVALID_NUMBER_OF_CHANNELS: return "A3DERROR_INVALID_NUMBER_OF_CHANNELS"; break;
		case A3DERROR_SOURCE_IN_NATIVE_MODE: return "A3DERROR_SOURCE_IN_NATIVE_MODE"; break;
		case A3DERROR_SOURCE_IN_A3D_MODE: return "A3DERROR_SOURCE_IN_A3D_MODE"; break;
		case A3DERROR_BBOX_CANNOT_ENABLE_AFTER_BEGIN_LIST_CALL: return "A3DERROR_BBOX_CANNOT_ENABLE_AFTER_BEGIN_LIST_CALL"; break;
		case A3DERROR_CANNOT_CHANGE_FORMAT_FOR_ALLOCATED_BUFFER: return "A3DERROR_CANNOT_CHANGE_FORMAT_FOR_ALLOCATED_BUFFER"; break;
		case A3DERROR_FAILED_QUERY_DIRECTSOUNDNOTIFY: return "A3DERROR_FAILED_QUERY_DIRECTSOUNDNOTIFY"; break;
		case A3DERROR_DIRECTSOUNDNOTIFY_FAILED: return "A3DERROR_DIRECTSOUNDNOTIFY_FAILED"; break;
		case A3DERROR_RESOURCE_MANAGER_ALWAYS_ON: return "A3DERROR_RESOURCE_MANAGER_ALWAYS_ON"; break;
		case A3DERROR_CLOSED_LIST_CANNOT_BE_CHANGED: return "A3DERROR_CLOSED_LIST_CANNOT_BE_CHANGED"; break;
		case A3DERROR_END_CALLED_BEFORE_BEGIN: return "A3DERROR_END_CALLED_BEFORE_BEGIN"; break;
		case A3DERROR_UNMANAGED_BUFFER: return "A3DERROR_UNMANAGED_BUFFER"; break;
		case A3DERROR_COORD_SYSTEM_CAN_ONLY_BE_SET_ONCE: return "A3DERROR_COORD_SYSTEM_CAN_ONLY_BE_SET_ONCE"; break;
		case A3DERROR_BUFFER_IN_SOFTWARE	: return "A3DERROR_BUFFER_IN_SOFTWARE"; break;
		case A3DERROR_INITIAL_PARAMETERS_NOT_SET: return "A3DERROR_INITIAL_PARAMETERS_NOT_SET"; break;
		case A3DERROR_INCORRECT_FORMAT_SPECIFIED: return "A3DERROR_INCORRECT_FORMAT_SPECIFIED"; break;
	default: return "*********** UNKNOWN ERROR ********"; break;
	};
}

vrpn_Sound_Server_A3D::vrpn_Sound_Server_A3D(const char      * name, 
											 vrpn_Connection * c, 
											 HWND              hWin) 
						   : vrpn_Sound_Server(name, c){

	// initialize the goodies
	a3droot     = NULL;
    a3dgeom     = NULL;
    a3dlis      = NULL;
    
	A3dRegister();

	// initialize COM 
	CoInitialize(NULL);

	// init the a3d root
	lasterror = A3dCreate(NULL, (void **)&a3droot, NULL, A3D_1ST_REFLECTIONS | A3D_OCCLUSIONS);

    if (FAILED(lasterror)) {
		printf("Error in CoCreateInstance in constructor\n");
		return;
	}

	// ensure that we have exclusive use of the card

	a3droot->SetCooperativeLevel(hWin, A3D_CL_EXCLUSIVE);

	lasterror = a3droot->QueryInterface(IID_IA3dListener, (void **)&a3dlis);

    if (FAILED(lasterror)) {
		printf("Couldnt get Listener in constructor\n");
		return;
	}

	lasterror = a3droot->QueryInterface(IID_IA3dGeom, (void **)&a3dgeom);
	if (FAILED(lasterror)) {
		printf("Couldnt get Geometry in constructor\n");
		return;
	}

	a3dgeom->Enable(A3D_OCCLUSIONS);
	a3dgeom->Enable(A3D_1ST_REFLECTIONS);

	// set some global default values
	// default DistanceModelScale is 1.0 which means gain is reducded 6dB for doubling in distance from listener
	// default DopplerScale is 1.0 (340m/s)
	// default Equalization is 1.0
	// default MaxReflectionDelayTime is .3 seconds which according to
	//   the A3D document is adequate for models smaller than a football stadium
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
	
	a3dsamples = new (IA3dSource*);

	// initialize playlist
	for (int i(0); i < maxSounds; i++) {
		a3dsamples[i] = NULL;
	}

	a3droot->Clear();

}
					
vrpn_Sound_Server_A3D::~vrpn_Sound_Server_A3D() {

	printf("Before freeing... \n");
	// free up COM
	CoUninitialize();

	a3droot->Shutdown();
	printf("After free... \n");
}

	
void vrpn_Sound_Server_A3D::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef) {
    
	if (id <= numSounds) {
	  if (repeat == 0)
	    lasterror = a3dsamples[id]->Play(A3D_LOOPED);
	  else 
		lasterror = a3dsamples[id]->Play(A3D_SINGLE);
	  printf("Playing sound %d\n", id);
	}
}

void vrpn_Sound_Server_A3D::stopSound(vrpn_SoundID id) {
	printf("Stopping sound %d\n",id);
	lasterror = a3dsamples[id]->Stop();
}

void vrpn_Sound_Server_A3D::loadSound(char* filename, vrpn_SoundID id) {

	printf("Loading sound #%d: %s\n",numSounds+1,filename);
   numSounds++;
   if (numSounds == maxSounds) {
		maxSounds = 2 * maxSounds;
		IA3dSource ** temp = new (IA3dSource*);

		for (int i(0); i < maxSounds; i++) {
			if (i <= numSounds)
			  temp[i] = a3dsamples[i];
			else temp[i] = NULL;
		}
		delete [] a3dsamples;
		a3dsamples = temp;
	}

   lasterror = a3droot->NewSource( A3DSOURCE_INITIAL_RENDERMODE_A3D , &a3dsamples[numSounds]);

  if (FAILED(lasterror)) {
    printf("Error making new source\n");
	return;
  }

  lasterror = a3dsamples[numSounds]->LoadWaveFile(filename);
  
  if (FAILED(lasterror)) {
    printf("Error making new source\n");
	return;
  }

  // initialize sound params

  a3dsamples[numSounds]->SetCone(0,0,1);
  // these values are defaults:
  // default ModelScale of 6dB for each doubling of distance
  // default DopplerScale of 1.0
  // default Equalization of 1.0
  // default Gain is 1.0 
  // default orientation doesnt matter since we have no directional (cone) source
  // default pitch is 1.0 (unaltered pitch)
  // default priority of .5 on a scale from 0 to 1
  // default ReflectionDelayScale is 1.0 (normal delay in reflections [.3 seconds])
  // default RefelectionGainScale is 1.0 
  // default RenderMode uses occlusion, reflection, a3d (??)
  // default TransformMode is relative to origin
    
  
  a3dsamples[numSounds]->SetMinMaxDistance(1.0, 6.0, A3D_MUTE);

  // default position is at the origin
  a3dsamples[numSounds]->SetPosition3f(0.0,0.0,0.0);
}

void vrpn_Sound_Server_A3D::unloadSound(vrpn_SoundID id) {
  printf("Unloading sound %d\n", id);
  a3dsamples[id]->Release();

  for (int i(id); i < numSounds; i++) {
	a3dsamples[i] = a3dsamples[i+1];
  }
}

void vrpn_Sound_Server_A3D::changeListenerStatus(vrpn_ListenerDef listenerDef) {
	q_vec_type 	   angles;
	q_matrix_type  colMatrix;
	A3DVAL         X_val, Y_val, Z_val;

//	printf("Setting listener coords to %f %f %f\n",listenerDef.pose.position[0],listenerDef.pose.position[1],listenerDef.pose.position[2]);
	// go from the listeners orientation quaternion to euler angles
	// A3D wants angles in DEGREES!!
	q_to_col_matrix (colMatrix, listenerDef.pose.orientation);
	q_col_matrix_to_euler(angles,colMatrix);

	X_val = angles[0]*180.0/Q_PI;
	Y_val = angles[1]*180.0/Q_PI;
	Z_val = angles[2]*180.0/Q_PI;

	// uses Y(up),X(right),Z(out)
	a3dlis->SetOrientationAngles3f(Y_val,X_val,Z_val);

	a3dlis->SetPosition3f(listenerDef.pose.position[0],listenerDef.pose.position[1],listenerDef.pose.position[2]);
	// right now we ignore velocity
}
						
void vrpn_Sound_Server_A3D::changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef) {
	q_vec_type 	   angles;
	q_matrix_type  colMatrix;
	A3DVAL         X_val, Y_val, Z_val;

	printf("Setting sound coords to %f %f %f\n",soundDef.pose.position[0],soundDef.pose.position[1],soundDef.pose.position[2]);
	// for right now we ignore everything but position since we assume omnidirectional sounds
	a3dsamples[id]->SetPosition3f(soundDef.pose.position[0],soundDef.pose.position[1],soundDef.pose.position[2]);

	// do angles as in listener.. look there for a bit more commentation
	q_to_col_matrix (colMatrix, soundDef.pose.orientation);
	q_col_matrix_to_euler(angles,colMatrix);

	X_val = angles[0]*180.0/Q_PI;
	Y_val = angles[1]*180.0/Q_PI;
	Z_val = angles[2]*180.0/Q_PI;
	a3dsamples[id]->SetOrientationAngles3f(Y_val, X_val, Z_val);
	
	a3dsamples[id]->SetMinMaxDistance(soundDef.max_front_dist, soundDef.max_front_dist, A3D_MUTE);
	return;
}
void vrpn_Sound_Server_A3D::initModel(vrpn_ModelDef modelDef) {
}

void vrpn_Sound_Server_A3D::DrawModel(char * filename) {

	FILE  * model_file;
	float   x,y,z;
	int     num = 0;

	IA3dMaterial *materials[10];
	char 		  mat_names[10][512];
	int           valid_num = 0;
	int           mynum = 0;
	int           curnum = 0;
	char          material_name[512];
	float         trans1, trans2, refl1, refl2;
	float		  subopeningval;
	// open the file
		
	for (int i(0); i<10;i++)
		strcpy(mat_names[i],"");
	
	if ( (model_file = fopen(filename, "r")) == NULL) 
	{
		perror("Cannot open model file");
		printf("  (filename %s)\n", filename);
		return;
	}

	char	line[512];	// Line read from the input file
	char  * pch;
	char    scrap[512];
	
	// Read lines from the file until we run out
	while ( fgets(line, sizeof(line), model_file) != NULL ) 
		{
			
			// Make sure the line wasn't too long
			if (strlen(line) >= sizeof(line)-1) {
				printf("Line too long in config file: %s\n",line);
				return;
			}
			
			if ((strlen(line)<3)||(line[0]=='#')) {
				continue;
			}
			
			// copy for strtok work
			strncpy(scrap, line, 512);
			
#define isit(s) !strcmp(pch=strtok(scrap," \t"),s)
#define next() pch += strlen(pch) + 1
			
			// for now just have 10 materials
			
			if(isit("MATERIAL"))
			{	
			// material name should be first followed by trans and reflect values
				next();
				printf("%s\n",pch);
				if (sscanf(pch,"%511s\t%f\t%f\t%f\t%f\n",material_name, &trans1, &trans2, &refl1, &refl2) != 5) {
					printf("PROBLEM in material list with material: %s\n", material_name);
				}
				else {
					// make sure name is not already in the list
					for (int i(0); i < 10; i++) {
						if (strcmp(material_name, mat_names[i]) == 0)
						{
							printf("Material %s already exists in the list\n", material_name);
							return;
						}
					}

					// copy the name to the name array
					strcpy(mat_names[curnum], material_name);
					a3dgeom->NewMaterial(&materials[curnum]);
					materials[curnum]->SetTransmittance(trans1, trans2);
					materials[curnum]->SetReflectance(refl1, refl2);
					curnum++;
					printf("Material %d is %s\n", curnum-1, mat_names[curnum-1]);
					valid_num = curnum;
				}
			}
	
			else if(isit("A3D_QUAD"))
			{

				next();

				if (sscanf(pch,"%d\t%512s",&num, material_name) != 2) {
					printf("PROBLEM in A3D_QUAD %d\n", num);
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
					printf("Quad #%d bound to material: %s\n", num, material_name);
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_QUADS);
				a3dgeom->Tag(num);

				// read in 4 vertices
				for (int i(0); i<4; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
				    printf("Quad (#%d) line too long in model file: %s\n",num, line);
				    return;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  printf("%f\n",x);
					  a3dgeom->Vertex3f(x,y,z);
				  }
				 
					}
					else return;
				}
				
			}

			else if(isit("A3D_SUB_QUAD"))
			{
				printf("%s\n",pch); 
				next();
				printf("%s\n",pch);
				if (sscanf(pch,"%d\t%511s\t%f",&num, material_name,&subopeningval) != 3) {
					printf("PROBLEM in A3D_QUAD\n");
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
					printf("Sub_Quad #%d bound to material: %s\n", num, material_name);
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_SUB_QUADS);
				a3dgeom->Tag(num);
				a3dgeom->SetOpeningFactorf(subopeningval); // make sure to call this before specifiying vertices

				// read in 4 vertices
				for (int i(0); i<4; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
				    printf("Quad (#%d) line too long in model file: %s\n",num, line);
				    return;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  printf("%f\n",x);
					  a3dgeom->Vertex3f(x,y,z);
				  }
				 
					}
					else return;
				}

				
			}

			else if(isit("A3D_TRIANGLE"))
			{
				printf("%s\n",pch); 
				next();
				printf("%s\n",pch);
				if (sscanf(pch,"%d\t%511s\t%f",&num, material_name) != 2) {
					printf("PROBLEM in A3D_QUAD\n");
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
					printf("Triangle #%d bound to material: %s\n", num, material_name);
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_TRIANGLES);
				a3dgeom->Tag(num);

				// read in 3 vertices
				for (int i(0); i<3; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
				    printf("Tri (#%d) line too long in model file: %s\n",num, line);
				    return;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  printf("%f\n",x);
					  a3dgeom->Vertex3f(x,y,z);
				  }
				  
					}
					else return;
				}

				
			}

			else if(isit("A3D_SUB_TRIANGLE"))
			{
				printf("%s\n",pch); 
				next();
				printf("%s\n",pch);
				if (sscanf(pch,"%d\t%511s\t%f",&num, material_name,&subopeningval) != 3) {
					printf("PROBLEM in A3D_QUAD\n");
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
					printf("Sub_triangle #%d bound to material: %s\n", num, material_name);
				}

				a3dgeom->PushMatrix();
				a3dgeom->Begin(A3D_SUB_TRIANGLES);
				a3dgeom->Tag(num);
				a3dgeom->SetOpeningFactorf(subopeningval); // make sure to call this before specifiying vertices

				// read in 3 vertices
				for (int i(0); i<3; i++) {
					if (fgets(line, sizeof(line), model_file) != NULL) {
				  if (strlen(line) >= sizeof(line)-1) {
				    printf("Tri (#%d) line too long in model file: %s\n",num, line);
				    return;
				  }
			      if ((strlen(line)<3)||(line[0]=='#')) 
			   	  continue; // skip this line

				  // read in the vertices
				  if (sscanf(line,"%f\t%f\t%f",&x, &y, &z) != 3) 
					  return;
				  else {
					  printf("%f\n",x);
					  a3dgeom->Vertex3f(x,y,z);
				  }
				  					}
					else return;
				}
				
			}
	}

	fclose(model_file);

return;
}
	
void vrpn_Sound_Server_A3D::mainloop(const struct timeval * timeout) {
float        fSrcXYZ[3] = {5.0f, 0.0f, -5.0f};
float        fPolyXYZ[3] = {2.5f, 0.0f, 3.0f};

	if (!connection->connected())
	{
		unloadCSMap();
	}
	connection->mainloop(timeout);
	lasterror = a3droot->Flush();
}
bool vrpn_Sound_Server_A3D::noSounds(void) {return 0;}

void vrpn_Sound_Server_A3D::stopAllSounds() {
  for (int i(0); i<numSounds;i++)
	  a3dsamples[i]->Stop();
}

void vrpn_Sound_Server_A3D::shutDown() {
}
vrpn_int32  vrpn_Sound_Server_A3D::GetCurrentVolume(const vrpn_int32 CurrentSoundId) {
	float val;
	lasterror = a3dsamples[CurrentSoundId]->GetGain(&val);
	return val;
}

char * vrpn_Sound_Server_A3D::GetLastError() {
	char buf[1024];
	strcpy(buf,"");
	if (FAILED(lasterror))
    sprintf(buf,"ERROR: %s",(const char*)PrintError(lasterror));
	return buf;
}

vrpn_int32  vrpn_Sound_Server_A3D::GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId) {
 return 1;
}

void vrpn_Sound_Server_A3D::GetCurrentPosition(const vrpn_int32 CurrentSoundId, float* X_val, float* Y_val, float* Z_val) {
	lasterror =  a3dsamples[CurrentSoundId]->GetPosition3f(X_val, Y_val, Z_val);
	return;
}

void vrpn_Sound_Server_A3D::GetListenerPosition(float* X_val, float* Y_val, float* Z_val) {
    lasterror =  a3dlis->GetPosition3f(X_val, Y_val, Z_val);
	return;
}

void vrpn_Sound_Server_A3D::GetCurrentDistances(const vrpn_int32 CurrentSoundId, float* FMin, float* FMax) {
	lasterror =  a3dsamples[CurrentSoundId]->GetMinMaxDistance(FMax, FMin,NULL);
	return;
}

void vrpn_Sound_Server_A3D::GetListenerOrientation(float* X_val, float *Y_val, float *Z_val) {
	// z y x is what we expect to get back..  these arent named right but deal for now
	lasterror =  a3dlis->GetOrientationAngles3f(Y_val, Z_val, X_val);
	return;
}

void vrpn_Sound_Server_A3D::GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *X_val, float *Y_val, float *Z_val) {
	// z y x is what we expect to get back..  these arent named right but deal for now
	lasterror =  a3dsamples[CurrentSoundId]->GetOrientationAngles3f(Y_val, Z_val, X_val);
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

int got_report;
void main(int argc, char **argv) {

HWND hWin;
vrpn_Sound_Server_A3D * soundServer = NULL;
vrpn_Tracker_Remote   * tracker_connection;
char                    tracker_device[512];
char                    tracker_name[512];
vrpn_Connection       * connection;
vrpn_Connection       * trackerCon;

int USE_TRACKER;

	char	* config_file_name = "vrpn.cfg";
	char	  modelFileName[512];
	FILE	* config_file;
	char 	* client_name   = NULL;
	int	      client_port   = 4150;
	int	      bail_on_error = 1;
	int	      verbose       = 1;
	int	      auto_quit     = 0;
	int	      realparams    = 0;
	int 	  loop          = 0;
	int	      port          = vrpn_DEFAULT_LISTEN_PORT_NO;

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
			strncpy(scrap, line, 512);
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
				if (sscanf(pch,"%511s\t%d\t%511s\t%511s\t%511s",s2,&USE_TRACKER,tracker_name, tracker_device,modelFileName) != 5) 
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

				printf("about to init sound server\n");	
				soundServer = NULL;
				soundServer = new vrpn_Sound_Server_A3D(s2, connection,hWin);
				if (soundServer == NULL) 
					printf("Can't create sound server\n");
				
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

	// Open the Forwarder Server

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

	if (strcmp(modelFileName,"NOMODEL") != 0) { // will use a model
		printf("Drawing model from %s\n", modelFileName);
		soundServer->DrawModel(modelFileName);
	}
	else printf("Not using soundmodel file\n");
		
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

	printf("Begin main loop\n");
	while (!stopNow && 	!_kbhit()) {

	  if (!strncmp(soundServer->GetLastError(),"ERROR",5))
		printf("%s", soundServer->GetLastError());

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

		if (!connection->connected() || !connection->doing_okay())  {
			soundServer->shutDown();
		    stopNow =1;
		}
	}

	printf("about to shutdown\n");
	delete connection;

	soundServer->shutDown();
}
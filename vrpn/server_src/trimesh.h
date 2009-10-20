/*	Trimesh.h 
	this class represents a triangle mesh for haptic display
*/

#ifndef TRIMESH_H
#define TRIMESH_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#include "ghost.h"

//XXX There's no easy was to implement this class in the absence of GHOST.
//XXX It may make sense to pull it out of ForceDevice and provide it as a
//XXX separate interface.  Who knows?

#ifndef	VRPN_USE_HDAPI

// the different types of trimeshes we have available for haptic display
enum TrimeshType {GHOST,HCOLLIDE};
// the GHOST mesh does not properly deal with vertex normals


class Trimesh {
protected:
  // we actually store our verts as doubles for the sake of gstPolyMesh
  TrimeshType ourType;

  // this is the ghost polymesh we allocated (if we got one)
  gstTriPolyMesh *ghostPolyMesh;
  
  // this is the trimesh we are displaying
  gstTriPolyMeshHaptic *gstMesh;
  // the node containing this trimesh
  gstSeparator *ourNode;
  // true once we've been added to ourNode
  bool inNode;

  float minX,minY,minZ,maxX,maxY,maxZ;

  //pre: NULL==gstMesh; && NULL==ghostPolyMesh
  void getGstMesh();

  float xFormMat[16];

  // flags to only print warnings once
  bool scpWarn,normWarn;
public:

  // display with ghost by default
  Trimesh(TrimeshType oT=GHOST);

  ~Trimesh(){ clear(); }

  void clear();

  // true if there is a mesh being displayed
  bool displayStatus();

  void setType(TrimeshType newType){ 
    ourType=newType; 
    scpWarn=normWarn=false;
  }

  void startRecording();
  void stopRecording();

  // set the new bounding volume for this object, 
  //     that is the smallest cube which will contain all the verts 
  //     for the duration of the modifications
  // for HCOLLIDE this should really be called before any of the below functions 
  void setBoundingVolume(const float &nMinX,const float &nMinY,const float &nMinZ,
			 const float &nMaxX,const float &nMaxY,const float &nMaxZ);


  // --- modify one step at a time --------------------

  // vertNum normNum & triNum start at 0, true on success
  bool setVertex(int vertNum,double x,double y,double z);
  bool setNormal(int normNum,double x,double y,double z);
  // pre: specified verts and norms have already been set
  bool setTriangle(int triNum,int vert0,int vert1,int vert2,
		   int norm0=-1,int norm1=-1,int norm2=-1);
  bool removeTriangle(int triNum);
  bool updateChanges();
  // --------------------------------------------------

  bool getVertex(int vertNum, float &x,float &y,float &z);

  // if HCOLLIDE = ourType, returns the id of the triangle the probe is contacting
  //    otherwise it returns -1
  int getScpTriId();

  void addToScene(gstSeparator *ourScene);

  void setSurfaceKspring(double Ks){
    gstMesh->setSurfaceKspring(Ks);
  }
  void setSurfaceFstatic(double Fs){
    gstMesh->setSurfaceFstatic(Fs);
  }
  void setSurfaceFdynamic(double Fd){
    gstMesh->setSurfaceFdynamic(Fd);
  }
  void setSurfaceKdamping(double Kd){
    gstMesh->setSurfaceKdamping(Kd);
  }

  // set the transformatrix for the mesh (xFormMat is in row major order)
  void setTransformMatrix(float xFormMat[16]);

  void setTouchableFromBothSides(bool touch=true);
};

#endif	// VRPN_USE_HDAPI

#endif
#endif  // TRIMESH_H

/*	Trimesh.h 
	this class represents a triangle mesh for haptic display
*/

#ifndef TRIMESH_H
#define TRIMESH_H
/*
#include <gstPHANToM.h>
#include <gstShape.h>
*/ 
#include "GHOST.H"

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
  gstShape *gstMesh;
  // the node containing this trimesh
  gstSeparator *ourNode;
  // true once we've been added to ourNode
  bool inNode;

  float minX,minY,minZ,maxX,maxY,maxZ;
public:
  
  Trimesh(TrimeshType oT=GHOST);

  ~Trimesh(){ clear(); }

  void clear();

  // true if there is a mesh being displayed
  bool displayStatus();

  void setType(TrimeshType newType){ ourType=newType; }

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
  bool setTriangle(int triNum,int vert0,int vert1,int vert2,
		   int norm0=-1,int norm1=-1,int norm2=-1);
  bool removeTriangle(int triNum);
  bool updateChanges();
  // --------------------------------------------------

  bool getVertex(int vertNum, double &x,double &y,double &z);

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
  void setTransformMatrix(float xFormMat[16]){
	// it appears that ghost prefers column major order
	gstMesh->setTransformMatrix(
		gstTransformMatrix(xFormMat[0],xFormMat[4],xFormMat[8],xFormMat[12],
				   xFormMat[1],xFormMat[5],xFormMat[9],xFormMat[13],
				   xFormMat[2],xFormMat[6],xFormMat[10],xFormMat[14],
				   xFormMat[3],xFormMat[7],xFormMat[11],xFormMat[15])); 
  }
};

#endif  // TRIMESH_H








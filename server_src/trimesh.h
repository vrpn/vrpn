/*	mesh.h 
	this class represents a triangle mesh
	currently it is just a gstPolyMesh
*/

#ifndef MESH_H
#define MESH_H
/*
#include <gstPHANToM.h>
#include <gstShape.h>
*/ 
#include "GHOST.H"

class Trimesh {
protected:
  // we actually store our verts as doubles for the sake of gstPolyMesh
  int numVerts;
  double (*vert)[3];
  int numTris;
  int (*tri)[3];

  gstPolyMesh *gstMesh;
  // the node containing this trimesh
  gstSeparator *ourNode;
public:
  Trimesh(int nV,float v[][3],int nT,int t[][3]);

  void clear();

  // true if there is a mesh being displayed
  bool displayStatus();
	
  // --- define one step at a time --------------------
  void startDefining(int nV,int nT);
  // vertNum & triNum start at 0, true on success
  bool setVertex(int vertNum,double x,double y,double z);
  bool setTriangle(int triNum,int vert0,int vert1,int vert2);
  void finishDefining();
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

#endif  // MESH_H


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
};

#endif  // MESH_H


#include "trimesh.h"

Trimesh::Trimesh(int nV,float v[][3],int nT,int t[][3]){
  ourNode=NULL;
  gstMesh=NULL;
  vert=NULL;
  tri=NULL;
  numVerts=nV;
  numTris=nT;

  if(numVerts!=0 && numTris!=0){
    vert=new double[numVerts][3];
    tri=new int[numTris][3];

    int i;
    for(i=0;i<numVerts;i++){
      for(int j=0;j<3;j++){
	vert[i][j]=(double)v[i][j];
      }
    }

    for(i=0;i<numTris;i++){
      for(int j=0;j<3;j++){
	tri[i][j]=t[i][j];
      }
    }
    gstMesh=new gstPolyMesh(numVerts,vert,numTris,tri);

  }
}

void Trimesh::clear(){
  delete vert;
  vert=NULL;
  delete tri;
  tri=NULL;
  numVerts=0;
  numTris=0;

  if(NULL!=ourNode && NULL!=gstMesh){
    ourNode->removeChild(gstMesh);
    delete gstMesh;
    gstMesh=NULL;
  }
}

// true if there is a mesh being displayed
bool Trimesh::displayStatus(){
	if(NULL!=ourNode && NULL!=gstMesh){
		return true;
	}
	else 
		return false;
}

void Trimesh::startDefining(int nV,int nT){
  clear();
  if(nV>0 && nT>0){
    numVerts=nV;
    numTris=nT;
    
    vert=new double[numVerts][3];
    tri=new int[numTris][3];
  }
}

bool Trimesh::getVertex(int vertNum,double &x,double &y,double &z){
  if(vertNum >= numVerts || vertNum < 0){
    fprintf(stderr,"Trimesh::error: vertNum=%d out of range [%d,%d)\n",vertNum,0
,numVerts);
    return false;
  }
  else {
    x = vert[vertNum][0];
    y = vert[vertNum][1];
    z = vert[vertNum][2];
    return true;
  }
}


bool Trimesh::setVertex(int vertNum,double x,double y,double z){
  if(vertNum >= numVerts || vertNum < 0){
    fprintf(stderr,"Trimesh::error: vertNum=%d out of range [%d,%d)\n",vertNum,0,numVerts);
    return false;
  }
  else {
    vert[vertNum][0]=x;
    vert[vertNum][1]=y;
    vert[vertNum][2]=z;
    return true;
  }
}

bool Trimesh::setTriangle(int triNum,int vert0,int vert1,int vert2){
  if(triNum >=numTris || triNum < 0)
  {
    fprintf(stderr,"Trimesh::error: triNum=%d out of range [%d,%d)\n",triNum,0,numTris);
    return false;
  }
  else {
    tri[triNum][0]=vert0;
    tri[triNum][1]=vert1;
    tri[triNum][2]=vert2;
    return true;
  }
}

void Trimesh::finishDefining(){
  if(numVerts > 0 && numTris > 0){
    gstMesh=new gstPolyMesh(numVerts,vert,numTris,tri);

    if(NULL!=ourNode)
      ourNode->addChild(gstMesh);
  }
}

void Trimesh::addToScene(gstSeparator *ourScene){
  ourNode=ourScene;
  if(NULL!=gstMesh)
    ourNode->addChild(gstMesh);
}

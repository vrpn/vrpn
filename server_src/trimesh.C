#include "trimesh.h"

Trimesh::Trimesh(TrimeshType oT){
  ourType=oT;

  ourNode=NULL;
  gstMesh=NULL;
  inNode=false;
  ghostPolyMesh=NULL;

  // a bounding volume of a cubic meter should work, but is not very tight!
  minX=-500;
  minY=-500;
  minZ=-500;
  maxX=500;
  maxY=500;
  maxZ=500;
}

void Trimesh::clear(){

  if(NULL!=ourNode && inNode){
    ourNode->removeChild(gstMesh);
  }

  delete gstMesh;
  gstMesh=NULL;
  inNode=false;
  
  delete ghostPolyMesh;
  ghostPolyMesh=NULL;

}

// true if there is a mesh being displayed
bool Trimesh::displayStatus(){
  if(NULL!=ourNode && NULL!=gstMesh){
    return true;
  }
  else 
    return false;
}

// set the new bounding volume for this object, 
//     that is the smallest cube which will contain all the verts 
//     for the duration of the modifications
// for HCOLLIDE this should really be called before any of the below functions 
void Trimesh::setBoundingVolume(const float &nMinX,const float &nMinY,const float &nMinZ,
		       const float &nMaxX,const float &nMaxY,const float &nMaxZ){
  minX=nMinX;
  minY=nMinY;
  minZ=nMinZ;

  maxX=nMaxX;
  maxY=nMaxY;
  maxZ=nMaxZ;

}

bool Trimesh::getVertex(int vertNum,double &x,double &y,double &z){
  if(GHOST==ourType){
    gstVertex *curVert;
    if(NULL==ghostPolyMesh || NULL==(curVert=ghostPolyMesh->getVertex(vertNum)))
      return false;
    else {
      x = (*curVert)[0];
      y = (*curVert)[1];
      z = (*curVert)[2];
      return true;
    }
  }
  else if(HCOLLIDE==ourType){
    if(NULL==gstMesh)
      return false;
    else
      return ((gstHybridHashGridTriMesh *)gstMesh)->getVertex(vertNum,x,y,z);
  }
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}


bool Trimesh::setVertex(int vertNum,double x,double y,double z){

  if(GHOST==ourType){
    if(NULL==gstMesh){
      //gstMesh=new gstPolyMesh(numVerts,vert,numTris,tri);
      ghostPolyMesh=new gstTriPolyMesh();
      gstMesh=new gstTriPolyMeshHaptic(ghostPolyMesh);
    }
    
    gstVertex *curVert=ghostPolyMesh->getVertex(vertNum);
    if(NULL==curVert){
      ghostPolyMesh->createVertex(gstPoint(x,y,z),vertNum);
    }
    else {
      // the way this works with ghost is a bit awkward, maybe I'm not using it right (AG)
      // but we do only need to notify of one moved poly right????
      gstTriPoly *movedPoly=NULL;
      if(!curVert->isStranded()){
	movedPoly=(curVert->incidentEdgesBegin().node->data.getEdge()->
		   incidentPolysBegin().node->data);
	ghostPolyMesh->beginModify(movedPoly);
      }
      
      (*curVert)[0]=x;
      (*curVert)[1]=y;
      (*curVert)[2]=z;

      if(movedPoly){
	ghostPolyMesh->endModify(movedPoly);
      }
    }
    
    return true;
  }
  else if(HCOLLIDE==ourType){
    if(NULL==gstMesh){
      gstMesh=new gstHybridHashGridTriMesh(minX,minY,minZ,maxX,maxY,maxZ);      
    }
	bool retVal=true;
    if(x<minX || x>maxX || y<minY || y>maxY || z<minZ || z>maxZ){
      fprintf(stderr,"ERROR: vertex: <%lf,%lf,%lf> is outside the bounding volume\n",x,y,z);
	  retVal=false;
    }

    ((gstHybridHashGridTriMesh *)gstMesh)->setVertex(vertNum,x,y,z);
	return retVal;
  }
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}

bool Trimesh::setNormal(int,double,double,double){
  fprintf(stderr,"Trimesh::ERROR: norms no longer implemented\n");
  return false;
}

bool Trimesh::setTriangle(int triNum,int vert0,int vert1,int vert2,
			  int,int,int){
  if(GHOST==ourType){
    if(NULL==ghostPolyMesh)
      return false;
    
    gstTriPoly *curPoly=ghostPolyMesh->getPolygon(triNum);
    if(NULL!=curPoly)
      ghostPolyMesh->removePolygon(&curPoly);
    ghostPolyMesh->createTriPoly(vert0,vert1,vert2,triNum);
    return true;
  }
  else if(HCOLLIDE==ourType){
    if(NULL==gstMesh)
      return false;
    else {
      ((gstHybridHashGridTriMesh *)gstMesh)->setTriangle(triNum,vert0,vert1,vert2);
	  return true;
    }
  }
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}

bool Trimesh::removeTriangle(int triNum){
  if(GHOST==ourType){
    if(NULL==ghostPolyMesh)
      return false;
    
    gstTriPoly *curPoly=ghostPolyMesh->getPolygon(triNum);
    if(NULL!=curPoly){
      ghostPolyMesh->removePolygon(&curPoly);
      return true;
    }
    else 
      return false;
  }
  else if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->removeTriangle(triNum);
	return true;
  }
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}

bool Trimesh::updateChanges(){
  if(NULL==gstMesh)
    return false;

  if(!inNode && NULL!=ourNode){
    ourNode->addChild(gstMesh);
    inNode=true;
  }


  if(GHOST==ourType){
    if(NULL==ghostPolyMesh)
      return false;
    
    ghostPolyMesh->initSpatialPartition();
    ghostPolyMesh->endModifications();
    
    return true;
  }
  else if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->updateModifications();
    return true;
  }
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}

void Trimesh::addToScene(gstSeparator *ourScene){
  ourNode=ourScene;
  if(NULL!=gstMesh){
    ourNode->addChild(gstMesh);
    inNode=true;
  }
}







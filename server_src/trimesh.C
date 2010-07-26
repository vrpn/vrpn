#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "trimesh.h"

#ifndef	VRPN_USE_HDAPI

//pre: NULL==gstMesh; && NULL==ghostPolyMesh
void Trimesh::getGstMesh(){
  if(GHOST==ourType){
    ghostPolyMesh=new gstTriPolyMesh();
    gstMesh=new gstTriPolyMeshHaptic(ghostPolyMesh);
  }
#ifdef USING_HCOLLIDE
  else if(HCOLLIDE==ourType){
    gstMesh=new gstHybridHashGridTriMesh(minX,minY,minZ,maxX,maxY,maxZ);      
  }
#endif
  else {
    fprintf(stderr,"ERROR: unreckognized type: %d\n",ourType);
  }

  if(NULL!=gstMesh){
    // it appears that ghost prefers column major order
    gstMesh->setTransformMatrix(
				gstTransformMatrix(xFormMat[0],xFormMat[4],xFormMat[8],xFormMat[12],
						   xFormMat[1],xFormMat[5],xFormMat[9],xFormMat[13],
						   xFormMat[2],xFormMat[6],xFormMat[10],xFormMat[14],
						   xFormMat[3],xFormMat[7],xFormMat[11],xFormMat[15])); 
  }
}

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

  for(int i=1;i<16;i++)
    xFormMat[i]=0;
  xFormMat[0]=xFormMat[5]=xFormMat[10]=xFormMat[15]=1;

  scpWarn=normWarn=false;
}

void Trimesh::clear(){
  for(int i=1;i<16;i++)
    xFormMat[i]=0;
  xFormMat[0]=xFormMat[5]=xFormMat[10]=xFormMat[15]=1;

  if(NULL!=ourNode && inNode)
  	ourNode->removeChild(gstMesh);
  

  delete gstMesh;
  gstMesh=NULL;
  inNode=false;
  
  delete ghostPolyMesh;
  ghostPolyMesh=NULL;

  // return ourType back to the default
  setType(GHOST);
}

// true if there is a mesh being displayed
bool Trimesh::displayStatus(){
  if(NULL!=ourNode && NULL!=gstMesh){
    return true;
  }
  else 
    return false;
}

void Trimesh::startRecording(){
  if(NULL==gstMesh)
    return;
#ifdef MONITOR_TRIMESH
  else if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->startRecordSession();
  }
#endif
  else 
    fprintf(stderr,
	    "trimesh force interaction recording is not supported in this compile of this server\n");
}

void Trimesh::stopRecording(){
  if(NULL==gstMesh)
    return;
#ifdef MONITOR_TRIMESH
  else if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->stopRecordSession();
    return;
  }
#endif
  else 
    fprintf(stderr,
	    "trimesh force interaction recording is not supported in this compile of this server\n");
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

bool Trimesh::getVertex(int vertNum,float &x,float &y,float &z){
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
#ifdef USING_HCOLLIDE
  else if(HCOLLIDE==ourType){
    if(NULL==gstMesh)
      return false;
    else 
      return ((gstHybridHashGridTriMesh *)gstMesh)->getVertex(vertNum,x,y,z);
  }
#endif
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}

// if HCOLLIDE = ourType, returns the id of the triangle the probe is contacting
//    otherwise it returns -1
int Trimesh::getScpTriId(){
#ifdef USING_HCOLLIDE
  if(HCOLLIDE==ourType){
    if(NULL==gstMesh)
      return -1;
    else
      return ((gstHybridHashGridTriMesh *)gstMesh)->getScpTriId();
  } 
#endif

  // this is not really an error since scp triId is not necessary for haptic display
  if(!scpWarn){
    /* temp disaple:
    fprintf(stderr,
	    "Trimesh::WARNING: getScpId is not implemented for the current primitive type\n");
	    */
    scpWarn=true;
  }
  return true;

}

bool Trimesh::setVertex(int vertNum,double x,double y,double z){
  if(NULL==gstMesh)
    getGstMesh();

  if(GHOST==ourType){
    
    gstVertex *curVert=ghostPolyMesh->getVertex(vertNum);
    if(NULL==curVert){
      ghostPolyMesh->createVertex(gstPoint(x,y,z),vertNum);
    }
    else {
      // the way this works with ghost is a bit awkward, maybe I'm not using it right (AG)
      // but we do only need to notify of one moved poly right????
      gstTriPoly *movedPoly=NULL;
      if(!curVert->isStranded()){
	movedPoly=(curVert->incidentEdgesBegin()._M_node->_M_data.getEdge()->
		  incidentPolysBegin()._M_node->_M_data);
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
#ifdef USING_HCOLLIDE
  else if(HCOLLIDE==ourType){
    bool retVal=true;
    if(x<minX || x>maxX || y<minY || y>maxY || z<minZ || z>maxZ){
      fprintf(stderr,"ERROR: vertex: <%lf,%lf,%lf> is outside the bounding volume\n",x,y,z);
      retVal=false;
    }
    
    ((gstHybridHashGridTriMesh *)gstMesh)->setVertex(vertNum,x,y,z);
    return retVal;
  }
#endif
  else {
    fprintf(stderr,"ERROR: ourType is not valid\n");
    return false;
  }
}

bool Trimesh::setNormal(int normNum,double x,double y,double z){
  if(NULL==gstMesh)
    getGstMesh();

#ifdef USING_HCOLLIDE
  if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->setNormal(normNum,x,y,z);
    return true;
  }
#endif
  // this is not really an error since norms are not necessary for haptic display
  if(!normWarn){
    fprintf(stderr,"Trimesh::WARNING: norms not implemented for the current primitive type\n");
    normWarn=true;
  }
  return true;
}

bool Trimesh::setTriangle(int triNum,int vert0,int vert1,int vert2,
			  int norm0,int norm1,int norm2){
  if(GHOST==ourType){
    if(NULL==ghostPolyMesh)
      return false;
    
    gstTriPoly *curPoly=ghostPolyMesh->getPolygon(triNum);
    if(NULL!=curPoly)
      ghostPolyMesh->removePolygon(&curPoly);
    ghostPolyMesh->createTriPoly(vert0,vert1,vert2,triNum);
    return true;
  }
#ifdef USING_HCOLLIDE
  else if(HCOLLIDE==ourType){
    if(NULL==gstMesh)
      return false;
    else {
      ((gstHybridHashGridTriMesh *)gstMesh)->setTriangle(triNum,vert0,vert1,vert2,
							 norm0,norm1,norm2);
	  return true;
    }
  }
#endif
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
#ifdef USING_HCOLLIDE
  else if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->removeTriangle(triNum);
	return true;
  }
#endif
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
#ifdef USING_HCOLLIDE
  else if(HCOLLIDE==ourType){
    ((gstHybridHashGridTriMesh *)gstMesh)->updateModifications();
    return true;
  }
#endif
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

// set the transformatrix for the mesh (xFormMat is in row major order)
void Trimesh::setTransformMatrix(float xfMat[16]){
 
  for(int i=0;i<16;i++)
    xFormMat[i]=xfMat[i];

  if(NULL!=gstMesh){
    // it appears that ghost prefers column major order
    gstMesh->setTransformMatrix(
				gstTransformMatrix(xFormMat[0],xFormMat[4],xFormMat[8],xFormMat[12],
						   xFormMat[1],xFormMat[5],xFormMat[9],xFormMat[13],
						   xFormMat[2],xFormMat[6],xFormMat[10],xFormMat[14],
						   xFormMat[3],xFormMat[7],xFormMat[11],xFormMat[15])); 
  }
}

void Trimesh::setTouchableFromBothSides(bool touch)
{
	if(touch)
		gstMesh->setTouchableFrom( gstTriPolyMeshHaptic:: RV_FRONT_AND_BACK );
	else
		gstMesh->setTouchableFrom( gstTriPolyMeshHaptic::RV_FRONT );
}


#endif	VRPN_USE_HDAPI
#endif

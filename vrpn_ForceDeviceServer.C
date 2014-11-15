// vrpn_ForceDeviceServer.cpp: implementation of the vrpn_ForceDeviceServer
// class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h> // for fprintf, stderr, NULL

#include "vrpn_Connection.h" // for vrpn_HANDLERPARAM, etc
#include "vrpn_ForceDeviceServer.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

vrpn_ForceDeviceServer::vrpn_ForceDeviceServer(const char *name,
                                               vrpn_Connection *c)
    : vrpn_ForceDevice(name, c)
{
    if (register_autodeleted_handler(addObject_message_id,
                                     handle_addObject_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(addObjectExScene_message_id,
                                     handle_addObjectExScene_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setVertex_message_id,
                                     handle_setVertex_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setNormal_message_id,
                                     handle_setNormal_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setTriangle_message_id,
                                     handle_setTriangle_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(removeTriangle_message_id,
                                     handle_removeTriangle_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(updateTrimeshChanges_message_id,
                                     handle_updateTrimeshChanges_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(transformTrimesh_message_id,
                                     handle_transformTrimesh_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setTrimeshType_message_id,
                                     handle_setTrimeshType_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setObjectPosition_message_id,
                                     handle_setObjectPosition_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setObjectOrientation_message_id,
                                     handle_setObjectOrientation_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setObjectScale_message_id,
                                     handle_setObjectScale_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(removeObject_message_id,
                                     handle_removeObject_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(moveToParent_message_id,
                                     handle_moveToParent_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setHapticOrigin_message_id,
                                     handle_setHapticOrigin_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setHapticScale_message_id,
                                     handle_setHapticScale_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setSceneOrigin_message_id,
                                     handle_setSceneOrigin_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(setObjectIsTouchable_message_id,
                                     handle_setObjectIsTouchable_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
    if (register_autodeleted_handler(clearTrimesh_message_id,
                                     handle_clearTrimesh_message, this,
                                     vrpn_ForceDevice::d_sender_id)) {
        fprintf(stderr, "vrpn_Phantom:can't register handler\n");
        vrpn_ForceDevice::d_connection = NULL;
    }
}

vrpn_ForceDeviceServer::~vrpn_ForceDeviceServer() {}

int vrpn_ForceDeviceServer::handle_addObject_message(void *userdata,
                                                     vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum, parentNum;
    decode_addObject(p.buffer, p.payload_len, &objNum, &parentNum);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    if (me->addObject(objNum, parentNum)) {
        return 0;
    }
    else {
        fprintf(stderr, "vrpn_Phantom: error in trimesh::addObject\n");
        return -1;
    }
#endif
}

int vrpn_ForceDeviceServer::handle_addObjectExScene_message(void *userdata,
                                                            vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum;
    decode_addObjectExScene(p.buffer, p.payload_len, &objNum);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    if (me->addObjectExScene(objNum)) {
        return 0;
    }
    else {
        fprintf(stderr, "vrpn_Phantom: error in trimesh::addObjectExScene\n");
        return -1;
    }
#endif
}

int vrpn_ForceDeviceServer::handle_setVertex_message(void *userdata,
                                                     vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 temp;
    vrpn_int32 objNum;
    int vertNum;
    float x, y, z;
    decode_vertex(p.buffer, p.payload_len, &objNum, &temp, &x, &y, &z);
    vertNum = temp;

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    if (me->setVertex(objNum, vertNum, x, y, z)) {
        return 0;
    }
    else {
        fprintf(stderr, "vrpn_Phantom: error in trimesh::setVertex\n");
        return -1;
    }
#endif
}

int vrpn_ForceDeviceServer::handle_setNormal_message(void *userdata,
                                                     vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 temp;
    vrpn_int32 objNum;
    int normNum;
    float x, y, z;

    decode_normal(p.buffer, p.payload_len, &objNum, &temp, &x, &y, &z);
    normNum = temp;

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    if (me->setNormal(objNum, normNum, x, y, z))
        return 0;
    else {
        fprintf(stderr, "vrpn_Phantom: error in trimesh::setNormal\n");
        return -1;
    }
#endif
}

int vrpn_ForceDeviceServer::handle_setTriangle_message(void *userdata,
                                                       vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 triNum, v0, v1, v2, n0, n1, n2;
    vrpn_int32 objNum;
    decode_triangle(p.buffer, p.payload_len, &objNum, &triNum, &v0, &v1, &v2,
                    &n0, &n1, &n2);
#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    if (me->setTriangle(objNum, triNum, v0, v1, v2, n0, n1, n2))
        return 0;
    else {
        fprintf(stderr, "vrpn_Phantom: error in trimesh::setTriangle\n");
        return -1;
    }
#endif
}

int vrpn_ForceDeviceServer::handle_removeTriangle_message(void *userdata,
                                                          vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 triNum;
    vrpn_int32 objNum;

    decode_removeTriangle(p.buffer, p.payload_len, &objNum, &triNum);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    if (me->removeTriangle(objNum, triNum))
        return 0;
    else {
        fprintf(stderr, "vrpn_Phantom: error in trimesh::removeTriangle\n");
        return -1;
    }
#endif
}

int vrpn_ForceDeviceServer::handle_updateTrimeshChanges_message(
    void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;
    float SurfaceKspring, SurfaceKdamping, SurfaceFdynamic, SurfaceFstatic;
    vrpn_int32 objNum;

    decode_updateTrimeshChanges(p.buffer, p.payload_len, &objNum,
                                &SurfaceKspring, &SurfaceKdamping,
                                &SurfaceFdynamic, &SurfaceFstatic);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->updateTrimeshChanges(objNum, SurfaceKspring, SurfaceFstatic,
                             SurfaceFdynamic, SurfaceKdamping);

    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_setTrimeshType_message(void *userdata,
                                                          vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 temp;
    vrpn_int32 objNum;

    decode_setTrimeshType(p.buffer, p.payload_len, &objNum, &temp);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setTrimeshType(objNum, temp);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_clearTrimesh_message(void *userdata,
                                                        vrpn_HANDLERPARAM p)
{
    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum;

    decode_clearTrimesh(p.buffer, p.payload_len, &objNum);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->clearTrimesh(objNum);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_transformTrimesh_message(void *userdata,
                                                            vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    float xformMatrix[16];
    vrpn_int32 objNum;

    decode_trimeshTransform(p.buffer, p.payload_len, &objNum, xformMatrix);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setTrimeshTransform(objNum, xformMatrix);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_setObjectPosition_message(
    void *userdata, vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum;
    vrpn_float32 pos[3];

    decode_objectPosition(p.buffer, p.payload_len, &objNum, pos);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setObjectPosition(objNum, pos);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_setObjectOrientation_message(
    void *userdata, vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum;
    vrpn_float32 axis[3];
    vrpn_float32 angle;

    decode_objectOrientation(p.buffer, p.payload_len, &objNum, axis, &angle);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setObjectOrientation(objNum, axis, angle);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_setObjectScale_message(void *userdata,
                                                          vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_float32 scale[3];
    vrpn_int32 objNum;

    decode_objectScale(p.buffer, p.payload_len, &objNum, scale);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setObjectScale(objNum, scale);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_removeObject_message(void *userdata,
                                                        vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum;

    decode_removeObject(p.buffer, p.payload_len, &objNum);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->removeObject(objNum);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_moveToParent_message(void *userdata,
                                                        vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_int32 objNum, parentNum;

    decode_moveToParent(p.buffer, p.payload_len, &objNum, &parentNum);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->moveToParent(objNum, parentNum);
    return 0;
#endif
}

int vrpn_ForceDeviceServer::handle_setHapticOrigin_message(void *userdata,
                                                           vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_float32 pos[3];
    vrpn_float32 axis[3], angle;

    decode_setHapticOrigin(p.buffer, p.payload_len, pos, axis, &angle);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setHapticOrigin(pos, axis, angle);
    return 0;
#endif
}
int vrpn_ForceDeviceServer::handle_setHapticScale_message(void *userdata,
                                                          vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_float32 scale;

    decode_setHapticScale(p.buffer, p.payload_len, &scale);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setHapticScale(scale);
    return 0;
#endif
}
int vrpn_ForceDeviceServer::handle_setSceneOrigin_message(void *userdata,
                                                          vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_float32 pos[3];
    vrpn_float32 axis[3], angle;

    decode_setSceneOrigin(p.buffer, p.payload_len, pos, axis, &angle);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setSceneOrigin(pos, axis, angle);
    return 0;
#endif
}
int vrpn_ForceDeviceServer::handle_setObjectIsTouchable_message(
    void *userdata, vrpn_HANDLERPARAM p)
{

    vrpn_ForceDeviceServer *me = (vrpn_ForceDeviceServer *)userdata;

    vrpn_bool touch;
    vrpn_int32 objNum;

    decode_setObjectIsTouchable(p.buffer, p.payload_len, &objNum, &touch);

#ifdef VRPN_USE_HDAPI
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    me->send_text_message("Trimesh not supported under HDAPI", now,
                          vrpn_TEXT_ERROR);
    return 0;
#else
    me->setObjectIsTouchable(objNum, touch);
    return 0;
#endif
}

#include <stdio.h>  // for fprintf, stderr, printf
#include <string.h> // for strlen

#include "vrpn_Connection.h" // for vrpn_HANDLERPARAM, etc
#include "vrpn_Sound.h"

// vrpn_Sound constructor.
vrpn_Sound::vrpn_Sound(const char *name, vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
{
    vrpn_BaseClass::init();
}

int vrpn_Sound::register_types(void)
{
    load_sound_local =
        d_connection->register_message_type("vrpn_Sound Load_Local");
    load_sound_remote =
        d_connection->register_message_type("vrpn_Sound Load_Remote");
    unload_sound = d_connection->register_message_type("vrpn_Sound Unload");
    play_sound = d_connection->register_message_type("vrpn_Sound Play");
    stop_sound = d_connection->register_message_type("vrpn_Sound Stop");
    change_sound_status =
        d_connection->register_message_type("vrpn_Sound Status");

    set_listener_pose =
        d_connection->register_message_type("vrpn_Sound Listener_Pose");
    set_listener_velocity =
        d_connection->register_message_type("vrpn_Sound Listener_Velocity");

    set_sound_pose = d_connection->register_message_type("vrpn_Sound Pose");
    set_sound_velocity =
        d_connection->register_message_type("vrpn_Sound Velocity");
    set_sound_distanceinfo =
        d_connection->register_message_type("vrpn_Sound DistInfo");
    set_sound_coneinfo =
        d_connection->register_message_type("vrpn_Sound ConeInfo");

    set_sound_doplerfactor =
        d_connection->register_message_type("vrpn_Sound DopFac");
    set_sound_eqvalue = d_connection->register_message_type("vrpn_Sound EqVal");
    set_sound_pitch = d_connection->register_message_type("vrpn_Sound Pitch");
    set_sound_volume = d_connection->register_message_type("vrpn_Sound Volume");

    load_model_local =
        d_connection->register_message_type("vrpn_Sound Load_Model_Local");
    load_model_remote =
        d_connection->register_message_type("vrpn_Sound Load_Model_Remote");
    load_polyquad =
        d_connection->register_message_type("vrpn_Sound Load_Poly_Quad");
    load_polytri =
        d_connection->register_message_type("vrpn_Sound Load_Poly_Tri");
    load_material =
        d_connection->register_message_type("vrpn_Sound Load_Material");
    set_polyquad_vertices =
        d_connection->register_message_type("vrpn_Sound Quad_Vertices");
    set_polytri_vertices =
        d_connection->register_message_type("vrpn_Sound Tri_Vertices");
    set_poly_openingfactor =
        d_connection->register_message_type("vrpn_Sound Poly_OF");
    set_poly_material =
        d_connection->register_message_type("vrpn_Sound Poly_Material");

    return 0;
}

vrpn_Sound::~vrpn_Sound() {}

vrpn_int32 vrpn_Sound::encodeSound_local(const char *filename,
                                         const vrpn_SoundID id,
                                         const vrpn_SoundDef sound, char **buf)
{
    vrpn_int32 len = static_cast<vrpn_int32>(
        sizeof(vrpn_SoundID) + strlen(filename) + sizeof(vrpn_SoundDef) + 1);
    vrpn_int32 ret = len;
    char *mptr;
    int i;

    *buf = NULL;
    try { *buf = new char[len]; }
    catch (...) {
      fprintf(stderr, "vrpn_Sound::encodeSound_local(): Out of memory.\n");
      return 0;
    }

    mptr = *buf;
    vrpn_buffer(&mptr, &len, id);

    for (i = 0; i < 3; i++)
        vrpn_buffer(&mptr, &len, sound.pose.position[i]);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, sound.pose.orientation[i]);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, sound.velocity[i]);

    vrpn_buffer(&mptr, &len, sound.volume);

    vrpn_buffer(&mptr, &len, sound.max_back_dist);
    vrpn_buffer(&mptr, &len, sound.min_back_dist);
    vrpn_buffer(&mptr, &len, sound.max_front_dist);
    vrpn_buffer(&mptr, &len, sound.min_front_dist);

    vrpn_buffer(&mptr, &len, sound.cone_inner_angle);
    vrpn_buffer(&mptr, &len, sound.cone_outer_angle);
    vrpn_buffer(&mptr, &len, sound.cone_gain);
    vrpn_buffer(&mptr, &len, sound.dopler_scale);
    vrpn_buffer(&mptr, &len, sound.equalization_val);
    vrpn_buffer(&mptr, &len, sound.pitch);

    vrpn_buffer(&mptr, &len, filename,
                static_cast<vrpn_int32>(strlen(filename)) + 1);

    return ret;
}

// Decodes the file and Client Index number
vrpn_int32 vrpn_Sound::decodeSound_local(const char *buf, char **filename,
                                         vrpn_SoundID *id, vrpn_SoundDef *sound,
                                         const int payload)
{
    const char *mptr = buf;
    int i;

    *filename = NULL;
    try { *filename =
        new char[payload - sizeof(vrpn_SoundID) - sizeof(vrpn_SoundDef)]; }
    catch (...) {
      fprintf(stderr, "vrpn_Sound::decodeSound_local(): Out of memory.\n");
      return -1;
    }

    vrpn_unbuffer(&mptr, id);

    for (i = 0; i < 3; i++)
        vrpn_unbuffer(&mptr, &(sound->pose.position[i]));

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &(sound->pose.orientation[i]));

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &(sound->velocity[i]));

    vrpn_unbuffer(&mptr, &(sound->volume));

    vrpn_unbuffer(&mptr, &(sound->max_back_dist));
    vrpn_unbuffer(&mptr, &(sound->min_back_dist));
    vrpn_unbuffer(&mptr, &(sound->max_front_dist));
    vrpn_unbuffer(&mptr, &(sound->min_front_dist));

    vrpn_unbuffer(&mptr, &(sound->cone_inner_angle));
    vrpn_unbuffer(&mptr, &(sound->cone_outer_angle));
    vrpn_unbuffer(&mptr, &(sound->cone_gain));
    vrpn_unbuffer(&mptr, &(sound->dopler_scale));
    vrpn_unbuffer(&mptr, &(sound->equalization_val));
    vrpn_unbuffer(&mptr, &(sound->pitch));

    vrpn_unbuffer(&mptr, *filename,
                  payload - sizeof(vrpn_SoundID) - sizeof(vrpn_SoundDef));

    return 0;
}

/// @todo not supported
vrpn_int32 vrpn_Sound::encodeSound_remote(const char * /*filename*/,
                                          const vrpn_SoundID /*id*/,
                                          char ** /*buf*/)
{
    return 0;
}
/// @todo not supported yet
vrpn_int32 vrpn_Sound::decodeSound_remote(const char * /*buf*/,
                                          char ** /*filename*/,
                                          vrpn_SoundID * /*id*/,
                                          const int /*payload*/)
{
    return 0;
}

// Encodes the client sound ID
vrpn_int32 vrpn_Sound::encodeSoundID(const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID);

    vrpn_buffer(&mptr, &len, id);

    return (sizeof(vrpn_SoundID));
}

// Decodes the client sound ID
vrpn_int32 vrpn_Sound::decodeSoundID(const char *buf, vrpn_SoundID *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);

    return 0;
}

// Sends all the information necessary to play a sound appropriately.
// IE, The sounds position, orientation, velocity, volume and repeat count
vrpn_int32 vrpn_Sound::encodeSoundDef(const vrpn_SoundDef sound,
                                      const vrpn_SoundID id,
                                      const vrpn_int32 repeat, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len =
        sizeof(vrpn_SoundDef) + sizeof(vrpn_SoundID) + sizeof(vrpn_int32);
    vrpn_int32 ret = len;
    int i;

    vrpn_buffer(&mptr, &len, repeat);
    vrpn_buffer(&mptr, &len, id);

    for (i = 0; i < 3; i++)
        vrpn_buffer(&mptr, &len, sound.pose.position[i]);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, sound.pose.orientation[i]);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, sound.velocity[i]);

    vrpn_buffer(&mptr, &len, sound.volume);

    vrpn_buffer(&mptr, &len, sound.max_back_dist);
    vrpn_buffer(&mptr, &len, sound.min_back_dist);
    vrpn_buffer(&mptr, &len, sound.max_front_dist);
    vrpn_buffer(&mptr, &len, sound.min_front_dist);

    vrpn_buffer(&mptr, &len, sound.cone_inner_angle);
    vrpn_buffer(&mptr, &len, sound.cone_outer_angle);
    vrpn_buffer(&mptr, &len, sound.cone_gain);
    vrpn_buffer(&mptr, &len, sound.dopler_scale);
    vrpn_buffer(&mptr, &len, sound.equalization_val);
    vrpn_buffer(&mptr, &len, sound.pitch);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundDef(const char *buf, vrpn_SoundDef *sound,
                                      vrpn_SoundID *id, vrpn_int32 *repeat)
{
    const char *mptr = buf;
    int i;

    vrpn_unbuffer(&mptr, repeat);
    vrpn_unbuffer(&mptr, id);

    for (i = 0; i < 3; i++)
        vrpn_unbuffer(&mptr, &(sound->pose.position[i]));

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &(sound->pose.orientation[i]));

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &(sound->velocity[i]));

    vrpn_unbuffer(&mptr, &(sound->volume));

    vrpn_unbuffer(&mptr, &(sound->max_back_dist));
    vrpn_unbuffer(&mptr, &(sound->min_back_dist));
    vrpn_unbuffer(&mptr, &(sound->max_front_dist));
    vrpn_unbuffer(&mptr, &(sound->min_front_dist));

    vrpn_unbuffer(&mptr, &(sound->cone_inner_angle));
    vrpn_unbuffer(&mptr, &(sound->cone_outer_angle));
    vrpn_unbuffer(&mptr, &(sound->cone_gain));
    vrpn_unbuffer(&mptr, &(sound->dopler_scale));
    vrpn_unbuffer(&mptr, &(sound->equalization_val));
    vrpn_unbuffer(&mptr, &(sound->pitch));

    return 0;
}

// Sends information about the listener. IE  position, orientation and velocity
vrpn_int32 vrpn_Sound::encodeListenerPose(const vrpn_PoseDef pose, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_ListenerDef);
    vrpn_int32 ret = len;
    int i;

    for (i = 0; i < 3; i++)
        vrpn_buffer(&mptr, &len, pose.position[i]);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, pose.orientation[i]);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeListenerPose(const char *buf, vrpn_PoseDef *pose)
{
    const char *mptr = buf;
    int i;

    for (i = 0; i < 3; i++)
        vrpn_unbuffer(&mptr, &pose->position[i]);

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &pose->orientation[i]);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundPlay(const vrpn_SoundID id,
                                       const vrpn_int32 repeat, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_int32);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, repeat);
    vrpn_buffer(&mptr, &len, id);
    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundPlay(const char *buf, vrpn_SoundID *id,
                                       vrpn_int32 *repeat)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, repeat);
    vrpn_unbuffer(&mptr, id);
    return 0;
}

vrpn_int32 vrpn_Sound::encodeListenerVelocity(const vrpn_float64 *velocity,
                                              char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_float64) * 4;
    vrpn_int32 ret = len;
    int i;

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, velocity[i]);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeListenerVelocity(const char *buf,
                                              vrpn_float64 *velocity)
{
    const char *mptr = buf;

    for (int i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &velocity[i]);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundPose(const vrpn_PoseDef pose,
                                       const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_PoseDef);
    vrpn_int32 ret = len;
    int i;

    vrpn_buffer(&mptr, &len, id);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, pose.orientation[i]);

    for (i = 0; i < 3; i++)
        vrpn_buffer(&mptr, &len, pose.position[i]);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundPose(const char *buf, vrpn_PoseDef *pose,
                                       vrpn_SoundID *id)
{
    const char *mptr = buf;
    int i;

    vrpn_unbuffer(&mptr, id);

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &pose->orientation[i]);

    for (i = 0; i < 3; i++)
        vrpn_unbuffer(&mptr, &pose->position[i]);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundVelocity(const vrpn_float64 *velocity,
                                           const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64) * 4;
    vrpn_int32 ret = len;
    int i;

    vrpn_buffer(&mptr, &len, id);

    for (i = 0; i < 4; i++)
        vrpn_buffer(&mptr, &len, velocity[i]);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundVelocity(const char *buf,
                                           vrpn_float64 *velocity,
                                           vrpn_SoundID *id)
{
    const char *mptr = buf;
    int i;

    vrpn_unbuffer(&mptr, id);

    for (i = 0; i < 4; i++)
        vrpn_unbuffer(&mptr, &velocity[i]);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundDistInfo(const vrpn_float64 min_back,
                                           const vrpn_float64 max_back,
                                           const vrpn_float64 min_front,
                                           const vrpn_float64 max_front,
                                           const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64) * 4;
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, min_back);
    vrpn_buffer(&mptr, &len, max_back);
    vrpn_buffer(&mptr, &len, min_front);
    vrpn_buffer(&mptr, &len, max_front);

    return ret;
}
vrpn_int32
vrpn_Sound::decodeSoundDistInfo(const char *buf, vrpn_float64 *min_back,
                                vrpn_float64 *max_back, vrpn_float64 *min_front,
                                vrpn_float64 *max_front, vrpn_SoundID *id)
{
    const char *mptr = buf;
    vrpn_unbuffer(&mptr, id);

    vrpn_unbuffer(&mptr, min_back);
    vrpn_unbuffer(&mptr, max_back);
    vrpn_unbuffer(&mptr, min_front);
    vrpn_unbuffer(&mptr, max_front);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundConeInfo(const vrpn_float64 cone_inner_angle,
                                           const vrpn_float64 cone_outer_angle,
                                           const vrpn_float64 cone_gain,
                                           const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64) * 3;
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, cone_inner_angle);
    vrpn_buffer(&mptr, &len, cone_outer_angle);
    vrpn_buffer(&mptr, &len, cone_gain);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundConeInfo(const char *buf,
                                           vrpn_float64 *cone_inner_angle,
                                           vrpn_float64 *cone_outer_angle,
                                           vrpn_float64 *cone_gain,
                                           vrpn_SoundID *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);

    vrpn_unbuffer(&mptr, cone_inner_angle);
    vrpn_unbuffer(&mptr, cone_outer_angle);
    vrpn_unbuffer(&mptr, cone_gain);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundDoplerScale(const vrpn_float64 doplerfactor,
                                              const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, doplerfactor);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundDoplerScale(const char *buf,
                                              vrpn_float64 *doplerfactor,
                                              vrpn_SoundID *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);
    vrpn_unbuffer(&mptr, doplerfactor);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundEqFactor(const vrpn_float64 eqfactor,
                                           const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, eqfactor);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundEqFactor(const char *buf,
                                           vrpn_float64 *eqfactor,
                                           vrpn_SoundID *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);

    vrpn_unbuffer(&mptr, eqfactor);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundPitch(const vrpn_float64 pitch,
                                        const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, pitch);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundPitch(const char *buf, vrpn_float64 *pitch,
                                        vrpn_SoundID *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);

    vrpn_unbuffer(&mptr, pitch);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSoundVolume(const vrpn_float64 volume,
                                         const vrpn_SoundID id, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, volume);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSoundVolume(const char *buf, vrpn_float64 *volume,
                                         vrpn_SoundID *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);
    vrpn_unbuffer(&mptr, volume);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeLoadModel_local(const char *filename, char **buf)
{
    vrpn_int32 len =
        static_cast<vrpn_int32>(sizeof(vrpn_SoundID) + strlen(filename) + 1);
    vrpn_int32 ret = len;
    char *mptr;

    *buf = NULL;
    try { *buf = new char[strlen(filename) + sizeof(vrpn_SoundID) + 1]; }
    catch (...) {
      fprintf(stderr, "vrpn_Sound::encodeLoadModel_local(): Out of memory.\n");
      return 0;
    }

    mptr = *buf;
    vrpn_buffer(&mptr, &len, filename,
                static_cast<vrpn_int32>(strlen(filename)) + 1);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeLoadModel_local(const char *buf, char **filename,
                                             const int payload)
{
    const char *mptr = buf;

    *filename = NULL;
    try { *filename = new char[payload - sizeof(vrpn_SoundID)]; }
    catch (...) {
      fprintf(stderr, "vrpn_Sound::decodeLoadModel_local(): Out of memory.\n");
      return -1;
    }

    vrpn_unbuffer(&mptr, *filename, payload - sizeof(vrpn_SoundID));

    return 0;
}

/// @todo Remote stuff not supported yet!
vrpn_int32 vrpn_Sound::encodeLoadModel_remote(const char * /*filename*/,
                                              char ** /*buf*/)
{
    return 0;
}
/// @todo Remote stuff not supported yet!
vrpn_int32 vrpn_Sound::decodeLoadModel_remote(const char * /*buf*/,
                                              char ** /*filename*/,
                                              const int /*payload*/)
{
    return 0;
}

vrpn_int32 vrpn_Sound::encodeLoadPolyQuad(const vrpn_QuadDef quad, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_QuadDef);
    vrpn_int32 ret = len;
    int i;

    vrpn_buffer(&mptr, &len, quad.subQuad);
    vrpn_buffer(&mptr, &len, quad.openingFactor);
    vrpn_buffer(&mptr, &len, quad.tag);
    for (i = 0; i < 4; i++)
        for (int j(0); j < 3; j++)
            vrpn_buffer(&mptr, &len, quad.vertices[i][j]);

    vrpn_buffer(&mptr, &len, quad.material_name, MAX_MATERIAL_NAME_LENGTH);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeLoadPolyQuad(const char *buf, vrpn_QuadDef *quad)
{
    const char *mptr = buf;
    int i;

    vrpn_unbuffer(&mptr, &quad->subQuad);
    vrpn_unbuffer(&mptr, &quad->openingFactor);
    vrpn_unbuffer(&mptr, &quad->tag);
    for (i = 0; i < 4; i++)
        for (int j(0); j < 3; j++)
            vrpn_unbuffer(&mptr, &quad->vertices[i][j]);
    vrpn_unbuffer(&mptr, quad->material_name, MAX_MATERIAL_NAME_LENGTH);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeLoadPolyTri(const vrpn_TriDef tri, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_int32) + sizeof(vrpn_TriDef);
    vrpn_int32 ret = len;
    int i;

    vrpn_buffer(&mptr, &len, tri.subTri);
    vrpn_buffer(&mptr, &len, tri.openingFactor);
    vrpn_buffer(&mptr, &len, tri.tag);
    for (i = 0; i < 3; i++)
        for (int j(0); j < 3; j++)
            vrpn_buffer(&mptr, &len, tri.vertices[i][j]);
    vrpn_buffer(&mptr, &len, tri.material_name, MAX_MATERIAL_NAME_LENGTH);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeLoadPolyTri(const char *buf, vrpn_TriDef *tri)
{
    const char *mptr = buf;
    int i;

    vrpn_unbuffer(&mptr, &tri->subTri);
    vrpn_unbuffer(&mptr, &tri->openingFactor);
    vrpn_unbuffer(&mptr, &tri->tag);
    for (i = 0; i < 3; i++)
        for (int j(0); j < 3; j++)
            vrpn_unbuffer(&mptr, &tri->vertices[i][j]);
    vrpn_unbuffer(&mptr, tri->material_name, MAX_MATERIAL_NAME_LENGTH);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeLoadMaterial(const vrpn_int32 id,
                                          const vrpn_MaterialDef material,
                                          char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_int32) + sizeof(vrpn_MaterialDef);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, id);

    vrpn_buffer(&mptr, &len, material.material_name, MAX_MATERIAL_NAME_LENGTH);
    vrpn_buffer(&mptr, &len, material.transmittance_gain);
    vrpn_buffer(&mptr, &len, material.transmittance_highfreq);
    vrpn_buffer(&mptr, &len, material.reflectance_gain);
    vrpn_buffer(&mptr, &len, material.reflectance_highfreq);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeLoadMaterial(const char *buf,
                                          vrpn_MaterialDef *material,
                                          vrpn_int32 *id)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, id);

    vrpn_unbuffer(&mptr, material->material_name, MAX_MATERIAL_NAME_LENGTH);
    vrpn_unbuffer(&mptr, &material->transmittance_gain);
    vrpn_unbuffer(&mptr, &material->transmittance_highfreq);
    vrpn_unbuffer(&mptr, &material->reflectance_gain);
    vrpn_unbuffer(&mptr, &material->reflectance_highfreq);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSetQuadVert(const vrpn_float64 vertices[4][3],
                                         const vrpn_int32 tag, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_int32) + sizeof(vrpn_float64) * 12;
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, tag);

    for (int i = 0; i < 4; i++)
        for (int j(0); j < 3; j++)
            vrpn_buffer(&mptr, &len, vertices[i][j]);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSetQuadVert(const char *buf,
                                         vrpn_float64 (*vertices)[4][3],
                                         vrpn_int32 *tag)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, tag);

    for (int i = 0; i < 4; i++)
        for (int j(0); j < 3; j++)
            vrpn_unbuffer(&mptr, vertices[i][j]);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSetTriVert(const vrpn_float64 vertices[4][3],
                                        const vrpn_int32 tag, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_int32) + sizeof(vrpn_float64) * 9;
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, tag);

    for (int i = 0; i < 3; i++)
        for (int j(0); j < 3; j++)
            vrpn_buffer(&mptr, &len, vertices[i][j]);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSetTriVert(const char *buf,
                                        vrpn_float64 (*vertices)[3][3],
                                        vrpn_int32 *tag)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, tag);

    for (int i = 0; i < 3; i++)
        for (int j(0); j < 3; j++)
            vrpn_unbuffer(&mptr, vertices[i][j]);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSetPolyOF(const vrpn_float64 openingfactor,
                                       const vrpn_int32 tag, char *buf)
{
    char *mptr = buf;
    vrpn_int32 len = sizeof(vrpn_SoundID) + sizeof(vrpn_float64);
    vrpn_int32 ret = len;

    vrpn_buffer(&mptr, &len, tag);

    vrpn_buffer(&mptr, &len, openingfactor);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSetPolyOF(const char *buf,
                                       vrpn_float64 *openingfactor,
                                       vrpn_int32 *tag)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, tag);

    vrpn_unbuffer(&mptr, openingfactor);

    return 0;
}

vrpn_int32 vrpn_Sound::encodeSetPolyMaterial(const char *material,
                                             const vrpn_int32 tag, char *buf)
{
    vrpn_int32 len = sizeof(vrpn_SoundID) + 128;
    vrpn_int32 ret = len;
    char *mptr = buf;

    vrpn_buffer(&mptr, &len, tag);
    vrpn_buffer(&mptr, &len, material, 128);

    return ret;
}

vrpn_int32 vrpn_Sound::decodeSetPolyMaterial(const char *buf, char **material,
                                             vrpn_int32 *tag, const int)
{
    const char *mptr = buf;

    vrpn_unbuffer(&mptr, tag);
    vrpn_unbuffer(&mptr, *material, 128);

    return 0;
}

/********************************************************************************************
 Begin vrpn_Sound_Client
 *******************************************************************************************/
vrpn_Sound_Client::vrpn_Sound_Client(const char *name, vrpn_Connection *c)
    : vrpn_Sound(name, c)
    , vrpn_Text_Receiver(name, c)
{
    vrpn_Text_Receiver::register_message_handler(this,
                                                 handle_receiveTextMessage);
}

vrpn_Sound_Client::~vrpn_Sound_Client() {}

/*Sends a play sound message to the server.  Sends the id of the sound to play
  and the repeat value.  If 0 then it plays it continuously*/
vrpn_int32 vrpn_Sound_Client::playSound(const vrpn_SoundID id,
                                        vrpn_int32 repeat)
{

    char buf[sizeof(vrpn_SoundID) + sizeof(vrpn_int32)];
    vrpn_int32 len;

    len = encodeSoundPlay(id, repeat, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, play_sound,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message play: tossing\n");

    return 0;
}

/*Stops a playing sound*/
vrpn_int32 vrpn_Sound_Client::stopSound(const vrpn_SoundID id)
{
    char buf[sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundID(id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, stop_sound,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message play: tossing\n");

    return 0;
}

/*Loads a sound file on the server machine for playing.  Returns a vrpn_SoundID
  to
  be used to refer to that sound from now on*/
vrpn_SoundID vrpn_Sound_Client::loadSound(const char *sound,
                                          const vrpn_SoundID id,
                                          const vrpn_SoundDef soundDef)
{
    vrpn_int32 len;
    char *buf;

    len = encodeSound_local(sound, id, soundDef, &buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, load_sound_local,
        d_sender_id, buf,
        vrpn_CONNECTION_RELIABLE)) {
      fprintf(stderr,
        "vrpn_Sound_Client: cannot write message load: tossing\n");
    }

    try {
      delete[] buf;
    } catch (...) {
      fprintf(stderr, "vrpn_Sound_Client::loadSound(): delete failed\n");
      return -1;
    }
    return id;
}

/*Unloads a sound file on the server side*/
vrpn_int32 vrpn_Sound_Client::unloadSound(const vrpn_SoundID id)
{
    vrpn_int32 len;
    char buf[sizeof(vrpn_SoundID)];

    len = encodeSoundID(id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, unload_sound,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message unload: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundVolume(const vrpn_SoundID id,
                                             const vrpn_float64 volume)
{
    char buf[sizeof(vrpn_SoundID) + sizeof(vrpn_float64)];
    vrpn_int32 len;

    len = encodeSoundVolume(volume, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, set_sound_volume,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundPose(const vrpn_SoundID id,
                                           vrpn_float64 position[3],
                                           vrpn_float64 orientation[4])
{
    char buf[sizeof(vrpn_PoseDef) + sizeof(vrpn_SoundID)];
    vrpn_int32 len;
    vrpn_PoseDef tempdef;
    int i;
    for (i = 0; i < 4; i++) {
        tempdef.orientation[i] = orientation[i];
    }
    for (i = 0; i < 3; i++) {
        tempdef.position[i] = position[i];
    }

    len = encodeSoundPose(tempdef, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, set_sound_pose,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE)) {
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");
    }

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundVelocity(const vrpn_SoundID id,
                                               const vrpn_float64 velocity[4])
{
    char buf[sizeof(vrpn_float64) * 4 + sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundVelocity(velocity, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp,
                                               set_sound_velocity, d_sender_id,
                                               buf, vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundDistances(
    const vrpn_SoundID id, const vrpn_float64 max_front_dist,
    const vrpn_float64 min_front_dist, const vrpn_float64 max_back_dist,
    const vrpn_float64 min_back_dist)
{
    char buf[sizeof(vrpn_float64) * 4 + sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundDistInfo(max_front_dist, min_front_dist, max_back_dist,
                              min_back_dist, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(
            len, timestamp, set_sound_distanceinfo, d_sender_id, buf,
            vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundConeInfo(const vrpn_SoundID id,
                                               const vrpn_float64 inner_angle,
                                               const vrpn_float64 outer_angle,
                                               const vrpn_float64 gain)
{

    char buf[sizeof(vrpn_float64) * 3 + sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundConeInfo(inner_angle, outer_angle, gain, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp,
                                               set_sound_coneinfo, d_sender_id,
                                               buf, vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundDopScale(const vrpn_SoundID id,
                                               vrpn_float64 dopfactor)
{

    char buf[sizeof(vrpn_float64) + sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundDoplerScale(dopfactor, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(
            len, timestamp, set_sound_doplerfactor, d_sender_id, buf,
            vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundEqValue(const vrpn_SoundID id,
                                              vrpn_float64 eq_value)
{
    char buf[sizeof(vrpn_float64) + sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundEqFactor(eq_value, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp,
                                               set_sound_eqvalue, d_sender_id,
                                               buf, vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setSoundPitch(const vrpn_SoundID id,
                                            vrpn_float64 pitch)
{
    char buf[sizeof(vrpn_float64) + sizeof(vrpn_SoundID)];
    vrpn_int32 len;

    len = encodeSoundPitch(pitch, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, set_sound_pitch,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setListenerPose(const vrpn_float64 position[3],
                                              const vrpn_float64 orientation[4])
{
    char buf[sizeof(vrpn_PoseDef)];
    vrpn_int32 len;
    vrpn_PoseDef tempdef;
    int i;

    for (i = 0; i < 4; i++) {
        tempdef.orientation[i] = orientation[i];
    }
    for (i = 0; i < 3; i++) {
        tempdef.position[i] = position[i];
    }

    len = encodeListenerPose(tempdef, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp,
                                               set_listener_pose, d_sender_id,
                                               buf, vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32
vrpn_Sound_Client::setListenerVelocity(const vrpn_float64 velocity[4])
{
    char buf[sizeof(vrpn_float64) * 4];
    vrpn_int32 len;

    len = encodeListenerVelocity(velocity, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(
            len, timestamp, set_listener_velocity, d_sender_id, buf,
            vrpn_CONNECTION_RELIABLE)) {
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");
    }

    return 0;
}

vrpn_int32 vrpn_Sound_Client::LoadModel_local(const char *filename)
{
    vrpn_int32 len;
    char *buf;

    len = encodeLoadModel_local(filename, &buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, load_model_local,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message load: tossing\n");

    return 1;
}

/** @todo Remote stuff not supported yet!*/
vrpn_int32 vrpn_Sound_Client::LoadModel_remote(const char * /*data*/)
{
    return 0;
}

vrpn_int32 vrpn_Sound_Client::LoadPolyQuad(const vrpn_QuadDef quad)
{
    vrpn_int32 len;
    char buf[sizeof(vrpn_QuadDef)];

    len = encodeLoadPolyQuad(quad, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, load_polyquad,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message load: tossing\n");

    return quad.tag;
}

vrpn_int32 vrpn_Sound_Client::LoadPolyTri(const vrpn_TriDef tri)
{
    vrpn_int32 len;
    char buf[sizeof(vrpn_TriDef)];

    len = encodeLoadPolyTri(tri, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, load_polytri,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message load: tossing\n");

    return tri.tag;
}

vrpn_int32 vrpn_Sound_Client::LoadMaterial(const vrpn_int32 id,
                                           const vrpn_MaterialDef material)
{
    vrpn_int32 len;
    char buf[sizeof(vrpn_MaterialDef) + sizeof(vrpn_int32)];

    len = encodeLoadMaterial(id, material, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp, load_material,
                                               d_sender_id, buf,
                                               vrpn_CONNECTION_RELIABLE))
        fprintf(stderr,
                "vrpn_Sound_Client: cannot write message load: tossing\n");

    return id;
}

vrpn_int32 vrpn_Sound_Client::setPolyOF(const int id, const vrpn_float64 OF)
{
    char buf[sizeof(vrpn_int32) + sizeof(vrpn_float64)];
    vrpn_int32 len;

    len = encodeSetPolyOF(OF, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(
            len, timestamp, set_poly_openingfactor, d_sender_id, buf,
            vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setQuadVertices(const int id,
                                              const vrpn_float64 vertices[4][3])
{
    char buf[sizeof(vrpn_int32) + sizeof(vrpn_float64) * 12];
    vrpn_int32 len;

    len = encodeSetQuadVert(vertices, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(
            len, timestamp, set_polyquad_vertices, d_sender_id, buf,
            vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

vrpn_int32 vrpn_Sound_Client::setPolyMaterialName(const int id,
                                                  const char *material_name)
{
    char buf[sizeof(vrpn_int32) + sizeof(material_name)];
    vrpn_int32 len;

    len = encodeSetPolyMaterial(material_name, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(len, timestamp,
                                               set_poly_material, d_sender_id,
                                               buf, vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}
vrpn_int32 vrpn_Sound_Client::setTriVertices(const int id,
                                             const vrpn_float64 vertices[3][3])
{
    char buf[sizeof(vrpn_int32) + sizeof(vrpn_float64) * 9];
    vrpn_int32 len;

    len = encodeSetTriVert(vertices, id, buf);

    vrpn_gettimeofday(&timestamp, NULL);

    if (vrpn_Sound::d_connection->pack_message(
            len, timestamp, set_polytri_vertices, d_sender_id, buf,
            vrpn_CONNECTION_RELIABLE))
        fprintf(
            stderr,
            "vrpn_Sound_Client: cannot write message change status: tossing\n");

    return 0;
}

void vrpn_Sound_Client::mainloop()
{
    vrpn_Sound::d_connection->mainloop();
    client_mainloop();
}

void vrpn_Sound_Client::handle_receiveTextMessage(void *userdata,
                                                  const vrpn_TEXTCB t)
{
    vrpn_Sound_Client *me = (vrpn_Sound_Client *)userdata;
    me->receiveTextMessage(t.message, t.type, t.level, t.msg_time);
}

void vrpn_Sound_Client::receiveTextMessage(const char *message, vrpn_uint32,
                                           vrpn_uint32, struct timeval)
{
    printf("Virtual: %s\n", message);
    return;
}

/********************************************************************************************
 Begin vrpn_Sound_Server
 *******************************************************************************************/
#ifndef VRPN_CLIENT_ONLY
vrpn_Sound_Server::vrpn_Sound_Server(const char *name, vrpn_Connection *c)
    : vrpn_Sound(name, c)
    , vrpn_Text_Sender(name, c)
{
    /*Register the handlers*/
    register_autodeleted_handler(load_sound_local, handle_loadSoundLocal, this,
                                 d_sender_id);
    register_autodeleted_handler(load_sound_remote, handle_loadSoundRemote,
                                 this, d_sender_id);
    register_autodeleted_handler(unload_sound, handle_unloadSound, this,
                                 d_sender_id);
    register_autodeleted_handler(play_sound, handle_playSound, this,
                                 d_sender_id);
    register_autodeleted_handler(stop_sound, handle_stopSound, this,
                                 d_sender_id);
    register_autodeleted_handler(change_sound_status, handle_changeSoundStatus,
                                 this, d_sender_id);

    register_autodeleted_handler(set_listener_pose, handle_setListenerPose,
                                 this, d_sender_id);
    register_autodeleted_handler(set_listener_velocity,
                                 handle_setListenerVelocity, this, d_sender_id);

    register_autodeleted_handler(set_sound_pose, handle_setSoundPose, this,
                                 d_sender_id);
    register_autodeleted_handler(set_sound_velocity, handle_setSoundVelocity,
                                 this, d_sender_id);
    register_autodeleted_handler(
        set_sound_distanceinfo, handle_setSoundDistanceinfo, this, d_sender_id);
    register_autodeleted_handler(set_sound_coneinfo, handle_setSoundConeinfo,
                                 this, d_sender_id);

    register_autodeleted_handler(
        set_sound_doplerfactor, handle_setSoundDoplerfactor, this, d_sender_id);
    register_autodeleted_handler(set_sound_eqvalue, handle_setSoundEqvalue,
                                 this, d_sender_id);
    register_autodeleted_handler(set_sound_pitch, handle_setSoundPitch, this,
                                 d_sender_id);
    register_autodeleted_handler(set_sound_volume, handle_setSoundVolume, this,
                                 d_sender_id);

    register_autodeleted_handler(load_model_local, handle_loadModelLocal, this,
                                 d_sender_id);
    register_autodeleted_handler(load_model_remote, handle_loadModelRemote,
                                 this, d_sender_id);
    register_autodeleted_handler(load_polyquad, handle_loadPolyquad, this,
                                 d_sender_id);
    register_autodeleted_handler(load_polytri, handle_loadPolytri, this,
                                 d_sender_id);
    register_autodeleted_handler(load_material, handle_loadMaterial, this,
                                 d_sender_id);
    register_autodeleted_handler(set_polyquad_vertices,
                                 handle_setPolyquadVertices, this, d_sender_id);
    register_autodeleted_handler(set_polytri_vertices,
                                 handle_setPolytriVertices, this, d_sender_id);
    register_autodeleted_handler(
        set_poly_openingfactor, handle_setPolyOpeningfactor, this, d_sender_id);
    register_autodeleted_handler(set_poly_material, handle_setPolyMaterial,
                                 this, d_sender_id);
}

vrpn_Sound_Server::~vrpn_Sound_Server() {}

/******************************************************************************
 Callback Handler routines.
 ******************************************************************************/

int vrpn_Sound_Server::handle_loadSoundLocal(void *userdata,
                                             vrpn_HANDLERPARAM p)
{

    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_SoundID id;
    vrpn_SoundDef soundDef;
    char *filename;

    me->decodeSound_local(p.buffer, &filename, &id, &soundDef,
                          p.payload_len);
    me->loadSoundLocal(filename, id, soundDef);
    try {
      delete[] filename;
    } catch (...) {
      fprintf(stderr, "vrpn_Sound_Server::handle_loadSoundLocal(): delete failed\n");
      return -1;
    }
    return 0;
}

/* not supported */
int vrpn_Sound_Server::handle_loadSoundRemote(void *,
                                              vrpn_HANDLERPARAM)
{
    return 0;
}
int vrpn_Sound_Server::handle_playSound(void *userdata, vrpn_HANDLERPARAM p)
{

    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_SoundID id;
    vrpn_SoundDef soundDef;
    vrpn_int32 repeat;

    me->decodeSoundDef(p.buffer, &soundDef, &id, &repeat);
    me->playSound(id, repeat, soundDef);
    return 0;
}

int vrpn_Sound_Server::handle_stopSound(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_SoundID id;

    me->decodeSoundID(p.buffer, &id);
    me->stopSound(id);
    return 0;
}

int vrpn_Sound_Server::handle_unloadSound(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_SoundID id;

    me->decodeSoundID(p.buffer, &id);
    me->unloadSound(id);
    return 0;
}

int vrpn_Sound_Server::handle_changeSoundStatus(void *userdata,
                                                vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_SoundID id;
    vrpn_SoundDef soundDef;
    vrpn_int32 repeat;

    me->decodeSoundDef(p.buffer, &soundDef, &id, &repeat);
    me->changeSoundStatus(id, soundDef);
    return 0;
}

int vrpn_Sound_Server::handle_setListenerPose(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_PoseDef pose;

    me->decodeListenerPose(p.buffer, &pose);
    me->setListenerPose(pose);
    return 0;
}

int vrpn_Sound_Server::handle_setListenerVelocity(void *userdata,
                                                  vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 velocity[4];

    me->decodeListenerVelocity(p.buffer, velocity);
    me->setListenerVelocity(velocity);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundPose(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_PoseDef pose;
    vrpn_int32 id;

    me->decodeSoundPose(p.buffer, &pose, &id);
    me->setSoundPose(id, pose);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundVelocity(void *userdata,
                                               vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 velocity[4];
    vrpn_int32 id;

    me->decodeSoundVelocity(p.buffer, velocity, &id);
    me->setSoundVelocity(id, velocity);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundDistanceinfo(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 dist[4];
    vrpn_int32 id;
    /* order is min_back, max_back, min_front, max_front */
    me->decodeSoundDistInfo(p.buffer, &dist[0], &dist[1], &dist[2],
                            &dist[3], &id);
    me->setSoundDistInfo(id, dist);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundConeinfo(void *userdata,
                                               vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 cinfo[3];
    vrpn_int32 id;

    me->decodeSoundConeInfo(p.buffer, &cinfo[0], &cinfo[1], &cinfo[2],
                            &id);
    me->setSoundConeInfo(id, cinfo);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundDoplerfactor(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 df;
    vrpn_int32 id;

    me->decodeSoundDoplerScale(p.buffer, &df, &id);
    me->setSoundDoplerFactor(id, df);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundEqvalue(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 val;
    vrpn_int32 id;

    me->decodeSoundEqFactor(p.buffer, &val, &id);
    me->setSoundEqValue(id, val);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundPitch(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 pitch;
    vrpn_int32 id;

    me->decodeSoundPitch(p.buffer, &pitch, &id);
    me->setSoundPitch(id, pitch);
    return 0;
}

int vrpn_Sound_Server::handle_setSoundVolume(void *userdata,
                                             vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 vol;
    vrpn_int32 id;

    me->decodeSoundVolume(p.buffer, &vol, &id);
    me->setSoundVolume(id, vol);
    return 0;
}

int vrpn_Sound_Server::handle_loadModelLocal(void *userdata,
                                             vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    char *filename;

    me->decodeLoadModel_local(p.buffer, &filename, p.payload_len);
    me->loadModelLocal(filename);
    try {
      delete[] filename;
    } catch (...) {
      fprintf(stderr, "vrpn_Sound_Server::handle_loadModelLocal(): delete failed\n");
      return -1;
    }
    return 0;
}

/* not handled yet */
int vrpn_Sound_Server::handle_loadModelRemote(void *,
                                              vrpn_HANDLERPARAM)
{
    return 0;
}

int vrpn_Sound_Server::handle_loadPolyquad(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_QuadDef quad;

    me->decodeLoadPolyQuad(p.buffer, &quad);
    me->loadPolyQuad(&quad);
    return 0;
}

int vrpn_Sound_Server::handle_loadPolytri(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_TriDef tri;

    me->decodeLoadPolyTri(p.buffer, &tri);
    me->loadPolyTri(&tri);
    return 0;
}

int vrpn_Sound_Server::handle_loadMaterial(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_MaterialDef material;
    vrpn_int32 id;

    me->decodeLoadMaterial(p.buffer, &material, &id);

    me->loadMaterial(&material, id);
    return 0;
}

int vrpn_Sound_Server::handle_setPolyquadVertices(void *userdata,
                                                  vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64(*vertices)[4][3] = NULL;
    vrpn_int32 id;

    me->decodeSetQuadVert(p.buffer, vertices, &id);
    me->setPolyQuadVertices(*vertices, id);
    return 0;
}

int vrpn_Sound_Server::handle_setPolytriVertices(void *userdata,
                                                 vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64(*vertices)[3][3] = NULL;
    vrpn_int32 id;

    me->decodeSetTriVert(p.buffer, vertices, &id);
    me->setPolyTriVertices(*vertices, id);
    return 0;
}

int vrpn_Sound_Server::handle_setPolyOpeningfactor(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    vrpn_float64 of;
    vrpn_int32 id;

    me->decodeSetPolyOF(p.buffer, &of, &id);
    me->setPolyOF(of, id);
    return 0;
}

int vrpn_Sound_Server::handle_setPolyMaterial(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    vrpn_Sound_Server *me = (vrpn_Sound_Server *)userdata;
    char **material = NULL;
    vrpn_int32 id;

    me->decodeSetPolyMaterial(p.buffer, material, &id, p.payload_len);
    me->setPolyMaterial(*material, id);
    return 0;
}

#endif /*#ifndef VRPN_CLIENT_ONLY */

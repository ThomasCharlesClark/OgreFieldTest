
#ifndef _Mq_MqMessages_H_
#define _Mq_MqMessages_H_

#include <vector>
#include <assert.h>

namespace MyThirdOgre
{
    namespace Mq
    {
        enum MessageId
        {
            //Graphics <-  Logic
            LOGICFRAME_FINISHED,
            GAME_ENTITY_ADDED,
            GAME_ENTITY_REMOVED,
            //Graphics <-> Logic
            GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT,
            //Graphics  -> Logic
            SDL_EVENT,


            GAME_ENTITY_COLOUR_CHANGE,
            GAME_ENTITY_ALPHA_CHANGE,

            GAME_ENTITY_VISIBILITY_CHANGE,
        
            //...Leap -> Logic?
            LEAPFRAME_FINISHED,
            LEAPFRAME_VELOCITY,
            LEAPFRAME_MOTION,

            // ...? -> Graphics System <-- Compute! ^_^
            FIELD_COMPUTE_SYSTEM_WRITE_FILE_TESTING,
            FIELD_COMPUTE_SYSTEM_WRITE_VELOCITIES,
            FIELD_COMPUTE_JOB_REQUESTED,
            FIELD_COMPUTE_JOB_DESTROYED,

            REMOVE_STAGING_TEXTURE,

            NUM_MESSAGE_IDS,
        };
    }
}

#endif

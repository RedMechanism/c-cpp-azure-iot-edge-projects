#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "iothub_module_client_ll.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub.h"

static int callbackCounter;
static char msgText[1024];
static char propText[1024];
#define MESSAGE_COUNT 2
#define DOWORK_LOOP_NUM     60


typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EVENT_INSTANCE* eventInstance = (EVENT_INSTANCE*)userContextCallback;
    (void)printf("Confirmation[%d] received for message tracking id = %lu with result = %s\r\n", callbackCounter, (unsigned long)eventInstance->messageTrackingId, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

int main(void)
{
    
    IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;
    EVENT_INSTANCE messages[MESSAGE_COUNT];

    callbackCounter = 0;

    if (IoTHub_Init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
        return 1;
    }
    else if ((iotHubModuleClientHandle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol)) == NULL)
    {
        (void)printf("ERROR: iotHubModuleClientHandle is NULL!\r\n");
    }
    else
    {
        // Uncomment the following lines to enable verbose logging (e.g., for debugging).
        // bool traceOn = true;
        // IoTHubModuleClient_LL_SetOption(iotHubModuleClientHandle, OPTION_LOG_TRACE, &traceOn);

        size_t iterator = 0;

        do
        {
            if (iterator < MESSAGE_COUNT)
            {
                int address_id;
                char* shared_memory;
                struct shmid_ds shmbuffer;
                int segment_size;
                const int shared_segment_size = 0x6400;

                 // Allocate a shared memory segment.
                address_id = shmget (IPC_PRIVATE, shared_segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
                // Attach the shared memory segment. 
                printf("Shared memory segment ID is %d\n", address_id);
                shared_memory = (char*) shmat (address_id, 0, 0);
                printf ("shared memory attached at address %p\n", shared_memory);
                //Determine the segment's size.

                /*
                shmctl (address_id, IPC_STAT, &shmbuffer);
                segment_size  =               shmbuffer.shm_segsz;
                printf ("segment size: %d\n", segment_size);
                */

     
                //Write a string to the shared memory segment.
                sprintf (shared_memory, "Shared memory is working!");
                // Detach the shared memory segment.
                shmdt (shared_memory);

                // THe key is dispatched here
                sprintf_s(msgText, sizeof(msgText), "%d", address_id);

                if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromString(msgText)) == NULL)
                {
                    (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                }
                else
                {
                    (void)IoTHubMessage_SetMessageId(messages[iterator].messageHandle, "MSG_ID");
                    (void)IoTHubMessage_SetCorrelationId(messages[iterator].messageHandle, "CORE_ID");

                    messages[iterator].messageTrackingId = iterator;
                    MAP_HANDLE propMap = IoTHubMessage_Properties(messages[iterator].messageHandle);

                    if (IoTHubModuleClient_LL_SendEventToOutputAsync(iotHubModuleClientHandle, messages[iterator].messageHandle, "Output", SendConfirmationCallback, &messages[iterator]) != IOTHUB_CLIENT_OK)
                    {
                        (void)printf("ERROR: IoTHubModuleClient_LL_SendEventAsync..........FAILED!\r\n");
                    }
                    else
                    {
                        (void)printf("IoTHubModuleClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int)iterator);
                    }
                }
            }
            IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
            ThreadAPI_Sleep(1000);
            iterator++;
        } while (1);

        // Loop while we wait for messages to drain off.
        size_t index = 0;
        for (index = 0; index < DOWORK_LOOP_NUM; index++)
        {
            IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
            ThreadAPI_Sleep(100);
        }

        IoTHubModuleClient_LL_Destroy(iotHubModuleClientHandle);
    }

    (void)printf("Finished executing\n");
    IoTHub_Deinit();

    return 0;
}

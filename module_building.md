# Introduction to Building Azure IoT Edge Modules

Azure IoT Edge modules are the smallest unit of computation managed by IoT Edge.  The following guide will demonstrate how to create two types of modules in C, one that generates a 'Hello World' message to the module output and another that receives this message and then routes it to the IoT Hub. Before getting started make sure you have the [necessary prerequisites on Linux](https://docs.microsoft.com/en-us/azure/iot-edge/tutorial-develop-for-linux#prerequisites) or [Windows](https://docs.microsoft.com/en-us/azure/iot-edge/tutorial-develop-for-windows#prerequisites) specified by Microsoft.

### Module Identity and Twins
Before creating Azure IoT modules it is important to know what they are and how they work. Modules are any workload that can be deployed onto an edge device. They are containerized using docker and hence their associated Docker image can be hosted on any image repository such as Docker Hub or Microsoft's very own Azure Container Registry. Each time a module image is deployed to a device and started by the IoT Edge runtime, a new instance of that module is created. When a new module instance is created by the IoT Edge runtime, the instance is associated with a corresponding [module identity](https://docs.microsoft.com/en-us/azure/iot-edge/iot-edge-modules#module-identities). The module identity is stored in IoT Hub, and is employed as the addressing and security scope for all local and cloud communications for that specific module instance. Each module instance also has a corresponding [module twin](https://docs.microsoft.com/en-us/azure/iot-edge/iot-edge-modules#module-twins) in the form of a JSON document that can be used to configure the module instance.

### Deployment manifest
Before creating modules it is also important to have knowledge of what is known as the [Deployment Manifest](https://docs.microsoft.com/en-us/azure/iot-edge/module-composition#create-a-deployment-manifest). A deployment manifest is a JSON document that describes the modules to be configured on the targeted IoT Edge devices. It contains the configuration metadata for all the modules, including the required system modules i.e. $edgeAgent and $edgeHub. The module configuration metadata stored in the deployment manifest includes:

- Status (for example, running or stopped)
- Restart policy
- Routes for data input and output
- [Create options](createoptions.md)
- Image and container registry
- If the module image is stored in a private container registry, the IoT Edge agent holds the registry credentials.

A deployment manifest can be built by going through a wizard in the Azure IoT Edge portal. You can also apply a deployment manifest programmatically using REST or the IoT Hub Service SDK.


### Hello World :)
Once the module image has been created and the deployment manifest has been prepared, the modules are ready to be built. The easiest way to create modules outside the Azure portal is by using Visual Studio Code configured with the [Azure IoT Tools](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.azure-iot-tools). This tool will generate the deployment manifest based on all the modules you add to your particular edge device and will also handle everything else need to create and push edge modules such as management of credentials, target device specification, images etc. One can also use Azure IoT Hub SDKs with the module client to create modules.

The following code creates a C module that generates a message and the outputs this to the edge device's output steam.
```c
#include <stdio.h>
#include <stdlib.h>
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
static char propText[1024];#
define MESSAGE_COUNT 20# define DOWORK_LOOP_NUM 60

typedef struct EVENT_INSTANCE_TAG {
  IOTHUB_MESSAGE_HANDLE messageHandle;
  size_t messageTrackingId; // For tracking the messages within the user callback.
}
EVENT_INSTANCE;

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void * userContextCallback) {
  EVENT_INSTANCE * eventInstance = (EVENT_INSTANCE * ) userContextCallback;
  (void) printf("Confirmation[%d] received for message tracking id = %lu with result = %s\r\n", callbackCounter, (unsigned long) eventInstance - > messageTrackingId, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
  /* Some device specific action code goes here... */
  callbackCounter++;
  IoTHubMessage_Destroy(eventInstance - > messageHandle);
}

int main(void) {
  IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;
  EVENT_INSTANCE messages[MESSAGE_COUNT];

  callbackCounter = 0;

  if (IoTHub_Init() != 0) {
    (void) printf("Failed to initialize the platform.\r\n");
    return 1;
  } else if ((iotHubModuleClientHandle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol)) == NULL) {
    (void) printf("ERROR: iotHubModuleClientHandle is NULL!\r\n");
  } else {
    // Uncomment the following lines to enable verbose logging (e.g., for debugging).
    // bool traceOn = true;
    // IoTHubModuleClient_LL_SetOption(iotHubModuleClientHandle, OPTION_LOG_TRACE, &traceOn);

    size_t iterator = 0;

    do {
      if (iterator < MESSAGE_COUNT) {
        // THE MESSAGE IS GENERATED HERE!!
        sprintf_s(msgText, sizeof(msgText), "{\"Message from Device\":\"Hello World!\",\"Message Number\":%d}", iterator + 1);

        if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromString(msgText)) == NULL) {
          (void) printf("ERROR: iotHubMessageHandle is NULL!\r\n");
        } else {
          (void) IoTHubMessage_SetMessageId(messages[iterator].messageHandle, "MSG_ID");
          (void) IoTHubMessage_SetCorrelationId(messages[iterator].messageHandle, "CORE_ID");

          messages[iterator].messageTrackingId = iterator;
          MAP_HANDLE propMap = IoTHubMessage_Properties(messages[iterator].messageHandle);

          /*
          // Uncomment to set "applicationProperties" using conditional statements
          double Some_Condition = 30;
          (void)sprintf_s(propText, sizeof(propText), Some_Condition > 28 ? "true" : "false");
          Map_AddOrUpdate(propMap, "ApplicationProperty", propText);
          */

          if (IoTHubModuleClient_LL_SendEventToOutputAsync(iotHubModuleClientHandle, messages[iterator].messageHandle, "Output", SendConfirmationCallback, & messages[iterator]) != IOTHUB_CLIENT_OK) {
            (void) printf("ERROR: IoTHubModuleClient_LL_SendEventAsync..........FAILED!\r\n");
          } else {
            (void) printf("IoTHubModuleClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int) iterator);
          }
        }
      }
      IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
      ThreadAPI_Sleep(1000);
      iterator++;
    } while (1);

    // Loop while we wait for messages to drain off.
    size_t index = 0;
    for (index = 0; index < DOWORK_LOOP_NUM; index++) {
      IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
      ThreadAPI_Sleep(100);
    }

    IoTHubModuleClient_LL_Destroy(iotHubModuleClientHandle);
  }

  (void) printf("Finished executing\n");
  IoTHub_Deinit();
  return 0;
}
```

The particular section section of this code that generates and then displays the message is

```c
  sprintf_s(msgText, sizeof(msgText), "{\"Message from Device\":\"Hello World!\",\"Message Number\":%d}", iterator + 1);
``` 

This message also contains a message counter because the above example is formatted to send 20 message to the output before stopping. 

Some additional code that can be used to output the condition of certain module application Properties has been commented out in our above hello World example.
```C
double Some_Condition = 30;
(void)sprintf_s(propText, sizeof(propText), Some_Condition > 28 ? "true" : "false");
Map_AddOrUpdate(propMap, "ApplicationProperty", propText);
```
The rest of the code is made up of C language structures that are necessary to allow the module to properly function on any Azure IoT Edge device. 

Now that we have created a Hello World module we can create another module that takes the output of the Hello World module and then sends it to the IoT Hub. The Module can be defined as follows
```C
#include <stdio.h>
#include <stdlib.h>
#include "iothub_module_client_ll.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub.h"

typedef struct MESSAGE_INSTANCE_TAG {
  IOTHUB_MESSAGE_HANDLE messageHandle;
  size_t messageTrackingId; // For tracking the messages within the user callback.
}
MESSAGE_INSTANCE;

size_t messagesReceivedByInput1Queue = 0;

// SendConfirmationCallback is invoked when the message that was forwarded on from 'InputQueue1Callback'
// pipeline function is confirmed.
static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void * userContextCallback) {
  // The context corresponds to which message# we were at when we sent.
  MESSAGE_INSTANCE * messageInstance = (MESSAGE_INSTANCE * ) userContextCallback;
  printf("Confirmation[%zu] received for message with result = %d\r\n", messageInstance - > messageTrackingId, result);
  IoTHubMessage_Destroy(messageInstance - > messageHandle);
  free(messageInstance);
}

// Allocates a context for callback and clones the message
// NOTE: The message MUST be cloned at this stage. InputQueue1Callback's caller always frees the message
// so we need to pass down a new copy.
static MESSAGE_INSTANCE * CreateMessageInstance(IOTHUB_MESSAGE_HANDLE message) {
  MESSAGE_INSTANCE * messageInstance = (MESSAGE_INSTANCE * ) malloc(sizeof(MESSAGE_INSTANCE));
  if (NULL == messageInstance) {
    printf("Failed allocating 'MESSAGE_INSTANCE' for pipelined message\r\n");
  } else {
    memset(messageInstance, 0, sizeof( * messageInstance));

    if ((messageInstance - > messageHandle = IoTHubMessage_Clone(message)) == NULL) {
      free(messageInstance);
      messageInstance = NULL;
    } else {
      messageInstance - > messageTrackingId = messagesReceivedByInput1Queue;
    }
  }

  return messageInstance;
}

static IOTHUBMESSAGE_DISPOSITION_RESULT InputQueue1Callback(IOTHUB_MESSAGE_HANDLE message, void * userContextCallback) {
  IOTHUBMESSAGE_DISPOSITION_RESULT result;
  IOTHUB_CLIENT_RESULT clientResult;
  IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle = (IOTHUB_MODULE_CLIENT_LL_HANDLE) userContextCallback;

  unsigned
  const char * messageBody;
  size_t contentSize;

  if (IoTHubMessage_GetByteArray(message, & messageBody, & contentSize) != IOTHUB_MESSAGE_OK) {
    messageBody = "<null>";
  }

  printf("Received Message [%zu]\r\n Data: [%s]\r\n",
    messagesReceivedByInput1Queue, messageBody);

  // This message should be sent to next stop in the pipeline, namely "output1". What happens at "outpu1" is determined
  // by the configuration of the Edge routing table setup.
  MESSAGE_INSTANCE * messageInstance = CreateMessageInstance(message);
  if (NULL == messageInstance) {
    result = IOTHUBMESSAGE_ABANDONED;
  } else {
    printf("Sending message (%zu) to the next stage in pipeline\n", messagesReceivedByInput1Queue);

    clientResult = IoTHubModuleClient_LL_SendEventToOutputAsync(iotHubModuleClientHandle, messageInstance - > messageHandle, "output1", SendConfirmationCallback, (void * ) messageInstance);
    if (clientResult != IOTHUB_CLIENT_OK) {
      IoTHubMessage_Destroy(messageInstance - > messageHandle);
      free(messageInstance);
      printf("IoTHubModuleClient_LL_SendEventToOutputAsync failed on sending msg#=%zu, err=%d\n", messagesReceivedByInput1Queue, clientResult);
      result = IOTHUBMESSAGE_ABANDONED;
    } else {
      result = IOTHUBMESSAGE_ACCEPTED;
    }
  }

  messagesReceivedByInput1Queue++;
  return result;
}

static IOTHUB_MODULE_CLIENT_LL_HANDLE InitializeConnection() {
  IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;

  if (IoTHub_Init() != 0) {
    printf("Failed to initialize the platform.\r\n");
    iotHubModuleClientHandle = NULL;
  } else if ((iotHubModuleClientHandle = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol)) == NULL) {
    printf("ERROR: IoTHubModuleClient_LL_CreateFromEnvironment failed\r\n");
  } else {
    // Uncomment the following lines to enable verbose logging.
    // bool traceOn = true;
    // IoTHubModuleClient_LL_SetOption(iotHubModuleClientHandle, OPTION_LOG_TRACE, &trace);
  }

  return iotHubModuleClientHandle;
}

static void DeInitializeConnection(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle) {
  if (iotHubModuleClientHandle != NULL) {
    IoTHubModuleClient_LL_Destroy(iotHubModuleClientHandle);
  }
  IoTHub_Deinit();
}

static int SetupCallbacksForModule(IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle) {
  int ret;

  if (IoTHubModuleClient_LL_SetInputMessageCallback(iotHubModuleClientHandle, "input1", InputQueue1Callback, (void * ) iotHubModuleClientHandle) != IOTHUB_CLIENT_OK) {
    printf("ERROR: IoTHubModuleClient_LL_SetInputMessageCallback(\"input1\")..........FAILED!\r\n");
    ret = 1;
  } else {
    ret = 0;
  }

  return ret;
}

void iothub_module() {
  IOTHUB_MODULE_CLIENT_LL_HANDLE iotHubModuleClientHandle;

  srand((unsigned int) time(NULL));

  if ((iotHubModuleClientHandle = InitializeConnection()) != NULL && SetupCallbacksForModule(iotHubModuleClientHandle) == 0) {
    // The receiver just loops constantly waiting for messages.
    printf("Waiting for incoming messages.\r\n");
    while (true) {
      IoTHubModuleClient_LL_DoWork(iotHubModuleClientHandle);
      ThreadAPI_Sleep(100);
    }
  }

  DeInitializeConnection(iotHubModuleClientHandle);
}

int main(void) {
  static char msgText[1024];
  sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"Silgrat-Gorola\",\"Message Number\":%d}", 33);
  iothub_module();
  return 0;
}
```
### Routing
Now that these two modules have been defined, the next step is to update the deployment manifest so that it can properly handle the routing of the outputs from the modules. In the deployment manifest we need to edit the routes section to become
```JSON
{
  "routes": {
    "SenderModuleToIoTHub": "FROM /messages/modules/SenderModule/outputs/* INTO $upstream",
    "HelloModuleToIoTHub": "FROM /messages/modules/HelloModule/outputs/* INTO BrokeredEndpoint(\"/modules/SenderModule/inputs/input1\")"
  }
}
```
When his is done then the solution is ready to be pushed to the IoT hub!

For step by step instructions on how to setup VS Code for module development please look at [the following guide](https://docs.microsoft.com/en-us/azure/iot-edge/iot-edge-modules).

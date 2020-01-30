# Module Connection Methods

Azure IoT Edge treats modules as independent components that are loosely
coupled.  Communication between modules is managed by IoT Edge Hub.
Module twins contain a property called *routes* that declares how
messages are passed within a deployment. Modules can 'talk' to each other
through a well-defined interface established by the runtime. Not every
module needs to be mapped to a device.

Containers are the module delivery mechanism, and thus they use the
common standard for communicating from containers which is TCP. This
applies to whether the modules are on the same or networked computers.

  -----
  TCP
  -----

Azure IoT Hub natively supports communication over the MQTT, AMQP, and
HTTPS protocols. 

The following table provides the high-level recommendations for your
choice of protocol:

| Protocol            | When you should choose this protocol                                                                                                        |
|---------------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| MQTT                | Use on all devices that do not require to connect multiple devices (each with its own per-device credentials) over the same TLS connection. |
| MQTT over WebSocket | same as MQTT above                                                                                                                          |
| AMQP                | Use on field and cloud gateways to take advantage of connection multiplexing across devices.                                                |
| AMQP over WebSocket | Same as AMQT above                                                                                                                          |
| HTTPS               | Use for devices that cannot support other protocols.                                                                                        |

In some cases, devices or field gateways might not be able to use one of
these standard protocols and require protocol adaptation. In such cases,
you can use a custom gateway. The Microsoft guide entitled [Support
additional protocols for IoT
Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-protocol-gateway) contains more information.

  ---------------
  Other methods
  ---------------

-   It is also possible for two modules to communicate directly with
    each other, bypassing the Edge Hub. This can be done by exploiting
    Docker networking capabilities of the Azure Runtime which manages
    the DNS entries for each module (container). This allows one module
    to resolve the IP address of another module by its name.

-   An example of this is illustrated in the SQL tutorial
    here: <https://docs.microsoft.com/en-us/azure/iot-edge/tutorial-store-data-sql-server>.
    This tutorial uses a module to read data out of the Edge Hub and
    write it into another module hosting SQLServer using the SQLServer
    client SDK. This interaction with SQLServer does not use the Edge
    Hub for communicating.

-   In another example found (and resolved) here:
    <https://stackoverflow.com/questions/52073548/azure-iot-edge-moduleclient-invoke-direct-method-in-another-module>,
    a module client invokes in another module using a direct method.

  --------------
  File sharing
  --------------

Docker container properties can be utilized to mount a volume on the
host into a module (container). Using this, your host on which the Edge
is running and your module can freely exchange files. This approach
may be used to connect two modules. Volume mounting can be done using
the module\'s create options. Using this, your host and module can exchange files at will.
For more details on Docker mounting please refer to the following:

-   Link detailing docker
    mounts: <https://docs.docker.com/storage/volumes/>

-   Docker API
    options: <https://docs.docker.com/engine/api/v1.30/#operation/ContainerCreate>

    -   Use this to obtain the args to send to the docker REST API.

    -   Once you have this, head on over to your deployment and add
        these to your module.

References
==========

1.  [Understanding Azure IoT Edge
    Modules](https://docs.microsoft.com/en-us/azure/iot-edge/iot-edge-modules)

2.  [Azure IoT Hub communication protocols and
    ports](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-protocols)

3.  [Deploy Modules and Establish Routes in IoT
    Edge](https://docs.microsoft.com/bs-latn-ba/azure/iot-edge/module-composition)

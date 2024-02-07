# RAFT-Algorithm-GRPC
The code is implemented on top of the default helloworld project in grpc installed examples.

## Setup

For this project, I used the Ubuntu-based system and downloaded and installed GRPC as instructed by the official website. The dependencies and links with other files were used as it is from one of the examples given. I used the ‘Hello World’ example project to implement my master-slave implementation. The location of the proto file and other dependencies were predetermined and were used as it is. Cmake was used to build and run the project.

The commands to run my project from the root directory are as follows:
* mkdir -p cmake/build
* pushd cmake/build
* cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
* make -j 4



After this, we can run our node code (`./greeter_server.cc`).

## Explanation

`greeter_server.cc` (Node): I used `greeter_server.cc` to implement the code of a node that can send and receive messages from fellow nodes. There are headers that are followed by the necessary namespaces and include the `helloworld.proto` file, which defines the message structure that will be exchanged between the nodes.

The `NodeInfo` class has been defined to store information about a particular node, such as node ID, node status, and term number. This information is used for communication and coordination between the nodes.

The `GreeterClient` class has been defined to send messages and requests between the nodes. This class uses the `SayHello()` function to send a message to other nodes and receive a reply. It takes the message, node ID, node status, and term number as input parameters and returns the response from the node.

The `GreeterServiceImpl` class has been defined to deal with the request and responses received by the nodes. The `SayHello()` function of this class takes the received message, node ID, node status, and term number as input parameters, writes them to a log file, and sends a dummy reply.

The code also defines a `writeLogFiles()` function, which writes the messages sent and received by the node to a log file. This function sends a "Hello" message to all the other nodes and records the messages sent and received in the log file.

Three command line arguments are passed which contain the node ID (it is used to open a port on the local host), node status, and term number. The input or code is run using the following command:
* ./greeter_server [node id] [node status] [term number]

`Log Files`: Every node has its own log file which is opened in append mode to record all the messages sent and received by a particular node.

`helloworld.proto`: This file contains our basic service and other messages for communication between our nodes. Command and path locations for the proto file were already predetermined by the makefile provided by the original Hello World Package. This file introduced new services and messages for our Hello Request and Response services and their subsequent requests and replies. Message Hello Request now also contains strings of the node ID, node status, and term number.



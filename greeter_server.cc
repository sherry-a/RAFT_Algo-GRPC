#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <iomanip>
#include <sys/ioctl.h>
#include <termios.h>
#include <fstream>
#include <sstream>
// #include <hdfs.h>

using namespace std::chrono;
using namespace std;

#include <grpcpp/ext/proto_server_reflection_plugin.h> //automatically hardcoded for the file generation from makefile
#include <grpcpp/grpcpp.h> 
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD                                  //automatically hardcoded for the file generation from makefile
#include "examples/protos/helloworld.grpc.pb.h"         
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;                     //Grpc methods
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Channel;             //Grpc methods
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;        //from helloworld proto file
using helloworld::HelloReply;     //from helloworld proto file
using helloworld::HelloRequest;   //from helloworld proto file
using helloworld::VoteReply;     //from helloworld proto file
using helloworld::VoteRequest;   //from helloworld proto file
using helloworld::MapRequest;     //from helloworld proto file
using helloworld::MapReply;       //from helloworld proto file
using helloworld::ReduceRequest;  //from helloworld proto file
using helloworld::ReduceReply;    //from helloworld proto file

class NodeInfo {      //Class Struct to save info about a particular node
  public:
    string node_id;
    string node_status;
    string term_number;
    bool hasVoted = false;
    vector<string> yesVotes;
    vector<string> noVotes;
};

NodeInfo node_info;

class GreeterClient {       //class for sending messages and requests
 public:
  GreeterClient(shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  string SayHello(const std::string& user, const std::string& user1, const std::string& user2 , const std::string& user3) {
      HelloRequest request;       //for string we send to node as message
      request.set_name(user);     //containg hardcoded"hello"
      request.set_nodeid(user1);      //contains node id
      request.set_nodestatus(user2);  //contains node status
      request.set_termnum(user3);   //contains term number while sending
      HelloReply reply;           //to recieve data from slave

      ClientContext context;      //context for the nodes to specify extra informations during communication

      Status status = stub_->SayHello(&context, request, &reply);        //Status for RPC connection

      if (status.ok()) {
          return reply.message();          //Connection and reply successful
      }
      else {
          return "RPC failed";             //Connection unsuccessful
      }
  }

  string AskForVote(const std::string& filename, const std::string& termNumber) {
      VoteRequest request;       //for string we send to node as message
      request.set_name(filename);     //containg hardcoded"hello"
      request.set_termnum(termNumber);   //contains term number while sending
      VoteReply reply;           //to recieve data from slave

      ClientContext context;      //context for the nodes to specify extra informations during communication

      Status status = stub_->AskForVote(&context, request, &reply);        //Status for RPC connection

      if (status.ok()) {
          return reply.message();          //Connection and reply successful
      }
      else {
          return "RPC failed";             //Connection unsuccessful
      }
  }
  private:
    unique_ptr<Greeter::Stub> stub_;
};


vector<GreeterClient> nodes;     //Global variable for greeter client to send and receive message through thse
vector<string> nodePorts;       //contains the localhost port addresses of fellow nodes
string recvMessage = "";        

ofstream logfile;               //global object to write log file

class GreeterServiceImpl final : public Greeter::Service {
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {         //to deal with the request and response accordingly
    // string message = "Term number : " + node_info.term_number + "  ||   Response Content : " + "Reply";
    string name = request->name();
    string nodeid = request->nodeid();  
    string nodestatus = request->nodestatus();      //all three are saved as received by the node that sends them
    string termnum = request->termnum();
    logfile << "Currently: " << node_info.node_status << "   " << left << setw(25) << "Message RECEIVED from  " << "localhost:1000" << nodeid << " ==> " << "Term number : " << termnum << "  ||   Message  Content : " << name << endl;
    // logfile << left << setw(25) << "Message RECEIVED from  " << "localhost:1000" << nodeid << " (" << nodestatus << ") ==> " << "Term number : " << termnum << "  ||   Message  Content : " << name << endl;
                                                  //writing on log file
    // recvMessage = "Term number : " + node_info.term_number << "  ||   Message  Content : " << request
    reply->set_message("reply");                //the reply is set as reply
    node_info.term_number = to_string(stoi(node_info.term_number) + 1);
    return Status::OK;
  }

  Status AskForVote(ServerContext* context, const VoteRequest* request,
                  VoteReply* reply) override {
    string no = "NO," + node_info.node_id + "," + node_info.term_number;
    string yes = "YES," + node_info.node_id + "," + node_info.term_number;
    if(node_info.hasVoted == false){
      string name = request->name();
      string termnum = request->termnum();
      if(node_info.term_number > termnum){
        reply->set_message(no);
      }
      else{
        reply->set_message(yes);
        node_info.hasVoted = true;
      }
    }
    else{
      reply->set_message(no);
    }
    return Status::OK;
  }

};

void writeLogFiles(){
  string log_filename = "node_" + node_info.node_id + "_log.txt";     //specify name for each log file of node
  logfile.open(log_filename);                                         //open log file for that node
  // ofstream logfile(log_filename, std::ios_base::app); 

  for (int i = 0; i < nodes.size(); i++) {
    string request("Hello");                    //message to be sent to fellow nodes
    logfile << "Currently: " <<   node_info.node_status << "   " << left << setw(25) << "Message SENT to  " << nodePorts[i] << " ==> " << "Term number : " << node_info.term_number << "  ||   Message  Content : " << request << endl;
    // logfile << left << setw(25) << "Message SENT to  " << nodePorts[i] << " (" <<node_info.node_status << ") ==> " << "Term number : " << node_info.term_number << "  ||   Message  Content : " << request << endl;
    node_info.term_number = to_string(stoi(node_info.term_number) + 1);       //increment term number after sending a message to another node
    string response = nodes[i].SayHello(request, node_info.node_id, node_info.node_status, node_info.term_number);
    // logfile << left << setw(20) << "Message RECEIVED from  " << nodePorts[i] << " ==> " << response << endl;
    usleep(1000000);
  }
  
  // node_info.term_number = to_string(stoi(node_info.term_number) + 1);

  logfile.close();
}

void VoteFunction(){
  if(node_info.node_status == "Candidate"){
    usleep(((rand() % 5) + 1) * 100000);
    string fname = "node_" + node_info.node_id + "_log.txt";
    string message;
    string nodeid;
    string termnum;
    for (int i = 0; i < nodes.size(); i++) {
      string request(fname);
      string replystr = nodes[i].AskForVote(request, node_info.term_number);
      cout << replystr << endl;
      std::stringstream ss(replystr);
      std::vector<std::string> tokens;
      while (ss.good()) {
        string token;
        getline(ss, token, ',');
        tokens.push_back(token);
      }
      message = tokens[0];
      nodeid = tokens[1];
      termnum = tokens[2];
      cout << message << nodeid << termnum << endl;
      if(message == "NO"){
        if(node_info.term_number < termnum){
          node_info.node_status = "Follower";
        }
        else{
          node_info.noVotes.push_back(nodeid);
        }
      }
      else{
        node_info.yesVotes.push_back(nodeid);
      }
    }
    if(node_info.yesVotes.size() >= (5/2) + 1){
      for (int i = 0; i < nodes.size(); i++) {
        node_info.node_status = "Leader";
        string request("I am the NEW LEADER");                    //message to be sent to fellow nodes
        logfile << "Currently: " <<   node_info.node_status << "   " << left << setw(25) << "Message SENT to  " << nodePorts[i] << " (" << ") ==> " << "Term number : " << node_info.term_number << "  ||   Message  Content : " << request << endl;
        node_info.term_number = to_string(stoi(node_info.term_number) + 1);       //increment term number after sending a message to another node
        string response = nodes[i].SayHello(request, node_info.node_id, node_info.node_status, node_info.term_number);
      }
    }
  }
}

void RunServer(string node_id, string node_status, string term_number) {
  string server_address("localhost:1000" + node_id);          //the current node is assigned an address
  GreeterServiceImpl service;

  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&service);        //GRPC Listening server setup

  unique_ptr<Server> server(builder.BuildAndStart());
  cout << "Node Port Number : " << server_address << std::endl;

  thread t1 = thread(writeLogFiles);          //write log files for each node
  thread t2 = thread(VoteFunction);
  t1.join();
  t2.join();
  // VoteFunction();
  server->Wait();
}
void writeToConfigFile(string node_id, string node_status, string term_number){
  ofstream fileOut;
  fileOut.open("config_addresses.txt", ios_base::app);    //write addresses of the nodes and greeter servers as we type in
  fileOut << "localhost:1000" + node_id << endl;
  // fileOut << "localhost:1000" + node_id << "  " << node_id << "  " << node_status << "  " << term_number << endl;
  fileOut.close();
}

int main(int argc, char** argv) {  // 4 arguments... argv[1] = node id
  if(argc != 4){                   // argv[2] = node status
    return 0;                      // argv[3] = term number
  }
  
  node_info.node_id = argv[1];    //saving in class object and its variables
  if(stoi(node_info.node_id) > 9){    
    cout << "Please enter 1 digit number (0 - 9)...." << endl << endl << endl;
    return 0;
  }
  node_info.node_status = argv[2];
  if(node_info.node_status == "F"){
    node_info.node_status = "Follower";
  }
  else if(node_info.node_status == "C"){
    node_info.node_status = "Candidate";
  }
  else{
    cout << "Neither Follower nor Candidate" << endl;
    return 0;
  }
  node_info.term_number = argv[3];

  writeToConfigFile(node_info.node_id, node_info.node_status, node_info.term_number);   //enter info about new incoming node
  cout << "Node ID: " << node_info.node_id << endl;
  cout << "Node Status: " << node_info.node_status << endl;
  cout << "Term Number: " << node_info.term_number << endl << endl;

  ifstream configFile("config_addresses.txt");
  bool done = false;

  while (!done) {               //iterate the loop until there are 4 unique fellow nodes for each node
    if (configFile.is_open()) {
      string address;
      while (getline(configFile, address)) { 
        if (find(nodePorts.begin(), nodePorts.end(), address) == nodePorts.end()) { //to check for repetition
          if (address != ("localhost:1000" + node_info.node_id)) {
            nodePorts.push_back(address);
          }
        }
        if (nodePorts.size() == 4) {      //size = 4 means each node has 4 other unique nodes to communicate with
          done = true;
          break;
        }
      }
    }
    if (!done) {        //if there are no 4 unique addresses yet and we reach end of file... we begin to search again
      nodePorts.clear();
      configFile.clear();
      configFile.seekg(0, ios::beg);
    }
  }
  configFile.close();

  cout << "=== CONNECTIONS WITH ===" << endl;
  for(int i=0; i < nodePorts.size(); i++){
    cout << nodePorts[i] << endl;
    nodes.push_back(grpc::CreateChannel(nodePorts[i], grpc::InsecureChannelCredentials())); //making grpc channel connections
  }

  node_info.yesVotes.resize(0);
  node_info.noVotes.resize(0);
  RunServer(node_info.node_id, node_info.node_status, node_info.term_number);
  // thread t1 = thread(RunServer,node_info.node_id, node_info.node_status, node_info.term_number);    //to run server on listen mode
  // t1.join();

  return 0;
}
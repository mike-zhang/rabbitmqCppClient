#include "rabbitmqMgr.h"
#include <iostream>
#include <sstream>

using namespace std;

int callback(void *rmgr,amqp_envelope_t envelope)
{
    RabbitmqManager *mq = (RabbitmqManager *)rmgr;
    
    string msgRet = "rpc_result_";
    string temp = "";

    bytes_to_string(envelope.message.body,temp);
    //cout<<temp<<endl;

    msgRet += temp;
    //cout<<msgRet<<endl;

    mq->rpc_server_send("",msgRet.c_str(),envelope);        
    //mq->rpc_server_send("",temp.c_str(),envelope);        
    mq->doMsgAck(envelope);

    return 0;
}


int main()
{
    string qname = "rpc_queue";
    RabbitmqManager mq("127.0.0.1",5672);        
    
    if(!mq.connect())
    {
        mq.declareQueue(qname);
        mq.recv(callback,qname);
    }
    else
    {
        cout<<"connect fail!"<<endl;
    }
    return 0;    
}


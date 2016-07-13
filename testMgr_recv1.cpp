#include "rabbitmqMgr.h"
#include <string>
#include <iostream>

using namespace std;

int callback(void *rmgr,amqp_envelope_t envelope)
{
    RabbitmqManager *mq = (RabbitmqManager *)rmgr;     
    string msg;
    bytes_to_string(envelope.message.body,msg);
    cout<<"recv msg : " << msg <<endl;    
    mq->doMsgAck(envelope);
    sleep(1);
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


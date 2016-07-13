#include "rabbitmqMgr.h"
#include <string>
#include <iostream>

using namespace std;

int maxcount = 1000 * 1000;

int main()
{
    string qname = "rpc_queue";
    string reply_to_queue = "";
    RabbitmqManager mq("127.0.0.1",5672);
    
    if(!mq.connect())
    {
        mq.declareQueue(qname);
        mq.declareQueue(reply_to_queue,0,0,1,1);
        
        cout<<"reply_to_queue : "<<reply_to_queue<<endl;
        for(int i=0;i < maxcount;++i)
        {
            string recvdata;         
            char buf[128]={0};
            sprintf(buf,"%d",i);
            string curid = buf;
            if(!mq.rpc_client_send("",qname,buf,reply_to_queue,buf))
            {
                mq.rpc_client_recv(reply_to_queue,curid,recvdata);

                cout<<"send : "<<buf<<" ; recv : "<<recvdata<<endl;
            }
            else
            {
                cout<<"send fail!"<<endl;
                break;
            }
           // sleep(1);
        }
    }
    else
    {
        cout<<"connect fail!"<<endl;
    }
    
    return 0;    
}


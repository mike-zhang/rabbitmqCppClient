#include "rabbitmqMgr.h"
#include <iostream>

using namespace std;

int main()
{
    string qname = "rpc_queue";
    RabbitmqManager mq("127.0.0.1",5672);    
    char msgBuf[128]={0};
    
    if(!mq.connect())
    {
        mq.declareQueue(qname);
        // mq.queueBind(qname,"test_exchange");
        
        for(int i=0;i< 1000 * 1000;++i)
        {
            sprintf(msgBuf,"%d",i);
            if(!mq.send("",qname,msgBuf))
            {
                cout<<"send msg : "<<msgBuf<<endl;
            }
            else
            {
                cout<<"send fail!"<<endl;
                break;
            }
            sleep(1);
        }
    }
    else
    {
        cout<<"connect fail!"<<endl;
    }
    return 0;    
}


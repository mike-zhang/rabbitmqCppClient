#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <cstring>
#include <string>
#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>
#include <unistd.h>

using std::string;

typedef int (*msgdisposeFun)(void *,amqp_envelope_t);
int bytes_to_string(amqp_bytes_t bytes,string &retmsg);

class RabbitmqManager
{
public:
    RabbitmqManager(const string &hostname,int port);
    ~RabbitmqManager();    
    int connect();  
    bool doLogin();
    bool openChannel();
    int declareExchange(const string &exchange,string etype="fanout");    
    int declareQueue(string &qname,
        amqp_boolean_t passive = 0 ,
        amqp_boolean_t durable = 0,
        amqp_boolean_t exclusive = 0,
        amqp_boolean_t auto_delete = 0
    );
    int queueBind(const string &qname,const string &exchange);
    int doMsgAck(amqp_envelope_t envelope);
     
    int getSockfd();
    amqp_connection_state_t getConn();
    
    int send(const string &exchange,const string &qname,const string &msg);
    int rpc_server_send(const string &exchange,const string &msg,amqp_envelope_t envelope);
    int rpc_client_send(const string &exchange,const string &queue_name,
        const string &msg,const string &reply_to_queue,const string &correlation_id);
 
    int recv(msgdisposeFun callback,const string &qname,int msg_timeout=0);       
    int rpc_client_recv(const string &reply_to_queue,const string &correlation_id,
        string &rcvdata,int msg_timeout=0);

private:
    void init();
    void clean();
    int dispose_recv_error(int tstart,int msg_timeout,amqp_rpc_reply_t ret);
    void basic_consume(const string& qname);
    int sendBase(const string &exchange,const string &qname,
        const string &msg,amqp_basic_properties_t &props);
    
private:
    string hostname;
    int port;
    int status;      
    amqp_socket_t *socket;
    amqp_connection_state_t conn;   
    amqp_channel_t channel;
    bool bLogin;
    bool bChannelOpend;
    struct timeval consume_timeout;
	string cur_consume_queue;
	amqp_bytes_t cur_consumer_tag;
};



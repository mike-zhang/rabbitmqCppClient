#include "rabbitmqMgr.h"

amqp_bytes_t cstring_to_bytes(const char *str)
{
    return str ? amqp_cstring_bytes(str) : amqp_empty_bytes;
}

amqp_bytes_t string_to_bytes(const string &str)
{
    return cstring_to_bytes(str.c_str());
}

int bytes_to_string(amqp_bytes_t bytes,string &retmsg)
{
    int reslen = bytes.len + 1;
    char *buf = (char *)malloc(reslen);
    memset(buf,0,reslen);
    memcpy(buf,bytes.bytes,bytes.len);
    retmsg = string(buf);
    free(buf);
    return retmsg.length();
}

RabbitmqManager::RabbitmqManager(const string &hostname,int port):
        hostname(hostname),
        port(port)
{  
   init();
}

RabbitmqManager::~RabbitmqManager()
{   
    clean();
}

void RabbitmqManager::init()
{
    conn = amqp_new_connection(); 
    channel = 1;
    socket = NULL;    
    bLogin = false;
    bChannelOpend = false;    
    cur_consume_queue = "";    
    consume_timeout.tv_sec = 1; 
    consume_timeout.tv_usec = 0;
}

void RabbitmqManager::clean()
{
    if(conn)
    {
        amqp_channel_close(conn,channel,AMQP_REPLY_SUCCESS);
        amqp_connection_close(conn,AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(conn);        
    }
    if(socket)
    {
        free(socket);
    }
}

int RabbitmqManager::connect()
{
    int ret = 0;
    socket = amqp_tcp_socket_new(conn);
    if (socket) 
    {
        status = amqp_socket_open(socket,hostname.c_str(),port);
        if (status) 
        {
            bLogin = false;
            bChannelOpend = false;
            ret = -1;
        }
        else
        {
            doLogin();
            openChannel();
        }        
    }
    else
    {
        ret = -1;
    }
    return ret;    
}

bool RabbitmqManager::doLogin()
{
    if(!bLogin)
    {
        amqp_rpc_reply_t retX;
        retX = amqp_login(conn, "/", 
            AMQP_DEFAULT_MAX_CHANNELS, 
            AMQP_DEFAULT_FRAME_SIZE, 0,
            AMQP_SASL_METHOD_PLAIN, 
            "guest", "guest");
        if(retX.reply_type == AMQP_RESPONSE_NORMAL)
            bLogin = true;
    }   
    return bLogin;
}

bool RabbitmqManager::openChannel()
{
    if(!bChannelOpend)
    {
        amqp_channel_open(conn,channel);
        amqp_rpc_reply_t ret = amqp_get_rpc_reply(conn);
        if(!(ret.library_error < 0))
            bChannelOpend = true;
    }   
    return bChannelOpend;
}

int RabbitmqManager::declareExchange(const string &exchange,string etype)
{
    int result = -1;
    try 
    {
        amqp_exchange_declare(conn,channel,string_to_bytes(exchange),
            string_to_bytes(etype),0,1,amqp_empty_table);
        amqp_rpc_reply_t ret = amqp_get_rpc_reply(conn);
        if (!(ret.library_error < 0) )
            result = 0;
    }
    catch (...) 
    {
        result = -1;
    }   
    return result;
}

int RabbitmqManager::declareQueue(string &qname,
    amqp_boolean_t passive,amqp_boolean_t durable,
    amqp_boolean_t exclusive,amqp_boolean_t auto_delete)
{
    int result = -1;
    try 
    {        
        amqp_queue_declare_ok_t *reply=amqp_queue_declare(conn,channel,
            string_to_bytes(qname),passive,durable,exclusive,
            auto_delete,amqp_empty_table);
        amqp_get_rpc_reply(conn);
        if(reply)
        {
            bytes_to_string(reply->queue,qname);
            result = 0;
        }
    }
    catch (...) 
    {
        result = -1;
    }   
    return result;
}

int RabbitmqManager::queueBind(const string &qname,const string &exchange)
{
    int result = -1;
    try 
    {        
        amqp_queue_bind_ok_t *reply=amqp_queue_bind(conn,channel,
            string_to_bytes(qname),string_to_bytes(exchange),
            amqp_empty_bytes,amqp_empty_table);                    
        amqp_get_rpc_reply(conn);
        if(reply)
        {
            result = 0;
        }
    }
    catch (...) 
    {
        result = -1;
    }   
    return result;
}

int RabbitmqManager::doMsgAck(amqp_envelope_t envelope)
{
    amqp_boolean_t multiple=0;
    amqp_basic_ack(conn,channel,envelope.delivery_tag,multiple);
    return 0;
}

int  RabbitmqManager::getSockfd()
{
    return amqp_get_sockfd(conn);
}

amqp_connection_state_t RabbitmqManager::getConn()
{
    return conn;
}

int RabbitmqManager::sendBase(const string &exchange,const string &qname,
    const string &msg,amqp_basic_properties_t &props)
{
    int sendNum = 0;
    amqp_maybe_release_buffers(conn);
    amqp_bytes_t exchange_bytes = amqp_bytes_malloc_dup(string_to_bytes(exchange));
    amqp_bytes_t qname_bytes = amqp_bytes_malloc_dup(string_to_bytes(qname));
    amqp_bytes_t message_bytes = amqp_bytes_malloc_dup(string_to_bytes(msg));
    sendNum = amqp_basic_publish(conn,1,exchange_bytes,qname_bytes,0,0,&props,message_bytes);  
    amqp_bytes_free(exchange_bytes);    
    amqp_bytes_free(qname_bytes);    
    amqp_bytes_free(message_bytes);    
    return sendNum; //success : 0 , fail : < 0
}            

int RabbitmqManager::send(const string &exchange,const string &qname,const string &msg)
{   
    int sendNum = 0;    
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_bytes_malloc_dup(amqp_cstring_bytes("text/plain"));
    props.delivery_mode = 1; 
    
    sendNum = sendBase(exchange,qname,msg,props);
    return sendNum; //success : 0 , fail : < 0
}

int RabbitmqManager::rpc_server_send(const string &exchange,const string &msg,amqp_envelope_t envelope)
{   
    int sendNum = 0;   
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG |
           AMQP_BASIC_REPLY_TO_FLAG | AMQP_BASIC_CORRELATION_ID_FLAG;
    props.content_type = amqp_bytes_malloc_dup(amqp_cstring_bytes("text/plain"));
    props.delivery_mode = 1; 
    props.correlation_id = amqp_bytes_malloc_dup(envelope.message.properties.correlation_id);
    amqp_bytes_t routing_key = amqp_bytes_malloc_dup(envelope.message.properties.reply_to);       
    props.reply_to = amqp_bytes_malloc_dup(routing_key);    
    string reply_to;
    bytes_to_string(routing_key,reply_to);
    sendNum = sendBase(exchange,reply_to,msg,props);
    amqp_bytes_free(routing_key);    
    amqp_bytes_free(props.content_type);
    amqp_bytes_free(props.reply_to);
    amqp_bytes_free(props.correlation_id);    
    return sendNum; //success : 0 , fail : < 0
}

int RabbitmqManager::rpc_client_send(const string &exchange,
    const string &queue_name,const string &msg,
    const string &reply_to_queue,const string &correlation_id)
{   
    int sendNum = 0;
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG |
                   AMQP_BASIC_REPLY_TO_FLAG | AMQP_BASIC_CORRELATION_ID_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 1; 
    props.reply_to = string_to_bytes(reply_to_queue);  
    props.correlation_id = string_to_bytes(correlation_id);      
    sendNum = sendBase(exchange,queue_name,msg,props);    
    return sendNum; //success : 0 , fail : < 0
}

void RabbitmqManager::basic_consume(const string &qname)
{
	if(cur_consume_queue != qname)
	{
		amqp_basic_cancel(conn,channel,cur_consumer_tag);
		amqp_boolean_t no_ack=0; 
		amqp_boolean_t exclusive=0;
		//amqp_basic_qos(conn,channel,0,1,0);  
		amqp_basic_qos(conn,channel,0,0,0);   
		amqp_basic_consume_ok_t *cur_consume_ok = amqp_basic_consume(
			conn,channel,string_to_bytes(qname),amqp_empty_bytes,
			0,no_ack,exclusive,amqp_empty_table);
		cur_consumer_tag = amqp_bytes_malloc_dup(cur_consume_ok->consumer_tag);
		amqp_get_rpc_reply(conn);
		cur_consume_queue = qname;
	}
}

int RabbitmqManager::dispose_recv_error(int tstart,int msg_timeout,amqp_rpc_reply_t ret)
{
    int result = 0;
    if((msg_timeout > 0) and (time(NULL) - tstart > msg_timeout))
        result = -1;
    if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type && 
        AMQP_STATUS_TIMEOUT != ret.library_error &&
        AMQP_STATUS_HEARTBEAT_TIMEOUT != ret.library_error)
    {
        amqp_frame_t frame;
        if(AMQP_STATUS_OK != amqp_simple_wait_frame(conn,&frame))
            result = -2;
    }
    return result;
}

int RabbitmqManager::recv(msgdisposeFun callback,const string &qname,int msg_timeout)
{
    int result = 0;
    time_t tstart = time(NULL);
    basic_consume(qname);
    while(true)
    {		
        amqp_rpc_reply_t ret;
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(conn);
        ret = amqp_consume_message(conn,&envelope,&consume_timeout,0); 
        
        if (AMQP_RESPONSE_NORMAL == ret.reply_type) 
        {
            callback(this,envelope);
            amqp_destroy_envelope(&envelope);
        }
        else
        {
            result = dispose_recv_error(tstart,msg_timeout,ret);
            if(result < 0)
                break;
            usleep(20);
        }
    }
    return result;
}    

int RabbitmqManager::rpc_client_recv(const string &reply_to_queue,const string &correlation_id,
    string &rcvdata,int msg_timeout)
{
    int result = 0;
    time_t tstart = time(NULL);
    basic_consume(reply_to_queue);
    while(true)
    {		
        amqp_rpc_reply_t ret;
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(conn);
        ret = amqp_consume_message(conn,&envelope,&consume_timeout,0); 
        
        if (AMQP_RESPONSE_NORMAL == ret.reply_type) 
        {
            string temp = "";
            bytes_to_string(envelope.message.body,temp);
      
            string tmpid = "";
            bytes_to_string(envelope.message.properties.correlation_id,tmpid);
           
            if((correlation_id == "") or (correlation_id == tmpid))
            {
                rcvdata = temp;
                doMsgAck(envelope);
                amqp_destroy_envelope(&envelope);
                break;
            }
        }        
        else
        {
            result = dispose_recv_error(tstart,msg_timeout,ret);
            if(result < 0)
                break;
            usleep(20);
        }
       amqp_destroy_envelope(&envelope);
    }
    return result;
}    



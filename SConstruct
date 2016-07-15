libDepends = ['rabbitmq',]
mqmgrobj = Object("rabbitmqMgr.cpp")
proList = [
    "testMgr_send1",
    "testMgr_recv1",
    "testMgr_rpcClient1",
    "testMgr_rpcServer1"
]
for item in proList:
    Program(item, [item + '.cpp',mqmgrobj],LIBS=libDepends)

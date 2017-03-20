#ifndef SPRINKLER_H
#define SPRINKLER_H

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"

#define NOWTIME (double)Scheduler::instance().clock()
#define MYNODE Address::instance().get_nodeaddr(addr())
#define PKTNUM 60
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define INTERVAL 0.0332
#define T_HOP 0.0012
#define COLOR_NUM 16
#define MAX_REQ 10

enum MessageType
{
	BROADCAST,
	UNICAST,
	REQ,
	NAK,
};

enum StatusType
{
	MAINTAIN,
	RX, //接收
	TX //发送
};

enum PhaseType
{
	STREAMING,
	RECOVERY
};

struct hdr_sprinkler {
	
	MessageType msgType;

	int pktID; //数据ID
	int pktNum; //数据总数

	int srcID; //源节点ID
	int srcColor; //源节点Color
	int desID; //目标节点ID

	double sendTime;

	/* 下面是套路 */
	static int offset_;
    inline static int& offset() { return offset_; }
    inline static hdr_sprinkler * access(const Packet * p) {
        return (hdr_sprinkler*) p->access(offset_);
    }	
};

class Sprinkler_Agent;

class SprinklerTimer: public TimerHandler {
	public:
	    SprinklerTimer(Sprinkler_Agent* agent) : TimerHandler() { spl_agent = agent; }
	    inline virtual void expire(Event *); //处理超时
	private:
	    Sprinkler_Agent* spl_agent; 
};

class SprinklerTimer2: public TimerHandler {
	public:
	    SprinklerTimer2(Sprinkler_Agent* agent) : TimerHandler() { spl_agent = agent; }
	    inline virtual void expire(Event *); //处理超时
	private:
	    Sprinkler_Agent* spl_agent; 
};

class SprinklerTimer3: public TimerHandler {
	public:
	    SprinklerTimer3(Sprinkler_Agent* agent) : TimerHandler() { spl_agent = agent; }
	    inline virtual void expire(Event *); //处理超时
	private:
	    Sprinkler_Agent* spl_agent; 
};

class SprinklerTimer4: public TimerHandler {
	public:
	    SprinklerTimer4(Sprinkler_Agent* agent) : TimerHandler() { spl_agent = agent; }
	    inline virtual void expire(Event *); //处理超时
	private:
	    Sprinkler_Agent* spl_agent; 
};

class Sprinkler_Agent: public Agent {
	public :
		Sprinkler_Agent();
    	virtual void recv(Packet *p, Handler *);
		virtual int command(int argc, const char*const*argv);
		friend class SprinklerTimer; //用于接发packet
		friend class SprinklerTimer2; //用于控制phase
		friend class SprinklerTimer3; //用于控制phase
		friend class SprinklerTimer4; //干嘛的
    private:
    	int nodeID; //节点ID
    	int neighbors[4]; //邻居
    	int color; //D-2 Color的序号
    	int maxRow; //最大行数
    	int maxCol; //最大列数
    	int row; //所在行数
    	int col; //所在列数
    	
    	int parentID; //D-2 Color的序号
    	int reqID[MAX_REQ]; //请求的节点ID

    	bool CDS; //是否CDS节点
    	static int CDS_Num; //CDS的数量

    	StatusType nodeStatus; //节点所处状态
    	PhaseType nodePhase; //节点所处阶段

    	int pktNum; //数据包总数
    	int pktRecvNum; //数据接收数量
    	int lastSendPktID; //最后发送的数据包ID
    	int lastRecvPktID; //最后收到的数据包ID

    	static double sPeriod; //Streaming Phase的周期
    	SprinklerTimer* t; //广播
    	SprinklerTimer2* t2; //phase控制
    	SprinklerTimer3* t3; //单播延迟
    	SprinklerTimer4* t4; //REQ重传

    	void neighborsFind(int neighbors[], int maxRow, int maxCol, int row, int col); //寻找邻居
    	bool CDS_Judge(int maxRow, int maxCol, int row, int col); //判断CDS
    	int D2Color(bool CDS, int row, int col); //D-2 Coloring
    	int parent(int maxRow, int maxCol, int row, int col); //找父节点

    	void startSprinkler(); //启动Sprinkler
    	void send_Broadcast();
    	void send_Unicast();
    	void send_REQ();
    	void toRecovery();
    	void Broadcast_Handler(Packet *p);
    	void Unicast_Handler(Packet *p);
    	void REQ_Handler(Packet *p);
    	void NAK_Handler(Packet *p);
};


#endif
    
    
    

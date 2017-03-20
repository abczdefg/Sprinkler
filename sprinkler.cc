#include "sprinkler.h"
#include <iostream>
#include <fstream>

using namespace std;

int totalTr = 0;
int hdr_sprinkler::offset_;
int CDS_pktRecvNum = 0;
int NCDS_pktRecvNum = 0;
int lastCDS = 0;
int NCDS[49] = {0};
int firstCDS_row = 999;

static class SprinklerHeaderClass : public PacketHeaderClass {
	public:
		SprinklerHeaderClass() : PacketHeaderClass("PacketHeader/Sprinkler",sizeof(hdr_sprinkler)) {
			bind_offset(&hdr_sprinkler::offset_);
		}
} class_sprinkler_hdr;

static class SprinklerClass : public TclClass {
	public:
		SprinklerClass() : TclClass("Agent/Sprinkler") {}
		TclObject* create(int, const char*const*) {
			return (new Sprinkler_Agent());
		}
} class_sprinkler_agent;


Sprinkler_Agent::Sprinkler_Agent(): Agent(PT_SPRINKLER), nodeID(-1) {
	bind("nodeID", &nodeID);
	bind("maxRow", &maxRow);
	bind("maxCol", &maxCol);
	bind("row", &row);
	bind("col", &col);
	bind("pktNum", &pktNum);
	parentID = -1; //父节点
	memset(reqID, -1, sizeof(reqID));
	lastSendPktID = 0; //最后发送的ID
	lastRecvPktID = -1; //最后接收的ID
	// pktNum = PKTNUM; //数据包总数量
	pktRecvNum = 0; //接收数据包数量
	nodeStatus = MAINTAIN; //节点所处阶段
	nodePhase = STREAMING;
	neighbors[UP] = -1;
	neighbors[RIGHT] = -1;
	neighbors[DOWN] = -1;
	neighbors[LEFT] = -1;
	CDS = false; //是否CDS节点

	color = -1; //获取颜色	
	t = new SprinklerTimer(this); //用于广播packet
	t2 = new SprinklerTimer2(this); //用于控制phase
	t3 = new SprinklerTimer3(this); //用于单播
	t4 = new SprinklerTimer4(this); //用于单播
}
//静态成员变量在类外定义
int Sprinkler_Agent::CDS_Num = 0; 
double Sprinkler_Agent::sPeriod = 0.00;

/**
*处理初始启动Sprinkler的函数
*/
void Sprinkler_Agent::startSprinkler() {
	printf("======= Starting Sprinkler ======\n");
	printf("======= 进入Streaming阶段 ======\n");
	//设置node_0
	nodeStatus = TX;
	lastRecvPktID = pktNum - 1;
	++ CDS_pktRecvNum ;
	pktRecvNum = pktNum;
	t->resched(1.00);
}


void Sprinkler_Agent::toRecovery() {
	// printf("node_%d进入Recovery Phase \n", nodeID);
	nodePhase = RECOVERY;
	send_REQ();
}

void Sprinkler_Agent::send_Broadcast() {
	nodeStatus = TX;
	Packet* pkt = allocpkt(); //数据包的生成
	hdr_ip *iph = hdr_ip::access(pkt); //数据包的访问
	hdr_sprinkler *shdr = hdr_sprinkler::access(pkt);
	iph->saddr() = nodeID; //源IP地址
	iph->daddr() = IP_BROADCAST; //目的IP地址
	iph->ttl() = 2; //设置为2表示只能穿过一个路由

	shdr->msgType = BROADCAST;
	shdr->srcColor = color;
	shdr->srcID = nodeID;
	shdr->desID = IP_BROADCAST;
	shdr->pktID = lastSendPktID;
	shdr->sendTime = NOWTIME;

	// if(nodeID == 37) printf("=== At %lf node %d send Broadcast pkt_%d (send) ===\n", NOWTIME, nodeID, shdr->pktID);
	send( pkt, 0 );
	totalTr++;	 //发送数据包 send( pkt, 0 ) ，我们可以不用去管 0 是什么意思
	if (lastSendPktID < pktNum - 1) {
		++ lastSendPktID;
		t->resched(sPeriod);
	} else if (lastSendPktID == pktNum - 1) {
		//广播单播发送完毕，进入MAINTAIN状态
		nodeStatus = MAINTAIN;
	}
}

void Sprinkler_Agent::send_Unicast() {
	nodeStatus = TX;
	Packet* pkt = allocpkt(); //数据包的生成
	hdr_ip *iph = hdr_ip::access(pkt); //数据包的访问
	hdr_sprinkler *shdr = hdr_sprinkler::access(pkt);
	iph->saddr() = nodeID; //源IP地址
	iph->daddr() = reqID[0]; //目的IP地址

	shdr->msgType = UNICAST;
	shdr->srcColor = color;
	shdr->srcID = nodeID;
	shdr->desID = reqID[0];
	shdr->pktID = lastSendPktID;
	shdr->sendTime = NOWTIME;

	// printf("=== At %lf node %d send Unicast pkt_%d (send) ===\n", NOWTIME, nodeID, shdr->pktID);
	 
	send( pkt, 0 );
	totalTr++;	
	if (lastSendPktID < pktNum - 1) {
		//发送过程进入TX状态
		nodeStatus = TX;
		++ lastSendPktID;
		t3->resched(INTERVAL);
	} else if (lastSendPktID == pktNum - 1) {
		//单播发送完毕，检查reqID列表
		int lastIndex;
		reqID[0] = -1;
		for (lastIndex = 0; lastIndex < MAX_REQ - 1; ++lastIndex) {
			if (reqID[lastIndex + 1] == -1) break;
			reqID[lastIndex] = reqID[lastIndex + 1];
		}
		if (lastIndex == 0)	{
			nodeStatus = MAINTAIN;
		} else {
			lastSendPktID = 0;
			t3 -> resched (INTERVAL);
		}
		// printf("=== node_%d 发送Unicast到node_%d完毕 ===\n", nodeID, reqID);
	}
}

void Sprinkler_Agent::send_REQ() {
	nodeStatus = TX;
	Packet* pkt = allocpkt(); //数据包的生成
	hdr_ip *iph = hdr_ip::access(pkt); //数据包的访问
	hdr_sprinkler *shdr = hdr_sprinkler::access(pkt);
	iph->saddr() = nodeID; //源IP地址
	iph->daddr() = parentID; //目的IP地址

	shdr->msgType = REQ;
	shdr->srcColor = color;
	shdr->srcID = nodeID;
	shdr->desID = parentID;
	shdr->pktID = lastSendPktID;
	shdr->sendTime = NOWTIME;
	// printf("At %lf node_%d向node_%d发出了REQ\n",NOWTIME, nodeID, parentID);
	send( pkt, 0 );
	totalTr++;	

	nodeStatus = RX; //发送请求后进入RX模式

	// 请求超时
	if(pktRecvNum == 0) t4 -> resched(pktNum * (INTERVAL + T_HOP));
}

void Sprinkler_Agent::recv(Packet *p, Handler *) {
	hdr_sprinkler *shdr = hdr_sprinkler::access(p);
	switch(shdr->msgType) {
		case BROADCAST: 
			Broadcast_Handler(p);
			break;
		case REQ:
			REQ_Handler(p);
			break;
		case UNICAST:
			Unicast_Handler(p);
			break;		
		case NAK:
			NAK_Handler(p);
			break;					
		default:
			break;
	}
}

void Sprinkler_Agent::Broadcast_Handler(Packet *p) {
	hdr_sprinkler *shdr = hdr_sprinkler::access(p);
	// if(nodeID == 38) printf("=== At %lf node %d ==pkt %d==> node_%d (recv) ===\n", NOWTIME, shdr->srcID, shdr->pktID, nodeID);
	if (shdr->srcID == parentID) {
		// 计算TDMA时间差，注意同步时间
		int diffColor = color-shdr->srcColor;
		if (diffColor < 0) diffColor *= -1;
		double deltaC = diffColor % COLOR_NUM * T_HOP;
		if(!CDS) {
			// 根据数据包的ID / 接收数，控制进入recovery模式的时间
			t2 -> resched((CDS_Num-CDS_pktRecvNum+(col+row)%3)*pktNum*sPeriod+deltaC);
		} else if(CDS && pktRecvNum < pktNum) {
			// if(nodeID == 38) printf("=== At %lf node %d ==pkt %d==> node_%d (recv) ===\n", NOWTIME, shdr->srcID, shdr->pktID, nodeID);
			nodeStatus = RX;
			if (lastRecvPktID + 1 == shdr->pktID) {
				// 连续的数据包
				lastRecvPktID = shdr->pktID;
				++ pktRecvNum;
				if (lastRecvPktID >= 0 && lastRecvPktID <= pktNum - 1) {
					if (lastRecvPktID ==  pktNum - 1) {
						nodeStatus = MAINTAIN;

						if(pktNum == pktRecvNum) { 
							++ CDS_pktRecvNum; // 数据包接收完全，统计已接受完毕的CDS节点
							printf("At %lf node_%d接收完毕, 总数%d\n", NOWTIME, nodeID, CDS_pktRecvNum);
							if (CDS_pktRecvNum == CDS_Num) {
								// 所有CDS都完整接收
								// printf("At %lf CDS接收完毕, 总数%d\n", NOWTIME, CDS_pktRecvNum);
							}
							//通知最后一个进入Recovery Phase
							if (nodeID == lastCDS) {
								Packet* pkt = allocpkt(); //数据包的生成
								hdr_ip *iph = hdr_ip::access(pkt); //数据包的访问
								hdr_sprinkler *shdr = hdr_sprinkler::access(pkt);
								iph->saddr() = nodeID; //源IP地址
								iph->daddr() = IP_BROADCAST; //目的IP地址
								iph->ttl() = 2; //设置为2表示只能穿过一个路由

								shdr->msgType = BROADCAST;
								shdr->srcColor = color;
								shdr->srcID = nodeID;
								shdr->desID = IP_BROADCAST;
								shdr->pktID = -1;
								shdr->sendTime = NOWTIME;
								send(pkt, 0);
							}
						} else {
							// 数据包未接收完全
							printf("node_%d 未接收完整, 接收总数%d\n", nodeID, pktRecvNum);
						}
					} else {
						//在Stream阶段最后一个CDS节点不发送
						if (nodeID != lastCDS) {
							t->resched(sPeriod - (NOWTIME-shdr->sendTime) + deltaC);
						}
					}
				}
			} else if (lastRecvPktID <= shdr->pktID) {
				lastRecvPktID = shdr->pktID;
				pktRecvNum = lastRecvPktID + 1;
			} else{
				//非连续，发出NAK
				nodeStatus = TX;
				Packet* pkt_NAK = allocpkt(); //数据包的生成
				hdr_ip *iph_NAK = hdr_ip::access(pkt_NAK); //数据包的访问
				hdr_sprinkler *shdr_NAK = hdr_sprinkler::access(pkt_NAK);
				iph_NAK->saddr() = nodeID; //源IP地址
				iph_NAK->daddr() = parentID; //目的IP地址

				shdr_NAK->msgType = NAK;
				shdr_NAK->srcColor = color;
				shdr_NAK->srcID = nodeID;
				shdr_NAK->desID = parentID;
				shdr_NAK->pktID = lastSendPktID + 1; //请求last+1数据包
				shdr_NAK->sendTime = shdr->sendTime; //NAK包含接收数据包的发送时间
				// printf("At %lf node_%d向node_%d发出了NAK\n",NOWTIME, nodeID, parentID);
				send( pkt_NAK, 0 );
				totalTr++;	

				nodeStatus = RX; //发送请求后进入RX模式
			}
			
		}
	}
}

void Sprinkler_Agent::Unicast_Handler(Packet *p) {

	hdr_sprinkler *shdr = hdr_sprinkler::access(p);
	// printf("=== At %lf node %d ==pkt %d==> node_%d (recv) ===\n", NOWTIME, shdr->srcID, shdr->pktID, nodeID);
	if (shdr->srcID == parentID && !CDS) {
		nodeStatus = RX;
		if(pktRecvNum < pktNum && shdr->pktID >= 0) {
			lastRecvPktID = shdr->pktID;
			++ pktRecvNum;
			if (lastRecvPktID >= 0 && lastRecvPktID <= pktNum - 1) {
				if (lastRecvPktID ==  pktNum - 1) {
					nodeStatus = MAINTAIN;
					if(pktNum == pktRecvNum) { 
						++ NCDS_pktRecvNum;
						printf("At %lf node_%d 接收完毕, 来自node_%d, 目前NCDS接收%d个\n", NOWTIME, nodeID, parentID, NCDS_pktRecvNum);
						if (NCDS_pktRecvNum == maxRow * maxCol - CDS_Num) {
							printf("At %lf NCDS接收完毕, 总消息数%d\n", NOWTIME, totalTr);
						}
					} else {
						printf("node_%d 未接收完整, 接收总数%d, 来自node_%d\n", nodeID, pktRecvNum, parentID);
					}
				} 
			}
		} else if (shdr->pktID < 0) {
			//重发请求
			nodeStatus = MAINTAIN;
			t4->resched(pktNum*(T_HOP+INTERVAL));
		}
	}
}

void Sprinkler_Agent::REQ_Handler(Packet *p) {
	hdr_sprinkler *shdr_recv = hdr_sprinkler::access(p);
	// printf("At %lf node_%d收到来自node_%d的请求\n",NOWTIME, nodeID, shdr_recv->srcID);
	switch(nodeStatus) {
		case MAINTAIN: 
			nodeStatus = TX;
			reqID[0] = shdr_recv->srcID;
			lastSendPktID = 0;
			nodePhase = RECOVERY;
			t3 -> resched (INTERVAL);
			break;
		case TX: {
			// printf("node_%d收到来自node_%d的请求, 但还在发送状态(TX)\n", nodeID, shdr_recv->srcID);
			Packet* pkt = allocpkt(); //数据包的生成
			hdr_ip *iph = hdr_ip::access(pkt); //数据包的访问
			hdr_sprinkler *shdr_send = hdr_sprinkler::access(pkt);
			iph->saddr() = nodeID; //源IP地址
			iph->daddr() = shdr_recv->srcID; //目的IP地址

			shdr_send->msgType = UNICAST;
			shdr_send->srcColor = color;
			shdr_send->srcID = nodeID;
			shdr_send->desID = shdr_recv->srcID;
			shdr_send->pktID = -1;
			shdr_send->sendTime = NOWTIME;
			send(pkt, 0);
		}
			break;
		default:
			break;
	}
}

void Sprinkler_Agent::NAK_Handler(Packet *p) {
	hdr_sprinkler *shdr_recv = hdr_sprinkler::access(p);
	// printf("node_%d收到了来自node_%dNAK\n", nodeID, shdr_recv->srcID);

	Packet* pkt = allocpkt(); //数据包的生成
	hdr_ip *iph = hdr_ip::access(pkt); //数据包的访问
	hdr_sprinkler *shdr_send = hdr_sprinkler::access(pkt);
	iph->saddr() = nodeID; //源IP地址
	iph->daddr() = shdr_recv->srcID; //目的IP地址
	shdr_send->msgType = BROADCAST;
	shdr_send->srcColor = color;
	shdr_send->srcID = nodeID;
	shdr_send->desID = shdr_recv->srcID;
	shdr_send->pktID = shdr_recv->pktID;
	shdr_send->sendTime = NOWTIME;
	printf("At %lf node_%d发送pkt_%d\n", NOWTIME, nodeID, shdr_recv->pktID);
	send(pkt, 0);	

	t->resched(sPeriod - NOWTIME + shdr_recv->sendTime); //重置发送定时器
}

void Sprinkler_Agent::neighborsFind(int neighbors[], int maxRow, int maxCol, int row, int col) {
	if ( col == 0 ) {
		neighbors[DOWN] = (row+1)*maxCol+col; //下邻居
	} else if ( col > 0 && col < maxCol-1 ) {
		neighbors[UP] = (row-1)*maxCol+col; //上邻居
		neighbors[DOWN] = (row+1)*maxCol+col; //下邻居
	} else {
		neighbors[UP] = (row-1)*maxCol+col; //上邻居
	}

	if ( row == 0 ) {
		neighbors[RIGHT] = row*maxCol+col+1; //右邻居
	} else if ( row > 0 && row < maxRow-1 ) {
		neighbors[LEFT] = row*maxCol+col-1; //左邻居
		neighbors[RIGHT] = row*maxCol+col+1; //右邻居
	} else {
		neighbors[LEFT] = row*maxCol+col-1; //左邻居
	}
	//printf("node %d 的邻居为 %d %d %d %d\n", row*maxCol+col, neighbors[UP], neighbors[RIGHT], neighbors[DOWN], neighbors[LEFT]);
}

bool Sprinkler_Agent::CDS_Judge(int r, int c, int i, int j) {
	if (r % 3 == 0) {
		return (i % 3 == 1) || ( (i % 3 != 1) && j == 0 && i > 0 && i < r - 1 );
	} else if (r % 3 == 1) {
		return (i % 3 == 0) || ( (i % 3 != 0) && j == 0 );
	} else if (r % 3 == 2) {
		return (i % 3 == 1) || ( (i % 3 != 1) && j == 0 && i != 0 );
	}
	return false;
}

int Sprinkler_Agent::D2Color(bool CDS, int i, int j) {
	if (!CDS) return -1;
	else {
		int new_i = i - firstCDS_row;
		if (new_i % 3 == 0) {
			if (j == 0) return new_i * 8 / 3 % COLOR_NUM;
			else return (D2Color(CDS, i, 0) + j) % COLOR_NUM;
		} else if (new_i % 3 == 1) {
			if (j == 0) return (D2Color(CDS, i-1, 0) + 6) % COLOR_NUM;
		} else if (new_i % 3 == 2) {
			if (j == 0) return (D2Color(CDS, i-1, 0) + 7) % COLOR_NUM;
		}
		return -1;
	}
}

int Sprinkler_Agent::parent(int maxRow, int maxCol, int row, int col) {
	//特殊情况
	if (row >= maxRow || col >= maxCol) return -1;

	//第一行，按左下的顺序
	if (row == 0) {
		if (col == 0) {
			if (CDS_Judge(maxRow, maxCol, row, col)) return -1;
			else return (row + 1) * maxCol + col;
		} else {
			if (CDS_Judge(maxRow, maxCol, row, col - 1)) return row * maxCol + col - 1;
			else if (CDS_Judge(maxRow, maxCol, row + 1, col)) return (row + 1) * maxCol + col;
			else return -1;
		}
	}

	//其它，按上左下的顺序
	if (CDS_Judge(maxRow, maxCol, row - 1, col)) return (row - 1) * maxCol + col;
	else if (CDS_Judge(maxRow, maxCol, row, col - 1)) return row * maxCol + col - 1;
	else if (CDS_Judge(maxRow, maxCol, row + 1, col)) return (row + 1) * maxCol + col;
	else return -1;

	return -1;
}
	
int Sprinkler_Agent::command(int argc, const char*const* argv) {
	if( argc == 2 ) {
		if( strcmp( argv[1], "startSprinkler" ) == 0 ) {		
			startSprinkler();
			return (TCL_OK); 
		} else if ( strcmp( argv[1], "initSprinkler" ) == 0 ) {
			row = nodeID / maxRow; //当前行数 0 ~ MAX-1
			col = nodeID % maxRow; //当前列数 0 ~ MAX-1
			CDS = CDS_Judge(maxRow, maxCol, row, col); //是否CDS节点
			if (CDS) {
				++CDS_Num; //统计CDS数量
				if(lastCDS < nodeID) lastCDS = nodeID;
				if(row < firstCDS_row) firstCDS_row = row;
			}
			// else t2->resched(20.0 + (col + row) % 3 * pktNum * (INTERVAL + T_HOP)); //让N-CDS定时进入R-phase
			color = D2Color(CDS, row, col); //获取颜色
			neighborsFind(neighbors, maxRow, maxCol, row, col); //寻找邻居
			parentID = parent(maxRow, maxCol, row, col);
			sPeriod = T_HOP * COLOR_NUM;
			nodePhase = STREAMING;
			// printf("node_%d的父节点为node_%d\n", nodeID, parentID);
			// printf("node_%d的颜色为node_%d\n", nodeID, color);
			return (TCL_OK); 
		}
	}
	return (Agent::command(argc, argv));

}

void SprinklerTimer::expire(Event *){
	spl_agent->send_Broadcast();
}

void SprinklerTimer2::expire(Event *){
	spl_agent->toRecovery();
}

void SprinklerTimer3::expire(Event *){
	spl_agent->send_Unicast();
}

void SprinklerTimer4::expire(Event *){
	spl_agent->send_REQ();
}
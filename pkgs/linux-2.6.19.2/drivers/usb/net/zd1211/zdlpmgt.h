#if ZDCONF_LP_SUPPORT == 1
#ifndef __ZDLPMGT_H
#define __ZDLPMGT_H

#define LP_MAX_PKTS_IN_BUCKET 2
#define LP_BUCKETS 64 
#define LP_TIMEOUT 1 //in ms

struct lp_desc {
    struct lp_desc *next;
    u8 pktCnt;
	u32 pktSize;
    u32 createTime; //in jiffies
    u8 sending;
	u8 sendFirst;
    fragInfo_t pkt[LP_MAX_PKTS_IN_BUCKET];
};
struct lp_queue {
    struct lp_desc *first; 
    struct lp_desc *last;
    U32 count; 
};
int pushPkt(fragInfo_t *pkt, BOOLEAN LenType, U32 NOW);
struct lp_desc *popPkt(BOOLEAN ANYONE, BOOLEAN LenType, U32 NOW);
void initLP(void);
int LP_totalReadyLen(U32 NOW);
void setLP_MAX_PKTS_IN_BUCKET(U32 pkts);
void setLP_TIMEOUT(U32 uSec);
void LP_CNT_SHOW(void);
void lp_recycle_tx_bucket(struct lp_desc * bucket);
#endif
#endif



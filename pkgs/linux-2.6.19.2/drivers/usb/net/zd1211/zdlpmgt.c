#if ZDCONF_LP_SUPPORT == 1
#include "zd1205.h"
#include "zdlpmgt.h"

static struct lp_queue lp_q_sending;
static struct lp_queue lp_q_empty;
static struct lp_queue lp_q_half;
static struct lp_desc lp_bucket[LP_BUCKETS];
static spinlock_t lp_spinlock;
static U32 lp_spinlock_flag;
static U32 varLP_MAX_PKTS_IN_BUCKET = LP_MAX_PKTS_IN_BUCKET;
//static U32 varLP_MAX_SIZE_IN_BUCKET = LP_MAX_SIZE_IN_BUCKET;
static U32 varLP_TIMEOUT = LP_TIMEOUT;
static U16 LP_MAX_SIZE_IN_BUCKET[]= { 3839,1792 };


U32 LP_CNT_PUSH_SUCC;
U32 LP_CNT_PUSH_FAIL;
U32 LP_CNT_POP_SUCC;
U32 LP_CNT_POP_FAIL;
U32 LP_CNT_PERIOD_POP_SUCC;
U32 LP_CNT_PERIOD_POP_FAIL;
U32 LP_CNT_LAST_NEW_BUCK_IDX;
U32 LP_CNT_LAST_LATENCY;

void lp_lock(void) 
{
	spin_lock_irqsave(&lp_spinlock, lp_spinlock_flag);
}
void lp_unlock(void) 
{
	spin_unlock_irqrestore(&lp_spinlock, lp_spinlock_flag);
}
void setLP_MAX_PKTS_IN_BUCKET(U32 pkts) 
{
	lp_lock();
	varLP_MAX_PKTS_IN_BUCKET = pkts;
	lp_unlock();
}
void setLP_TIMEOUT(U32 uSec) 
{
    lp_lock();
    varLP_TIMEOUT= uSec;
    lp_unlock();

}
static struct lp_desc * LP_pop_lp_desc(struct lp_queue *lpq)
{
    struct lp_desc *element;
    if(lpq == NULL )
    {
        printk("LP_pop_lp_desc has NULL argument\n");
        return NULL;
    }
    if(lpq->first == NULL)
    {
        return NULL;
    }
    element = lpq->first;
    lpq->first = element->next;
    if(NULL == lpq->first) lpq->last = NULL; 
    element->next = NULL;
    lpq->count--;
    if(lpq->count < 0) printk("!!!!!!!! lpq->count < 0\n");
    return element;
}
static struct lp_desc * LP_midpop_lp_desc(struct lp_queue *lpq, struct lp_desc *desc)
{
    struct lp_desc *element;
    struct lp_desc *last_desc = NULL;
    U16 loopCheck = 0;
    if(lpq == NULL  || desc == NULL)
    {
        printk("LP_midpop_lp_desc has NULL argument\n");
        return NULL;
    }
    if(lpq->first == NULL)
    {
        return NULL;
    }
    element = lpq->first;
    while(element)
    {
        if(loopCheck++ > 100)
        {
            printk("infinite loop occurs in %s\n", __FUNCTION__);
            loopCheck = 0;
            break;
        }

        if(element == desc)
        {
            if(element == lpq->last) lpq->last = last_desc;
            if(!last_desc) //desc is lpq->first
            {
                lpq->first = element->next;
            }
            else
                last_desc->next = element->next;
            lpq->count --;
            element->next = NULL;
            return element;
        }
        last_desc = element;
        element = element->next;
    }
    return NULL;
}

static void LP_push_lp_desc(struct lp_queue *lpq, struct lp_desc *desc)
{
    struct lp_desc *element;
    if(lpq == NULL || desc == NULL)
    {
        printk("LP_push_lp_desc has NULL argument\n");
        return;
    }
    if(lpq->first == NULL)
    {
        lpq->first = desc;
        lpq->last = desc;
        desc->next = NULL;
        lpq->count = 1;
        return;
    }
    element = lpq->last;
    lpq->last = desc;
    element->next = desc;
    desc->next = NULL;
    lpq->count++;
    return;
}
static struct lp_desc * LP_first_lp_desc(struct lp_queue *lpq)
{
    if(lpq == NULL )
    {
        printk("LP_push_lp_desc has NULL argument\n");
        return NULL;
    }
    return lpq->first;
}
/*
int LP_totalReadyLen(U32 NOW) {
    int i,total=0;
    for(i=0;i<LP_BUCKETS;i++) {
        if(lp_bucket[i].sending == 1) continue;
        if(lp_bucket[i].pktCnt == 0) continue;
        if((lp_bucket[i].pktCnt >= varLP_MAX_PKTS_IN_BUCKET) ||
           (lp_bucket[i].pktSize > varLP_MAX_SIZE_IN_BUCKET*8/10) ||
           ((NOW - lp_bucket[i].createTime) >= varLP_TIMEOUT ))
        {
            total+= lp_bucket[i].pktSize;
        }

    }
    return total;
}
*/
void lp_recycle_tx_bucket(struct lp_desc * bucket)
{
    lp_lock();
    if(NULL == bucket)
    {
        printk("Why do you recycle NULL bucket\n");
        lp_unlock();
        return;
    }
    if(!LP_midpop_lp_desc(&lp_q_sending, bucket))
    {
        printk("%s, bucket doesn't exists\n", __FUNCTION__);
    }
    LP_push_lp_desc(&lp_q_empty, bucket);
    lp_unlock();
    return;
}

int pushPkt(fragInfo_t *pkt, BOOLEAN LenType, U32 NOW) 
{
	u32 ret=-1;
    struct lp_desc *desc;
    U16 loopCheck = 0;
    if(LenType & ~BIT_0)
    {
        printk("LenType != 0 or 1, Set it as 1\n");
        LenType = 1;
    }
	lp_lock();
    desc = LP_first_lp_desc(&lp_q_half);
    while(desc)
    {
        if(loopCheck++ > 100)
        {
            printk("infinite loop occurs in %s\n", __FUNCTION__);
            loopCheck = 0;
            break;
        }

		if(desc->sending == 1 ) {
            printk("Half is sending !?\n");
            desc = desc->next;
			continue;
		}
		if(desc->pktCnt > 0 && desc->pktCnt <  LP_MAX_PKTS_IN_BUCKET && desc->pktSize+pkt->bodyLen[0] < LP_MAX_SIZE_IN_BUCKET[LenType]) 
        {
			if(memcmp(desc->pkt[0].EthHdr, pkt->EthHdr, ETH_ALEN) == 0) 
            {
				//printk("Put packet in existing bucket\n ");
				memcpy(&desc->pkt[desc->pktCnt],pkt,sizeof(fragInfo_t));
				desc->pktCnt++;
				desc->pktSize+= pkt->bodyLen[0];
				lp_unlock();
				ret = 0;
				return ret;
			}

		}
        desc = desc->next;
	}

	if(!desc) {

		//printk("Create a new bucket\n");
		//LP_CNT_LAST_NEW_BUCK_IDX = freeBucket;
        desc = LP_pop_lp_desc(&lp_q_empty);
        if(desc == NULL)
        {
            printk("We are out of lp_q_empty\n");
        } 
        else
        {
            memcpy(&desc->pkt[0],pkt,sizeof(fragInfo_t));
            desc->pktCnt++;
            desc->pktSize = pkt->bodyLen[0];
            desc->createTime = NOW;
            LP_push_lp_desc(&lp_q_half, desc);
            ret = 0;
        }
    }
    lp_unlock();
	return ret;
}
struct lp_desc *popPkt(BOOLEAN ANYONE, BOOLEAN LenType, U32 NOW) 
{
    struct lp_desc *timeoutBucket=NULL;
    struct lp_desc *desc;
    U16 loopCheck = 0;
    if(LenType & ~BIT_0)
    {
        printk("LenType != 0 or 1, Set it as 1\n");
        LenType = 1;
    }

	lp_lock();
    desc = LP_first_lp_desc(&lp_q_half);
    if(NULL == desc) return NULL;
    while(desc)
    {
        if(loopCheck++ > 100)
        {
            printk("infinite loop occurs in %s\n", __FUNCTION__);
            loopCheck = 0;
            break;
        }

		if(desc->sending == 1) {
            printk("Wow ~~~ lp_q_half has a sending one\n");
            desc = desc->next;
			continue;
		}
		if(desc->pktSize > LP_MAX_SIZE_IN_BUCKET[LenType]*8/10) {
            //printk("popPkt an exceed Size LP : %d\n", i);
            desc->sending = 1;
            if(!LP_midpop_lp_desc(&lp_q_half, desc))
                printk("What !? midpop is NULL ?\n");
            LP_push_lp_desc(&lp_q_sending, desc);
            lp_unlock();
			LP_CNT_LAST_LATENCY = NOW - desc->createTime;
            return desc;
		}
		else if(desc->pktCnt >= varLP_MAX_PKTS_IN_BUCKET) {
			//printk("popPkt an Full LP : %d\n", i);
			desc->sending = 1;
            if(!LP_midpop_lp_desc(&lp_q_half, desc))
                printk("What !? midpop is NULL ?\n");
            LP_push_lp_desc(&lp_q_sending, desc);
			lp_unlock();
			LP_CNT_LAST_LATENCY = NOW - desc->createTime;
			return desc;
		}
		if((NOW - desc->createTime) >= varLP_TIMEOUT ) {
			if(desc->pktCnt > 0) {
				//printk("Level 2\n");
				timeoutBucket = desc;
			}
		}
		else if(ANYONE && desc->pktCnt > 0) {
			desc->sending = 1;
            if(!LP_midpop_lp_desc(&lp_q_half, desc))
                printk("What !? midpop is NULL ?\n");
            LP_push_lp_desc(&lp_q_sending, desc);

			lp_unlock();
			LP_CNT_LAST_LATENCY = NOW - desc->createTime;
			return desc;	
		}
        desc = desc->next;
	}	
	if(timeoutBucket) {
		//printk("popPkt an timeout LP : %d\n", timeoutBucket);
		timeoutBucket->sending = 1;
        if(!LP_midpop_lp_desc(&lp_q_half, timeoutBucket))
            printk("What !? midpop is NULL ?\n");
        LP_push_lp_desc(&lp_q_sending, timeoutBucket);

		lp_unlock();
		LP_CNT_LAST_LATENCY = NOW - timeoutBucket->createTime;
		return timeoutBucket;
	}
	lp_unlock();
	return NULL;
}
void LP_CNT_SHOW() 
{
	printk(KERN_ERR "LP_CNT_PUSH_SUCC:%ld\n",LP_CNT_PUSH_SUCC);
	printk(KERN_ERR "LP_CNT_PUSH_FAIL:%ld\n",LP_CNT_PUSH_FAIL);
	printk(KERN_ERR "LP_CNT_POP_SUCC:%ld\n",LP_CNT_POP_SUCC);
	printk(KERN_ERR "LP_CNT_POP_FAIL:%ld\n",LP_CNT_POP_FAIL);
	printk(KERN_ERR "LP_CNT_PERIOD_POP_SUCC:%ld\n",LP_CNT_PERIOD_POP_SUCC);
	printk(KERN_ERR "LP_CNT_PERIOD_POP_FAIL:%ld\n",LP_CNT_PERIOD_POP_FAIL);
	printk(KERN_ERR "LP_CNT_LAST_NEW_BUCK_IDX:%ld\n",LP_CNT_LAST_NEW_BUCK_IDX);
	printk(KERN_ERR "LP_CNT_LAST_LATENCY:%ld\n", LP_CNT_LAST_LATENCY);
    printk(KERN_ERR "lp_q_empty cnt : %ld\n", lp_q_empty.count);
    printk(KERN_ERR "lp_q_half cnt : %ld\n", lp_q_half.count);
    printk(KERN_ERR "lp_q_sending cnt : %ld\n", lp_q_sending.count);
}
void initLP() 
{
    int i;
	memset(lp_bucket,0,sizeof(struct lp_desc)* LP_BUCKETS);
	spin_lock_init(&lp_spinlock);
    lp_q_sending.first = NULL;
    lp_q_sending.count = 0;
    lp_q_sending.last = NULL;
  
    lp_q_empty.first = NULL;
    lp_q_empty.last = NULL;
    lp_q_empty.count = 0;

    lp_q_half.first = NULL;
    lp_q_half.last = NULL;
    lp_q_half.count = 0;

    for(i=0;i<LP_BUCKETS;i++)
        LP_push_lp_desc(&lp_q_empty, &lp_bucket[i]);
	LP_CNT_PUSH_SUCC = 0;
	LP_CNT_PUSH_FAIL = 0;
	LP_CNT_POP_SUCC = 0;
	LP_CNT_POP_FAIL = 0;
	LP_CNT_PERIOD_POP_SUCC = 0;
	LP_CNT_PERIOD_POP_FAIL = 0;
	LP_CNT_LAST_NEW_BUCK_IDX = 0;
	LP_CNT_LAST_LATENCY = 0;
}
#endif


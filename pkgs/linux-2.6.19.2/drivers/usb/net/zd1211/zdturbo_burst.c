#if ZDCONF_LP_SUPPORT == 1
#include "zd1205.h"
#include "zdglobal.h"
extern Hash_t *sstByAid[MAX_RECORD];
extern zd_80211Obj_t dot11Obj;
static U32 Turbo_Burst_Status = 0;
static U32 OLD_B00, OLD_B04,OLD_B10, OLD_B14;

BOOLEAN Turbo_getBurst_Status(void)
{
    return Turbo_Burst_Status;
}
void Turbo_BurstOn(void)
{
    void *reg = dot11Obj.reg;
    printk("Enter %s\n", __FUNCTION__);
 
    OLD_B00 = dot11Obj.GetReg(reg, 0xB00);
    OLD_B04 = dot11Obj.GetReg(reg, 0xB04);
    OLD_B10 = dot11Obj.GetReg(reg, 0xB10);
    OLD_B14 = dot11Obj.GetReg(reg, 0xB14);

    dot11Obj.SetReg(reg, 0xB00, 0);
    dot11Obj.SetReg(reg, 0xB04, 0);
	if(mBssType == AP_BSS)
	{
	    dot11Obj.SetReg(reg, 0xB10, 0x000a0032);
	    dot11Obj.SetReg(reg, 0xB14, 0x00320032);
	}
	else if(mBssType == INFRASTRUCTURE_BSS)
	{
	    dot11Obj.SetReg(reg, 0xB10, 0x000a000a);
	    dot11Obj.SetReg(reg, 0xB14, 0x000a000a);
	}



    Turbo_Burst_Status = 1;

}
void Turbo_BurstOff(void)
{
    void *reg = dot11Obj.reg;
    printk("Enter %s\n", __FUNCTION__);
    dot11Obj.SetReg(reg, 0xB00, OLD_B00);
    dot11Obj.SetReg(reg, 0xB04, OLD_B04);
    dot11Obj.SetReg(reg, 0xB10, OLD_B10);
    dot11Obj.SetReg(reg, 0xB10, OLD_B14);

    Turbo_Burst_Status = 0;

}
BOOLEAN Turbo_BurstSTA_Check(void)
{
    U8 i;
    for(i=0;i<MAX_RECORD;i++)
        if(sstByAid[i]->bValid)
            if(sstByAid[i]->Turbo_Burst)
                return TRUE;
    return FALSE;
}
#endif

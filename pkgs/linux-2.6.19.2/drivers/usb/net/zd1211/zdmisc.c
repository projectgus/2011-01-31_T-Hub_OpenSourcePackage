#include "zdtypes.h"
#include "zdos.h"
U16 zd_get_LE_U16(U8 *p)
{
    if(p == NULL)
    {
        printk("In %s, *p is NULL\n", __FUNCTION__);
        return 0;
    } 
    return *p + ((*(p+1)) << 8);
}


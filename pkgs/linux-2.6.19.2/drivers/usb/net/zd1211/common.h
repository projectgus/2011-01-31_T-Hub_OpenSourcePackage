#ifndef ZD_COMMON_H
#define ZD_COMMON_H

#include <linux/if.h>

struct  zd_point
{
    caddr_t       pointer;
    __u16         length;
};

struct  zdreq
{
    union
    {
        char    ifrn_name[IFNAMSIZ];
    } ifr_ifrn;

    union
    {
        struct  zd_point data;
    } u;
};

typedef struct oid_wrap
{
    u16 request;
    u16 seq;

    union
    {
        struct
        {
            u16 status;
        } dev;

        struct
        {
            u32 oid;
            u32 status;
            u32 length;
            u8  data[512];
        } info;
    } u;

} oid_wrap_t;

#define ZD_GENERIC_OID_HDR_LEN \
((int) (&((struct oid_wrap *) 0)->u.info.data))

#endif

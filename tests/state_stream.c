#include "state.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    CHECK(ngs_crc32("123456789",9)==UINT32_C(0xcbf43926));
    unsigned char bytes[15], expected[]={0xab,0x34,0x12,0xef,0xcd,0xab,0x89,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    uint8_t a=0xab; uint16_t b=0x1234; uint32_t c=0x89abcdef; int64_t d=-2;
    ng_state_t s={bytes,sizeof(bytes),0,NGS_WRITE,0};
    ngs_u8(&s,&a);ngs_u16(&s,&b);ngs_u32(&s,&c);ngs_i64(&s,&d);
    CHECK(!s.error && s.pos==sizeof(bytes) && !memcmp(bytes,expected,sizeof(bytes)));
    a=0;b=0;c=0;d=0;s.mode=NGS_CHECK;s.pos=0;
    CHECK(ngs_u8(&s,&a)==0xab && ngs_u16(&s,&b)==0x1234 && ngs_u32(&s,&c)==0x89abcdef && ngs_i64(&s,&d)==-2);
    CHECK(a==0 && b==0 && c==0 && d==0);
    s.mode=NGS_READ;s.pos=0;ngs_u8(&s,&a);ngs_u16(&s,&b);ngs_u32(&s,&c);ngs_i64(&s,&d);
    CHECK(a==0xab && b==0x1234 && c==0x89abcdef && d==-2 && !s.error);
    ngs_u8(&s,&a);CHECK(s.error);
    s=(ng_state_t){bytes,0,0,NGS_READ,0};a=42;ngs_u8(&s,&a);CHECK(s.error && a==42);
    s=(ng_state_t){NULL,SIZE_MAX,0,NGS_COUNT,0};ngs_bytes(&s,NULL,1000);CHECK(s.pos==1000 && !s.error);
    puts("canonical state stream/CRC/bounds/read-only validation passed");return 0;
}

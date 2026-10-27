#include "game.h"
#include <stdio.h>
#include <stdint.h>
static uint16_t vram[0x10000];
static ng_extra_sprite_t out[64];
int main(void) {
 for(int i=0;i<20;i++) {
  vram[i*64]=0x7417; vram[i*64+1]=0x100;
  vram[0x8000+i]=0xf00;vram[0x8200+i]=14;vram[0x8400+i]=(uint16_t)(i*16*128);
 }
 int count=k_kof98.extend(vram,400,out,64); unsigned sum=0;
 for(int i=0;i<count;i++) if(out[i].art) for(int j=0;j<8192;j++)sum+=out[i].art[j];
 if (count!=6 || !sum) return 1;
 for (int repeat=0;repeat<3;repeat++) {
  unsigned again=0;count=k_kof98.extend(vram,400,out,64);
  for(int i=0;i<count;i++) if(out[i].art) for(int j=0;j<8192;j++) again+=out[i].art[j];
  if(again!=sum) return 1;
  k_kof98.unload();
 }
 puts("cold/warm art cache column pointers and repeated unload passed");return 0;
}

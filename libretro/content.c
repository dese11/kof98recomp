/* Local set identity, including the exact inputs used by the recompiler. */
#define _XOPEN_SOURCE 700
#include "content.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct rom_identity { const char *name; size_t size; const char *sha256; int system; };
#ifdef KOF98_HAVE_RECOMP
#include "rom_identity.h" /* private, generated in the build directory */
#else
static const struct rom_identity k_rom_identity[] = {{NULL, 0, NULL, 0}};
#endif

/* SHA-256, FIPS 180-4. Streaming: ROM data is never kept by the validator. */
static uint32_t ror(uint32_t x, unsigned n) { return (x >> n) | (x << (32-n)); }
static void block(uint32_t h[8], const unsigned char *p) {
    static const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    uint32_t w[64];
    for (int i=0;i<16;i++) w[i]=((uint32_t)p[4*i]<<24)|((uint32_t)p[4*i+1]<<16)|((uint32_t)p[4*i+2]<<8)|p[4*i+3];
    for (int i=16;i<64;i++) {
        uint32_t a=w[i-15], b=w[i-2];
        w[i]=w[i-16]+(ror(a,7)^ror(a,18)^(a>>3))+w[i-7]+(ror(b,17)^ror(b,19)^(b>>10));
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
    for (int i=0;i<64;i++) {
        uint32_t t=z+(ror(e,6)^ror(e,11)^ror(e,25))+((e&f)^(~e&g))+k[i]+w[i];
        uint32_t u=(ror(a,2)^ror(a,13)^ror(a,22))+((a&b)^(a&c)^(b&c));
        z=g; g=f; f=e; e=d+t; d=c; c=b; b=a; a=t+u;
    }
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
}

int kof98_join(char *out, size_t n, const char *dir, const char *name) {
    if (!dir || !*dir) return -1;
    int len=snprintf(out,n,"%s/%s",dir,name);
    return len<0 || (size_t)len>=n ? -1 : 0;
}
int kof98_file_exists(const char *dir, const char *name) {
    char path[KOF98_PATH_MAX]; struct stat st;
    return !kof98_join(path,sizeof(path),dir,name) && !stat(path,&st) && S_ISREG(st.st_mode);
}
int kof98_content_directory(const char *content, char *out, size_t n) {
    if (!content) return -1;
    const char *last=strrchr(content,'/');
    if (strcmp(last ? last+1 : content,"242-pn1.bin")) return -1;
    struct stat st;
    if (stat(content,&st) || !S_ISREG(st.st_mode)) return -1;
    char *parent=strdup(content);
    if (!parent) return -1;
    if (!last) strcpy(parent,".");
    else if (last==content) parent[1]='\0';
    else parent[last-content]='\0';
    /* Resolve the selected directory, retaining a P1 symlink within its set. */
    char *path=realpath(parent,NULL);
    free(parent);
    if (!path) return -1;
    int ok=strlen(path)+64<n; /* room for runtime filenames and subdirectories */
    if (ok) memcpy(out,path,strlen(path)+1);
    free(path);
    return ok ? 0 : -1;
}

static int validate_file(const char *dir, const struct rom_identity *rom, char *err, size_t n) {
    char path[KOF98_PATH_MAX];
    if (kof98_join(path,sizeof(path),dir,rom->name)) { snprintf(err,n,"Ruta demasiado larga: %s",rom->name); return -1; }
    FILE *f=fopen(path,"rb");
    if (!f) { snprintf(err,n,"Falta %s en %s",rom->name,dir); return -1; }
    uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    unsigned char data[64]; size_t total=0, got;
    while ((got=fread(data,1,64,f))==64) {
        total+=64;
        if (total>rom->size) break;
        block(h,data);
    }
    int ok=!ferror(f) && total+got==rom->size;
    /* Full blocks already contribute to total, including the oversized case. */
    if (got==64) ok=0;
    fclose(f);
    if (!ok) { snprintf(err,n,"Tamano incorrecto: %s (esperado %zu bytes)",rom->name,rom->size); return -1; }
    total+=got;
    data[got++]=0x80;
    if (got>56) { memset(data+got,0,64-got); block(h,data); got=0; }
    memset(data+got,0,56-got);
    uint64_t bits=(uint64_t)total*8;
    for (int i=0;i<8;i++) data[63-i]=(unsigned char)(bits>>(i*8));
    block(h,data);
    char hex[65];
    for (int i=0;i<8;i++) snprintf(hex+8*i,9,"%08x",h[i]);
    if (strcmp(hex,rom->sha256)) {
        snprintf(err,n,"Identidad distinta: %s. Usa el set con el que compilaste el core.",rom->name);
        return -1;
    }
    return 0;
}

static int complete_bios(const char *dir) {
    return kof98_file_exists(dir,"neo-po.bin") && kof98_file_exists(dir,"sfix.sfix") &&
           kof98_file_exists(dir,"sm1.sm1") && kof98_file_exists(dir,"000-lo.lo");
}
int kof98_validate_content(const char *rom, const char *system, char *bios, size_t cap, char *err, size_t n) {
    if (!k_rom_identity[0].name) { snprintf(err,n,"Este build no contiene codigo recompilado."); return -1; }
    char subdir[KOF98_PATH_MAX]; const char *selected=NULL;
    if (system && !kof98_join(subdir,sizeof(subdir),system,"kof98") && complete_bios(subdir)) selected=subdir;
    else if (system && complete_bios(system)) selected=system;
    else if (complete_bios(rom)) selected=rom;
    if (!selected) {
        snprintf(err,n,"BIOS incompleta: neo-po.bin, sfix.sfix, sm1.sm1 y 000-lo.lo en system/kof98, system o la carpeta del juego.");
        return -1;
    }
    if (strlen(selected)>=cap) { snprintf(err,n,"Ruta de BIOS demasiado larga."); return -1; }
    strcpy(bios,selected);
    for (size_t i=0;i<sizeof(k_rom_identity)/sizeof(k_rom_identity[0]);i++)
        if (validate_file(k_rom_identity[i].system ? bios : rom,&k_rom_identity[i],err,n)) return -1;
    return 0;
}

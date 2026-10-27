/* Local integration: snapshots, divergent futures, cross-load and rollback.
 * Test inputs are synthetic; no ROM, state or captured media is tracked. */
#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include <neogeorecomp/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "state.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#ifdef KOF98_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#else
#define ng_recomp_table NULL
#define ng_recomp_count 0
#endif
static ng_input_t input_at(unsigned frame) {
    ng_input_t in={0};
    if (frame>=2300 && frame<=2305) in.start1=1;
    if (frame>=2500 && frame<=3405 && (frame-2500)%90<=5) in.p1=NG_A;
    if (frame>=3900 && frame<=3960) in.p1=NG_RIGHT;
    if (frame>=4000 && frame<=4400) in.p1|=(frame%40<20) ? NG_A : NG_B|NG_D;
    if (frame>=4100 && frame<=4105) { in.start2=in.select2=1; in.p2=NG_LEFT|NG_B; }
    if (frame>4400) { in.p1=(frame%90<45) ? NG_LEFT|NG_C : NG_RIGHT|NG_A; in.p2=NG_D; }
    return in;
}
static uint64_t run(unsigned from, unsigned count, int divergent) {
    uint64_t h=NGS_HASH_INIT;
    for (unsigned i=0;i<count;i++) {
        ng_input_t in=input_at(from+i); ng_runtime_frame_t out;
        if (divergent) { in.p1^=NG_A|NG_RIGHT; in.p2^=NG_C|NG_LEFT; }
        CHECK(!ng_runtime_run_frame(&in,&out));
        h=ngs_hash64(h,out.pixels,out.pitch*out.height);
        h=ngs_hash64(h,out.audio,out.audio_frames*4);
    }
    return h;
}
static void save(void *data, size_t n) { CHECK(ng_runtime_state_size()==n && !ng_runtime_serialize(data,n)); }
static void unchanged_rejection(void *bad, size_t bad_size, void *before, void *after, size_t n) {
    save(before,n); CHECK(ng_runtime_unserialize(bad,bad_size)!=0); save(after,n); CHECK(!memcmp(before,after,n));
}
static double ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1000.0+t.tv_nsec/1e6; }
int main(int argc, char **argv) {
    CHECK(!ng_runtime_state_size() && ng_runtime_serialize(NULL,0) && ng_runtime_unserialize(NULL,0));
    if (argc<2) { puts("state APIs reject use without content"); return 0; }
    ng_runtime_options_t o={.rom_path=argv[1],.screen_width=400,.soundtrack_volume=70};
    if (argc>2 && strcmp(argv[2],"-")) o.soundtrack_path=argv[2];
    if (argc>4) o.screen_width=atoi(argv[4]);
    CHECK(!ng_runtime_load(&k_kof98,ng_recomp_table,ng_recomp_count,&o));
    size_t n=ng_runtime_state_size(); CHECK(n>250000 && n<1024*1024);
    unsigned char *a=malloc(n), *b=malloc(n), *c=malloc(n), *d=malloc(n); CHECK(a&&b&&c&&d);
    printf("snapshot size %zu; %dpx; %s\n",n,o.screen_width,o.soundtrack_path ? "AST" : "AES"); fflush(stdout);
    if (argc>3 && strcmp(argv[3],"test")) {
        const char *mode=argv[3]; CHECK(argc>5);
        FILE *f;
        if (!strcmp(mode,"write")) {
            run(0,3650,0); save(a,n);
            uint64_t hash=run(3650,240,0); save(b,n);
            f=fopen(argv[5],"wb"); CHECK(f);
            CHECK(fwrite(&hash,sizeof(hash),1,f)==1 && fwrite(a,1,n,f)==n && fwrite(b,1,n,f)==n); fclose(f);
        } else {
            f=fopen(argv[5],"rb"); CHECK(f); uint64_t expected;
            CHECK(fread(&expected,sizeof(expected),1,f)==1 && fread(a,1,n,f)==n && fread(b,1,n,f)==n); fclose(f);
            CHECK(!ng_runtime_unserialize(a,n)); save(c,n); CHECK(!memcmp(a,c,n));
            CHECK(run(3650,240,0)==expected); save(c,n); CHECK(!memcmp(b,c,n));
        }
        puts("cross-process snapshot passed");
    } else {
        static const unsigned checkpoints[]={0,1,20,180,600,1200,2280,2299,2390,3650,4200,4500};
        unsigned current=0;
        for (unsigned point=0;point<sizeof(checkpoints)/sizeof(checkpoints[0]);point++) {
            unsigned frame=checkpoints[point];
            if (frame<current) { CHECK(!ng_runtime_unserialize(a,n)); current=checkpoints[point-1]; }
            run(current,frame-current,0); current=frame; save(a,n);
            CHECK(!ng_runtime_serialize(c,n)); CHECK(!memcmp(a,c,n)); /* saving is pure */
            uint64_t expected=run(frame,120,0); save(b,n);
            run(frame+120,37,1); /* discard a different future */
            CHECK(!ng_runtime_unserialize(a,n)); save(c,n);
            if (memcmp(a,c,n)) { for(size_t i=0;i<n;i++) if(a[i]!=c[i]) { fprintf(stderr,"roundtrip offset %zu: %02x != %02x\n",i,a[i],c[i]); break; } }
            CHECK(!memcmp(a,c,n));
            CHECK(run(frame,120,0)==expected); save(c,n);
            if (memcmp(b,c,n)) { for(size_t i=0;i<n;i++) if(b[i]!=c[i]) { fprintf(stderr,"resim offset %zu: %02x != %02x\n",i,b[i],c[i]); break; } }
            CHECK(!memcmp(b,c,n));
            CHECK(!ng_runtime_unserialize(a,n)); current=frame;
            printf("checkpoint %u restored state/video/audio exactly\n",frame); fflush(stdout);
        }
        save(a,n);
        CHECK(ng_runtime_serialize(b,n-1)!=0 && ng_runtime_serialize(NULL,n)!=0);
        unchanged_rejection(a,n-1,b,c,n); unchanged_rejection(NULL,n,b,c,n);
        size_t offsets[]={0,8,12,16,24,32,36,40,n-1};
        for (size_t i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
            memcpy(d,a,n); d[offsets[i]]^=0x80; unchanged_rejection(d,n,b,c,n);
        }
        /* Valid CRC with an invalid CPU IPL must still be rejected atomically. */
        memcpy(d,a,n); d[48+17+72+7]=255;
        uint32_t crc=ngs_crc32(d+48,n-48); for(int i=0;i<4;i++) d[40+i]=(unsigned char)(crc>>(8*i));
        unchanged_rejection(d,n,b,c,n);
        /* Reset and reload cannot strand pointers or depend on a previous run. */
        ng_runtime_reset(); CHECK(!ng_runtime_unserialize(a,n)); save(c,n); CHECK(!memcmp(a,c,n));
        ng_runtime_unload(); CHECK(!ng_runtime_state_size());
        CHECK(!ng_runtime_load(&k_kof98,ng_recomp_table,ng_recomp_count,&o));
        CHECK(!ng_runtime_unserialize(a,n)); save(c,n); CHECK(!memcmp(a,c,n));
        o.screen_width=o.screen_width==400 ? 320 : 400;
        ng_runtime_unload(); CHECK(!ng_runtime_load(&k_kof98,ng_recomp_table,ng_recomp_count,&o));
        unchanged_rejection(a,n,b,c,n);
        ng_runtime_unload(); o.screen_width=o.screen_width==400 ? 320 : 400;
        CHECK(!ng_runtime_load(&k_kof98,ng_recomp_table,ng_recomp_count,&o)); CHECK(!ng_runtime_unserialize(a,n));
        double start=ms();
        for (int i=0;i<500;i++) { save(b,n); CHECK(!ng_runtime_unserialize(b,n)); }
        printf("500 save+restore pairs: %.3f ms/pair\n",(ms()-start)/500);
        /* Every short replay must converge; measure real rollback work too. */
        start=ms();
        for (unsigned i=0;i<120;i++) {
            save(a,n); uint64_t expected=run(4500+i,6,0); save(b,n);
            CHECK(!ng_runtime_unserialize(a,n)); CHECK(run(4500+i,6,0)==expected); save(c,n); CHECK(!memcmp(b,c,n));
        }
        printf("120 rollbacks of 6 frames: %.3f ms per double-execution check\n",(ms()-start)/120);
        puts("snapshot determinism, rejection, reset/reload and rollback stress passed");
    }
    free(a);free(b);free(c);free(d); ng_runtime_unload(); return 0;
}

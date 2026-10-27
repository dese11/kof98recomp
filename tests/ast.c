/* Synthetic PCM fixtures only; no proprietary soundtrack or ROM required. */
#include "ast.h"
#include "state.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
static const uint8_t m1[]="123456789";
static int restore_ast(const unsigned char *data, size_t n) {
    ng_state_t state={(unsigned char *)data,n,0,NGS_CHECK,0}; ast_state(&state);
    if (state.error) { ast_state_cancel(); return -1; }
    state.mode=NGS_READ; state.pos=0; ast_state(&state); ast_state_cancel(); return state.error ? -1 : 0;
}
static size_t save_ast(unsigned char *data, size_t n) {
    ng_state_t state={data,n,0,NGS_WRITE,0}; ast_state(&state); return state.error ? 0 : state.pos;
}
int main(int argc,char **argv) {
    CHECK(argc==2 || argc==3);
    const char *dir=argv[1];
    if (argc==3) {
        CHECK(ast_open(dir,"kof98",m1,9,100)!=0 && !ast_enabled());
        ast_close(); return 0;
    }
    CHECK(ast_open(dir,"other",m1,9,100)!=0);
    CHECK(ast_open(dir,"kof98",m1,8,100)!=0);
    CHECK(ast_open(dir,"kof98",m1,9,101)!=0);
    CHECK(!ast_enabled());
    CHECK(ast_open(dir,"kof98",m1,9,100)==0);
    CHECK(ast_command(0x30)==1 && ast_current_track()==13);
    int16_t sound[4000];
    for (int i=0;i<4000;i++) sound[i]=500;
    ast_mix(sound,1);
    CHECK(sound[0]==1500 && sound[1]==-500); /* effect + CD stereo, not a replacement of all sound */
    ast_set_volume(0);
    sound[0]=500; sound[1]=500; ast_mix(sound,1);
    CHECK(sound[0]==500 && sound[1]==500); /* live gain leaves effects */
    ast_set_volume(50); ast_command(0x20); ast_command(0x30);
    sound[0]=500; sound[1]=500; ast_mix(sound,1);
    CHECK(sound[0]==1000 && sound[1]==0);
    ast_set_volume(101); ast_command(0x20); ast_command(0x30);
    sound[0]=500; sound[1]=500; ast_mix(sound,1);
    CHECK(sound[0]==1000 && sound[1]==0); /* invalid gain ignored */
    ast_set_volume(100);
    memset(sound,0,sizeof(sound)); ast_mix(sound,2000);
    CHECK(ast_current_track()==13); /* tiny track looped many times */
    CHECK(ast_command(0xd0)==0 && ast_current_track()==13); /* sound effect passes through */
    CHECK(ast_command(0x20)==0 && ast_current_track()==0);
    CHECK(ast_command(0x14)==0);
    CHECK(ast_command(0x30)==0 && ast_command(0x31)==0); /* both argument bytes pass */
    CHECK(ast_current_track()==0);
    CHECK(ast_command(0x30)==1);
    CHECK(ast_command(0x0e)==0 && ast_command(0x31)==0 && ast_current_track()==13);
    CHECK(ast_command(0x0a)==0 && ast_command(0xff)==0);
    memset(sound,0,sizeof(sound)); ast_mix(sound,2000);
    CHECK(ast_current_track()==13);
    CHECK(ast_command(0x11)==0); /* cancel fade */
    for(int n=0;n<40;n++){memset(sound,0,sizeof(sound));ast_mix(sound,2000);}
    CHECK(ast_current_track()==13);
    ast_command(0x0a); ast_command(0xff);
    for(int n=0;n<40;n++){memset(sound,0,sizeof(sound));ast_mix(sound,2000);}
    CHECK(ast_current_track()==0);
    CHECK(ast_command(0x31)==1 && ast_current_track()==14);
    for(int i=0;i<4000;i++) sound[i]=500;
    ast_mix(sound,2000);
    CHECK(ast_current_track()==0 && sound[3999]==500); /* one shot ends; effects continue */
    ast_command(0x14); ast_reset();
    CHECK(ast_command(0x30)==1); /* reset discarded pending arguments */
    ast_command(3); CHECK(ast_current_track()==0);
    ast_close(); ast_close();
    CHECK(ast_open(dir,"kof98",m1,9,0)==0 && ast_command(0x30)==1);
    sound[0]=123; sound[1]=-234; ast_mix(sound,1);
    CHECK(sound[0]==123 && sound[1]==-234); /* external volume zero preserves native effects */
    ast_close();
    CHECK(ast_open(dir,"kof98",m1,9,100)==0 && ast_command(0x30)==1);
    unsigned char state[64], again[64];
    int16_t expected[4000];
    ast_mix(sound,13); ast_command(0x0a); ast_command(128);
    size_t size=save_ast(state,sizeof(state)); CHECK(size>0);
    memset(expected,0,sizeof(expected)); ast_mix(expected,2000);
    ast_command(0x31); ast_mix(sound,2000); /* different song, then ended one-shot */
    CHECK(!restore_ast(state,size)); CHECK(save_ast(again,sizeof(again))==size && !memcmp(state,again,size));
    memset(sound,0,sizeof(sound)); ast_mix(sound,2000); CHECK(!memcmp(sound,expected,sizeof(sound)));
    ast_command(0x14); ast_command(0x30); size=save_ast(state,sizeof(state));
    ast_command(0x31); ast_command(0x30);
    CHECK(!restore_ast(state,size)); CHECK(ast_command(0x31)==0); /* remaining argument restored */
    ast_command(0x31); ast_mix(sound,1); size=save_ast(state,sizeof(state));
    memset(expected,0,sizeof(expected)); ast_mix(expected,2000);
    ast_command(0x30); CHECK(!restore_ast(state,size));
    memset(sound,0,sizeof(sound)); ast_mix(sound,2000);
    CHECK(!memcmp(sound,expected,sizeof(sound)) && !ast_current_track());
    ast_command(0x30); size=save_ast(state,sizeof(state)); ast_command(0x31);
    CHECK(save_ast(again,sizeof(again))==size);
    char path[1400], backup[1400];
    snprintf(path,sizeof(path),"%s/track13.wav",dir); snprintf(backup,sizeof(backup),"%s/track13.tmp",dir);
    CHECK(!rename(path,backup)); CHECK(restore_ast(state,size)!=0);
    unsigned char unchanged[64]; CHECK(save_ast(unchanged,sizeof(unchanged))==size && !memcmp(again,unchanged,size));
    CHECK(!rename(backup,path)); CHECK(!restore_ast(state,size));
    ast_set_volume(0); CHECK(!restore_ast(state,size)); sound[0]=123;sound[1]=-234;ast_mix(sound,1);
    CHECK(sound[0]==123 && sound[1]==-234); /* host gain survives a restore */
    ast_close();
    puts("AST mixing, resampling, loop, one-shot, controls, arguments and lifecycle passed");
    return 0;
}

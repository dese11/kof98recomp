/* Exercise the actual shared-object API; no private symbols or game data here. */
#include "libretro.h"
#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APIS(X) X(api_version) X(set_environment) X(set_video_refresh) X(set_audio_sample) \
 X(set_audio_sample_batch) X(set_input_poll) X(set_input_state) X(init) X(deinit) \
 X(get_system_info) X(get_system_av_info) X(set_controller_port_device) X(reset) X(run) \
 X(serialize_size) X(serialize) X(unserialize) X(cheat_reset) X(cheat_set) X(load_game) \
 X(load_game_special) X(unload_game) X(get_region) X(get_memory_data) X(get_memory_size)
#define DECLARE(n) static __typeof__(retro_##n) *r_##n;
APIS(DECLARE)
#undef DECLARE
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static const char *system_dir, *reject_text;
static char last_error[1400];
static const char *music="original", *gain="70";
static unsigned width=400, frame, videos, polls, queries[2], old_options;
static size_t samples;
static uint64_t hash, native_blocks;
static int update, allow_pixels=1;
static uint64_t hash_bytes(uint64_t h, const void *data, size_t n) {
    const unsigned char *p=data;
    while (n--) { h^=*p++; h*=UINT64_C(1099511628211); }
    return h;
}
static void logger(enum retro_log_level level, const char *fmt, ...) {
    char line[2048]; va_list ap; va_start(ap,fmt); vsnprintf(line,sizeof(line),fmt,ap); va_end(ap);
    fputs(line,stderr);
    if (level==RETRO_LOG_ERROR) snprintf(last_error,sizeof(last_error),"%.1399s",line);
    char *p=strstr(line,"bloques recompilados=");
    if (p) native_blocks+=strtoull(p+strlen("bloques recompilados="),NULL,10);
}
static bool environment(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: ((struct retro_log_callback *)data)->log=logger; return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: *(unsigned *)data=old_options ? 0 : 2; return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2: {
        struct retro_core_options_v2 *o=data; CHECK(o->definitions && o->definitions[0].key); return true;
    }
    case RETRO_ENVIRONMENT_SET_VARIABLES: CHECK(((struct retro_variable *)data)->key); return true;
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: CHECK(!*(bool *)data); return true;
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: {
        struct retro_controller_info *c=data; CHECK(c[0].num_types==1 && c[1].num_types==1 && !c[2].num_types); return true;
    }
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: {
        struct retro_input_descriptor *d=data; unsigned count=0;
        for (;d->description;d++) { CHECK(d->port<2); count++; }
        CHECK(count==20); return true;
    }
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: CHECK(*(enum retro_pixel_format *)data==RETRO_PIXEL_FORMAT_XRGB8888); return allow_pixels;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *v=data;
        if (!strcmp(v->key,"kof98_widescreen")) v->value=width==400 ? "enabled" : "disabled";
        else if (!strcmp(v->key,"kof98_soundtrack")) v->value=music;
        else if (!strcmp(v->key,"kof98_ast_volume")) v->value=gain;
        else return false;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: *(bool *)data=update; update=0; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: *(const char **)data=system_dir; return system_dir!=NULL;
    case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY: *(const char **)data=NULL; return false;
    case RETRO_ENVIRONMENT_SET_MESSAGE: fprintf(stderr,"message: %s\n",((struct retro_message *)data)->msg); return true;
    default: return false;
    }
}
static void video(const void *data, unsigned w, unsigned h, size_t pitch) {
    CHECK(data && w==width && h==224 && pitch==width*4);
    hash=hash_bytes(hash,data,pitch*h); videos++;
}
static size_t audio(const int16_t *data, size_t n) {
    CHECK(data && n<1000); samples+=n;
    hash=hash_bytes(hash,data,n*4); return n;
}
static void single(int16_t l, int16_t r) { int16_t data[2]={l,r}; audio(data,1); }
static void poll(void) { polls++; }
static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    CHECK(port<2 && device==RETRO_DEVICE_JOYPAD && index==0); queries[port]++;
    if (!port) {
        if (id==RETRO_DEVICE_ID_JOYPAD_START && frame>=2300 && frame<=2305) return 1;
        if (id==RETRO_DEVICE_ID_JOYPAD_B && frame>=2500 && frame<=3405 && (frame-2500)%90<=5) return 1;
        if (id==RETRO_DEVICE_ID_JOYPAD_RIGHT && frame>=3900 && frame<=3960) return 1;
        if (id==RETRO_DEVICE_ID_JOYPAD_B && frame>=4000 && frame<=4005) return 1;
        if (id==RETRO_DEVICE_ID_JOYPAD_A && frame>=4060 && frame<=4065) return 1;
        if (id==RETRO_DEVICE_ID_JOYPAD_Y && frame>=4120 && frame<=4125) return 1;
        if (id==RETRO_DEVICE_ID_JOYPAD_X && frame>=4180 && frame<=4185) return 1;
    } else if (frame>=4100 && frame<=4105) {
        return id==RETRO_DEVICE_ID_JOYPAD_START || id==RETRO_DEVICE_ID_JOYPAD_SELECT ||
               id==RETRO_DEVICE_ID_JOYPAD_LEFT || id==RETRO_DEVICE_ID_JOYPAD_A;
    }
    return 0;
}
static void clear_frame(void) { samples=videos=polls=queries[0]=queries[1]=0; }
int main(int argc, char **argv) {
    CHECK(argc>=2);
    void *lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if (!lib) { fprintf(stderr,"%s\n",dlerror()); return 1; }
#define LOAD(n) do { void *p=dlsym(lib,"retro_" #n); CHECK(p); memcpy(&r_##n,&p,sizeof(p)); } while (0);
    APIS(LOAD)
#undef LOAD
    r_set_environment(environment); r_set_video_refresh(video); r_set_audio_sample(single);
    r_set_audio_sample_batch(audio); r_set_input_poll(poll); r_set_input_state(input); r_init();
    CHECK(r_api_version()==RETRO_API_VERSION);
    struct retro_system_info info; r_get_system_info(&info);
    CHECK(info.need_fullpath && info.block_extract && !strcmp(info.valid_extensions,"bin"));
    CHECK(!r_serialize_size() && !r_serialize(NULL,0) && !r_unserialize(NULL,0));
    CHECK(!r_get_memory_data(RETRO_MEMORY_SAVE_RAM) && !r_get_memory_size(RETRO_MEMORY_SAVE_RAM));
    CHECK(!r_load_game_special(0,NULL,0) && r_get_region()==RETRO_REGION_NTSC);
    r_cheat_reset(); r_cheat_set(0,true,"ignored"); r_reset(); r_unload_game(); r_unload_game();
    CHECK(!r_load_game(NULL));
    struct retro_game_info game={.path="/nonexistent/242-pn1.bin"}; CHECK(!r_load_game(&game));
    if (argc>2 && !strcmp(argv[2],"--reject")) {
        CHECK(argc>=5); game.path=argv[3]; reject_text=argv[4];
        if (argc>5) system_dir=argv[5];
        CHECK(!r_load_game(&game)); CHECK(strstr(last_error,reject_text));
        r_deinit(); CHECK(!dlclose(lib)); puts("rejected content as expected"); return 0;
    }
    if (argc>2) {
        if (argc>3) system_dir=argv[3];
        if (argc>4) music=argv[4];
        char path[1024]; CHECK(snprintf(path,sizeof(path),"%s/242-pn1.bin",argv[2])<(int)sizeof(path)); game.path=path;
        allow_pixels=0; CHECK(!r_load_game(&game)); allow_pixels=1;
        uint64_t expected[2]={0};
        for (unsigned run=0;run<4;run++) {
            width=(run%2) ? 400 : 320; old_options=run%2;
            r_set_environment(environment);
            CHECK(r_load_game(&game)); CHECK(!r_load_game(&game));
            struct retro_system_av_info av; r_get_system_av_info(&av);
            CHECK(av.geometry.base_width==width && av.geometry.base_height==224 && av.geometry.max_width==400);
            CHECK(av.timing.fps>59.18 && av.timing.fps<59.19 && av.timing.sample_rate>55555.55 && av.timing.sample_rate<55555.56);
            hash=UINT64_C(14695981039346656037); clear_frame();
            for (frame=0;frame<4200;frame++) r_run();
            CHECK(videos==4200 && polls==4200 && queries[0]==42000 && queries[1]==42000 && samples==3942400);
            if (run<2) expected[run%2]=hash; else CHECK(hash==expected[run%2]);
            printf("libretro run %u width %u hash %016llx audio %zu\n",run,width,(unsigned long long)hash,samples);
            size_t state_size=r_serialize_size(); CHECK(state_size>250000 && state_size<1024*1024);
            unsigned char *a=malloc(state_size), *b=malloc(state_size), *c=malloc(state_size); CHECK(a&&b&&c);
            CHECK(r_serialize(a,state_size));
            hash=UINT64_C(14695981039346656037);
            for (frame=4200;frame<4320;frame++) r_run();
            uint64_t future=hash; CHECK(r_serialize(b,state_size));
            CHECK(r_unserialize(a,state_size)); CHECK(r_serialize(c,state_size)); CHECK(!memcmp(a,c,state_size));
            hash=UINT64_C(14695981039346656037);
            for (frame=4200;frame<4320;frame++) r_run();
            CHECK(hash==future && r_serialize(c,state_size) && !memcmp(b,c,state_size));
            CHECK(!r_unserialize(a,state_size-1)); CHECK(r_serialize(c,state_size) && !memcmp(b,c,state_size));
            free(a);free(b);free(c);
            r_reset(); clear_frame(); frame=0; r_run(); CHECK(videos==1 && samples<1000);
            /* Disconnected pads must not be polled; reconnection must work. */
            r_set_controller_port_device(1,RETRO_DEVICE_NONE); clear_frame(); r_run(); CHECK(!queries[1] && queries[0]==10);
            r_set_controller_port_device(1,RETRO_DEVICE_JOYPAD); clear_frame();
            /* Per-sample fallback and live volume updates. Geometry waits for reload. */
            r_set_audio_sample_batch(NULL); gain="0"; update=1; r_run(); CHECK(samples>900 && queries[1]==10);
            r_set_audio_sample_batch(audio); gain="70"; update=1;
            r_unload_game(); r_unload_game(); clear_frame(); r_run(); CHECK(!videos && !samples && !polls);
        }
        CHECK(native_blocks>0);
    }
    r_deinit(); CHECK(!dlclose(lib)); puts("libretro contract passed"); return 0;
}

/* libretro owns presentation, devices and pacing. The runtime advances one frame. */
#include "libretro.h"
#include "content.h"
#include "game.h"
#include <neogeorecomp/runtime.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef KOF98_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#else
#define ng_recomp_table NULL
#define ng_recomp_count 0
#endif

static retro_environment_t environment;
static retro_video_refresh_t video;
static retro_audio_sample_t sample;
static retro_audio_sample_batch_t batch;
static retro_input_poll_t poll;
static retro_input_state_t input;
static retro_log_printf_t logger;
static unsigned ports[2]={RETRO_DEVICE_JOYPAD,RETRO_DEVICE_JOYPAD};
static int loaded, width=400;
static unsigned volume=70;
static char rom_dir[KOF98_PATH_MAX], bios_dir[KOF98_PATH_MAX];
static char asset_dir[KOF98_PATH_MAX], ast_dir[KOF98_PATH_MAX];

static void log_message(enum retro_log_level level, const char *fmt, ...) {
    char text[1400]; va_list ap;
    va_start(ap,fmt); vsnprintf(text,sizeof(text),fmt,ap); va_end(ap);
    if (logger) logger(level,"[KOF98 Recomp] %s\n",text);
    else fprintf(stderr,"[KOF98 Recomp] %s\n",text);
}
static void message(const char *text) {
    struct retro_message m={text,360};
    if (environment) environment(RETRO_ENVIRONMENT_SET_MESSAGE,&m);
}
static bool fail(const char *text) { log_message(RETRO_LOG_ERROR,"%s",text); message(text); return false; }

static struct retro_core_option_v2_definition definitions[] = {
    {.key="kof98_widescreen", .desc="Pantalla panoramica (recargar contenido)",
     .info="Amplia la escena a 400x224. Cierra y vuelve a cargar el contenido para aplicar el cambio.",
     .values={{"enabled","Activada"},{"disabled","Desactivada"},{NULL,NULL}}, .default_value="enabled"},
    {.key="kof98_soundtrack", .desc="Musica (recargar contenido)",
     .info="Automatico usa el paquete AST si esta instalado. AST exige el paquete local del CD. Cierra y vuelve a cargar el contenido para aplicar el cambio.",
     .values={{"auto","Automatico"},{"original","Original AES"},{"ast","AST Neo Geo CD"},{NULL,NULL}}, .default_value="auto"},
    {.key="kof98_ast_volume", .desc="Volumen de musica AST",
     .info="Se aplica durante la partida. Conserva el volumen de voces y efectos.",
     .values={{"0",NULL},{"10",NULL},{"20",NULL},{"30",NULL},{"40",NULL},{"50",NULL},
              {"60",NULL},{"70",NULL},{"80",NULL},{"90",NULL},{"100",NULL},{NULL,NULL}}, .default_value="70"},
    {0}
};
static struct retro_core_options_v2 options={NULL,definitions};
static const struct retro_variable legacy_options[] = {
    {"kof98_widescreen","Pantalla panoramica (recargar contenido); enabled|disabled"},
    {"kof98_soundtrack","Musica (recargar contenido); auto|original|ast"},
    {"kof98_ast_volume","Volumen de musica AST; 70|0|10|20|30|40|50|60|80|90|100"},
    {NULL,NULL}
};
static const struct retro_controller_description controllers[]={{"RetroPad",RETRO_DEVICE_JOYPAD}};
static const struct retro_controller_info controller_info[]={{controllers,1},{controllers,1},{NULL,0}};
#define DESCRIPTORS(p) \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_UP,"Arriba"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_DOWN,"Abajo"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_LEFT,"Izquierda"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_RIGHT,"Derecha"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_B,"A - Golpe debil"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_A,"B - Patada debil"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_Y,"C - Golpe fuerte"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_X,"D - Patada fuerte"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_START,"Start"}, \
    {p,RETRO_DEVICE_JOYPAD,0,RETRO_DEVICE_ID_JOYPAD_SELECT,"Select (AES)"}
static const struct retro_input_descriptor descriptors[]={DESCRIPTORS(0),DESCRIPTORS(1),{0}};
#undef DESCRIPTORS

static const char *variable(const char *key, const char *fallback) {
    struct retro_variable v={key,NULL};
    return environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE,&v) && v.value ? v.value : fallback;
}
static unsigned get_volume(void) {
    const char *v=variable("kof98_ast_volume","70"); char *end;
    unsigned long value=strtoul(v,&end,10);
    return *v && !*end && value<=100 ? (unsigned)value : 70;
}
static const char *directory(unsigned command) {
    const char *p=NULL;
    if (environment) environment(command,&p);
    return p && *p ? p : NULL;
}
static int find_subdirectory(char *out, const char *root, const char *relative, const char *marker) {
    char path[KOF98_PATH_MAX];
    if (kof98_join(path,sizeof(path),root,relative) || !kof98_file_exists(path,marker)) return 0;
    strcpy(out,path); return 1;
}

void retro_set_environment(retro_environment_t cb) {
    environment=cb;
    if (!cb) return;
    unsigned version=0; bool no_game=false;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME,&no_game);
    cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION,&version);
    if (version>=2) cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,&options);
    else cb(RETRO_ENVIRONMENT_SET_VARIABLES,(void *)legacy_options);
    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO,(void *)controller_info);
    cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS,(void *)descriptors);
}
void retro_set_video_refresh(retro_video_refresh_t cb) { video=cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { sample=cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { batch=cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll=cb; }
void retro_set_input_state(retro_input_state_t cb) { input=cb; }
unsigned retro_api_version(void) { return RETRO_API_VERSION; }
void retro_init(void) {
    struct retro_log_callback l={0};
    if (environment && environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE,&l)) logger=l.log;
    log_message(RETRO_LOG_INFO,"Inicializado. Savestates, rewind, run-ahead y netplay aun no disponibles.");
}
void retro_unload_game(void) {
    if (loaded) {
        uint64_t native,interpreted;
        ng_runtime_counters(&native,&interpreted);
        log_message(RETRO_LOG_INFO,"Descarga: bloques recompilados=%llu; instrucciones interpretadas=%llu.",
                    (unsigned long long)native,(unsigned long long)interpreted);
    }
    ng_runtime_unload(); loaded=0;
    kof98_set_asset_directory(NULL);
    rom_dir[0]=bios_dir[0]=asset_dir[0]=ast_dir[0]='\0';
}
void retro_deinit(void) {
    retro_unload_game(); logger=NULL; width=400; volume=70;
    ports[0]=ports[1]=RETRO_DEVICE_JOYPAD;
}
void retro_get_system_info(struct retro_system_info *info) {
    memset(info,0,sizeof(*info));
    info->library_name="KOF98 Recomp"; info->library_version="0.2.0";
    info->valid_extensions="bin"; info->need_fullpath=true; info->block_extract=true;
}
void retro_get_system_av_info(struct retro_system_av_info *info) {
    memset(info,0,sizeof(*info));
    info->geometry.base_width=(unsigned)width; info->geometry.base_height=224;
    info->geometry.max_width=400; info->geometry.max_height=224;
    info->geometry.aspect_ratio=(float)width/224.0f;
    info->timing.fps=NG_RUNTIME_FPS; info->timing.sample_rate=NG_RUNTIME_AUDIO_RATE;
}
void retro_set_controller_port_device(unsigned port, unsigned device) {
    if (port<2) ports[port]=device==RETRO_DEVICE_JOYPAD ? device : RETRO_DEVICE_NONE;
}
bool retro_load_game(const struct retro_game_info *game) {
    if (loaded) return fail("Cierra el contenido actual antes de cargar otra partida.");
    if (!game || kof98_content_directory(game->path,rom_dir,sizeof(rom_dir)))
        return fail("Selecciona 242-pn1.bin dentro del set AES kof98h completo y extraido.");
    if (!ng_recomp_count) return fail("Este build no contiene codigo recompilado.");
    enum retro_pixel_format format=RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment || !environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT,&format))
        return fail("El frontend no acepta video XRGB8888.");
    char error[1400]; const char *system=directory(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY);
    if (kof98_validate_content(rom_dir,system,bios_dir,sizeof(bios_dir),error,sizeof(error))) return fail(error);
    asset_dir[0]=ast_dir[0]='\0';
    if (!find_subdirectory(asset_dir,system,"kof98/assets","stone.png") &&
        !find_subdirectory(asset_dir,rom_dir,"assets","stone.png"))
        find_subdirectory(asset_dir,directory(RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY),"kof98/assets","stone.png");
    kof98_set_asset_directory(asset_dir[0] ? asset_dir : NULL);
    if (!asset_dir[0]) log_message(RETRO_LOG_WARN,"No se encontraron assets: los margenes usan el reflejo de respaldo.");
    if (!find_subdirectory(ast_dir,system,"kof98/ast","soundtrack.txt"))
        find_subdirectory(ast_dir,rom_dir,"ast","soundtrack.txt");
    const char *music=variable("kof98_soundtrack","auto");
    if (!strcmp(music,"ast") && !ast_dir[0]) return fail("Falta AST: instala el paquete en system/kof98/ast o en ast junto a las ROM.");
    int ast=strcmp(music,"original") && ast_dir[0];
    width=!strcmp(variable("kof98_widescreen","enabled"),"disabled") ? 320 : 400;
    volume=get_volume();
    ng_runtime_options_t o={.rom_path=rom_dir,.bios_path=bios_dir,.screen_width=width,
        .soundtrack_path=ast ? ast_dir : NULL,.soundtrack_volume=volume};
    if (ng_runtime_load(&k_kof98,ng_recomp_table,ng_recomp_count,&o))
        return fail("No se pudo cargar el juego o el paquete AST. Revisa el registro de RetroArch.");
    loaded=1;
    log_message(RETRO_LOG_INFO,"Cargado %dx224; musica=%s; entradas recompiladas=%zu; BIOS=%s; assets=%s.",
                width,ast ? "AST" : "AES",ng_recomp_count,bios_dir,asset_dir);
    message(ast ? "KOF98 Recomp - AST Neo Geo CD" : "KOF98 Recomp - musica original AES");
    return true;
}
void retro_reset(void) { if (loaded) { ng_runtime_reset(); log_message(RETRO_LOG_INFO,"Reset AES."); } }
static int pressed(unsigned port, unsigned id) {
    return input && ports[port]==RETRO_DEVICE_JOYPAD && input(port,RETRO_DEVICE_JOYPAD,0,id)!=0;
}
void retro_run(void) {
    if (!loaded) return;
    bool updated=false;
    if (environment && environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE,&updated) && updated) {
        volume=get_volume(); ng_runtime_set_soundtrack_volume(volume);
    }
    if (poll) poll();
    ng_input_t in={0};
    const unsigned ids[]={RETRO_DEVICE_ID_JOYPAD_UP,RETRO_DEVICE_ID_JOYPAD_DOWN,
        RETRO_DEVICE_ID_JOYPAD_LEFT,RETRO_DEVICE_ID_JOYPAD_RIGHT,
        RETRO_DEVICE_ID_JOYPAD_B,RETRO_DEVICE_ID_JOYPAD_A,RETRO_DEVICE_ID_JOYPAD_Y,RETRO_DEVICE_ID_JOYPAD_X};
    for (unsigned p=0;p<2;p++) {
        uint8_t bits=0;
        for (unsigned i=0;i<8;i++) if (pressed(p,ids[i])) bits|=(uint8_t)(1u<<i);
        if (!p) { in.p1=bits; in.start1=pressed(p,RETRO_DEVICE_ID_JOYPAD_START); in.select1=pressed(p,RETRO_DEVICE_ID_JOYPAD_SELECT); }
        else { in.p2=bits; in.start2=pressed(p,RETRO_DEVICE_ID_JOYPAD_START); in.select2=pressed(p,RETRO_DEVICE_ID_JOYPAD_SELECT); }
    }
    ng_runtime_frame_t frame;
    if (ng_runtime_run_frame(&in,&frame)) { fail("Error al ejecutar un frame."); return; }
    if (video) video(frame.pixels,frame.width,frame.height,frame.pitch);
    if (batch) batch(frame.audio,frame.audio_frames);
    else if (sample) for (size_t i=0;i<frame.audio_frames;i++) sample(frame.audio[i*2],frame.audio[i*2+1]);
}
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t n) { (void)data; (void)n; return false; }
bool retro_unserialize(const void *data, size_t n) { (void)data; (void)n; return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned i, bool enabled, const char *code) { (void)i; (void)enabled; (void)code; }
bool retro_load_game_special(unsigned type, const struct retro_game_info *game, size_t n) { (void)type; (void)game; (void)n; return false; }
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
void *retro_get_memory_data(unsigned id) { (void)id; return NULL; }
size_t retro_get_memory_size(unsigned id) { (void)id; return 0; }

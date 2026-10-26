/* Local ROM integration test; also exercises invalid loads without ROMs. */
#include "game.h"
#include <neogeorecomp/bus.h>
#include <neogeorecomp/runtime.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
#ifdef KOF98_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#else
#define ng_recomp_table NULL
#define ng_recomp_count 0
#endif

static uint64_t hash_bytes(uint64_t h, const void *data, size_t size) {
    const unsigned char *p = data;
    while (size--) { h ^= *p++; h *= UINT64_C(1099511628211); }
    return h;
}

static ng_input_t input_at(int frame) {
    ng_input_t in = {0};
    if (frame >= 2300 && frame <= 2305) in.start1 = 1;
    if (frame >= 2500 && frame <= 3405 && (frame - 2500) % 90 <= 5) in.p1 = NG_A;
    if (frame >= 3900 && frame <= 3960) in.p1 = NG_RIGHT;
    if (frame >= 4000 && frame <= 4005) in.p1 = NG_A;
    if (frame >= 4060 && frame <= 4065) in.p1 = NG_B;
    if (frame >= 4120 && frame <= 4125) in.p1 = NG_C;
    if (frame >= 4180 && frame <= 4185) in.p1 = NG_D;
    if (frame >= 4100 && frame <= 4105) { in.start2 = in.select2 = 1; in.p2 = NG_LEFT | NG_B; }
    return in;
}

int main(int argc, char **argv) {
    ng_runtime_frame_t out;
    ng_runtime_options_t o = {.rom_path = "/nonexistent/kof98-test", .screen_width = 400};
    ng_runtime_unload(); ng_runtime_unload(); ng_runtime_reset();
    memset(&out, 0xAA, sizeof(out));
    CHECK(ng_runtime_run_frame(NULL, &out) != 0 && out.pixels == NULL && out.audio_frames == 0);
    CHECK(ng_runtime_load(&k_kof98, NULL, 1, &o) != 0);
    CHECK(ng_runtime_load(&k_kof98, ng_recomp_table, ng_recomp_count, &o) != 0);
    o.screen_width = 399;
    CHECK(ng_runtime_load(&k_kof98, ng_recomp_table, ng_recomp_count, &o) != 0);
    if (argc < 2) { puts("invalid-load lifecycle passed (no proprietary data)"); return 0; }
    o.rom_path = argv[1];
    o.screen_width = 400;
    o.bios_path = "/nonexistent/kof98-bios-test";
    CHECK(ng_runtime_load(&k_kof98, ng_recomp_table, ng_recomp_count, &o) != 0);
    o.bios_path = NULL;
    if (argc > 2) { o.soundtrack_path=argv[2]; o.soundtrack_volume=70; }
    uint64_t expected[2] = {0};
    for (int run = 0; run < 4; run++) {
        int mode = run % 2;
        o.screen_width = mode ? 400 : 320;
        CHECK(ng_runtime_load(&k_kof98, ng_recomp_table, ng_recomp_count, &o) == 0);
        CHECK(ng_runtime_load(&k_kof98, ng_recomp_table, ng_recomp_count, &o) != 0);
        uint64_t hash = UINT64_C(14695981039346656037);
        size_t audio_frames = 0;
        size_t audio_min = 4096, audio_max = 0;
        for (int frame = 0; frame < 4200; frame++) {
            ng_input_t input = input_at(frame);
            CHECK(ng_runtime_run_frame(&input, &out) == 0);
            CHECK(out.width == (unsigned)o.screen_width && out.height == 224);
            CHECK(out.pitch == out.width * 4 && out.pixels && out.audio);
            CHECK(out.audio_frames <= 4096);
            if (out.audio_frames < audio_min) audio_min = out.audio_frames;
            if (out.audio_frames > audio_max) audio_max = out.audio_frames;
            CHECK(ng_r8(0x300000) == (uint8_t)~input.p1);
            CHECK(ng_r8(0x340000) == (uint8_t)~input.p2);
            hash = hash_bytes(hash, out.pixels, out.pitch * out.height);
            hash = hash_bytes(hash, out.audio, out.audio_frames * 4);
            audio_frames += out.audio_frames;
        }
        uint64_t native, interpreted;
        ng_runtime_counters(&native, &interpreted);
#ifdef KOF98_HAVE_RECOMP
        CHECK(native > 0);
#endif
        /* Samples can arrive in bursts across frame boundaries. */
        CHECK(audio_frames == 3942400);
        if (run < 2) expected[mode] = hash;
        else CHECK(hash == expected[mode]);
        printf("run %d width %d hash %016llx native %llu interpreted %llu audio %zu [%zu,%zu]\n", run,
               o.screen_width, (unsigned long long)hash, (unsigned long long)native,
               (unsigned long long)interpreted, audio_frames, audio_min, audio_max);
        ng_runtime_reset();
        CHECK(ng_runtime_run_frame(NULL, &out) == 0 && out.audio_frames < 1000);
        ng_runtime_unload(); ng_runtime_unload();
        ng_runtime_counters(&native, &interpreted);
        CHECK(native == 0 && interpreted == 0);
        CHECK(ng_runtime_run_frame(NULL, &out) != 0);
    }
    puts("repeat-load/reset/frame lifecycle passed");
    return 0;
}

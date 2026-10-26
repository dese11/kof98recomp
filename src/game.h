#ifndef KOF98_GAME_H
#define KOF98_GAME_H
#include <neogeorecomp/neogeo.h>
extern const ng_game_t k_kof98;
/* Set before load; NULL restores the standalone default (or no core assets). */
void kof98_set_asset_directory(const char *path);
#endif

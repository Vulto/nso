/* assetpack.h - embedded UI assets for NSO_RELEASE builds (fonts, system
 * cards, UI art, title lists). Scrapable game covers are never packed. */
#ifndef NSO_ASSETPACK_H
#define NSO_ASSETPACK_H

#ifdef NSO_RELEASE

#include <stdbool.h>

void AssetPack_Install(void);
bool AssetPack_Has(const char *path);
bool Nso_FileExists(const char *path);
#define FileExists(p) Nso_FileExists(p)

#endif /* NSO_RELEASE */

#endif /* NSO_ASSETPACK_H */

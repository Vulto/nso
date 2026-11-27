/* sneslist.h - Compatibility aliases for syslist.h (SNES was the first
 * system to use the wiki title list). New code should include syslist.h. */
#ifndef NSO_SNESLIST_H
#define NSO_SNESLIST_H

#include "syslist.h"

#define SNESLIST_MAX  SYSLIST_MAX
#define SNESLIST_NAME SYSLIST_NAME
typedef SysListRegion SnesListRegion;
#define SNESLIST_ALL SYSLIST_ALL
#define SNESLIST_USA SYSLIST_USA
#define SNESLIST_JP  SYSLIST_JP
typedef SysListGame SnesListGame;
#define SnesList_Parse   SysList_Parse
#define SnesList_InRegion SysList_InRegion
#define SnesList_Filter  SysList_Filter
#define SnesList_Boxart  SysList_Boxart
#define SnesList_Slug    SysList_Slug

#endif /* NSO_SNESLIST_H */

#ifdef SNESLIST_IMPLEMENTATION
#define SYSLIST_IMPLEMENTATION
#include "syslist.h"
#endif

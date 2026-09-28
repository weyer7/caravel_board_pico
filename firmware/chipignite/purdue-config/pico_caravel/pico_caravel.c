//pin mappings + commands
#include "pico_includes.h"

//define this to flash a team's pico firmware (optional)
//they should make this in pico_teamXX.c
// #define TEAM_FIRMWARE

int main() {
#ifdef TEAM_FIRMWARE
    teamXX_main();
#else
    hkflash_main();
#endif
    return 0;
}
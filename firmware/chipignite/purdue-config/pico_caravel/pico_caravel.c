//pin mappings + commands
#include "pico_includes.h"

//define this to flash a team's pico firmware (optional)
//they should make this in pico_teamXX.c
// #define TEAM_FIRMWARE

#ifdef TEAM_FIRMWARE
#include pico_teamXX.h
int main(void)
{
    return teamXX_main();
}
#else
int main(void)
{
    return hkflash_main();
}
#endif
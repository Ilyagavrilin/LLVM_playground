#pragma once
#define SIM_X_SIZE 800
#define SIM_Y_SIZE 600

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __sim__
void simFlush();
void simPutPixel(int x, int y, int argb);
int simRand();
#endif

extern void simInit();
extern void app();
extern void simExit();

#ifdef __cplusplus
}
#endif

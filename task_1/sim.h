#define SIM_X_SIZE 800
#define SIM_Y_SIZE 600

#ifndef __sim__
void simFlush();
void simPutPixel(int x, int y, int argb);
int simRand();
#endif

extern void simInit();
extern void app();
extern void simExit();

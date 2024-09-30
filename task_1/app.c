#include "sim.h"

#define RED    0xFFFF0000
#define BLACK  0x00000000
#define WHITE  0xFFFFFFFF

void draw_circle(int x0, int y0, int radius, int argb) {
    int x = radius;
    int y = 0;
    int decisionOver2 = 1 - x;
    while (y <= x) {
        simPutPixel( x + x0,  y + y0, argb);
        simPutPixel( y + x0,  x + y0, argb);
        simPutPixel(-x + x0,  y + y0, argb);
        simPutPixel(-y + x0,  x + y0, argb);
        simPutPixel(-x + x0, -y + y0, argb);
        simPutPixel(-y + x0, -x + y0, argb);
        simPutPixel( x + x0, -y + y0, argb);
        simPutPixel( y + x0, -x + y0, argb);
        y++;
        if (decisionOver2 <= 0) {
            decisionOver2 += 2 * y + 1;
        } else {
            x--;
            decisionOver2 += 2 * (y - x) + 1;
        }
    }
}

void draw_rectangle(int x, int y, int width, int height, int argb) {
  for (int i = x; i < x + width; i++)
    {
      for (int j = y; j < y + height; j++)
      {
        simPutPixel(i, j, argb);
      }
    }
}

// main function
void app()
{
  int ball_x = SIM_X_SIZE / 2;
  int ball_y = SIM_Y_SIZE / 2;
  int ball_dx = 2;
  int ball_dy = 2;
  int ball_radius = 10;

  int obstacle_x = simRand() % SIM_X_SIZE;
  int obstacle_y = 0;
  int obstacle_speed = 2;
  int obstacle_width = 150;
  int obstacle_height = 150;
  int previous_obstacle_x = obstacle_x;
  int previous_obstacle_y = obstacle_y;

  int running = 1;

  while (running)
  {

    
    ball_x += ball_dx;
    ball_y += ball_dy;

    if (ball_x - ball_radius <= 0 || ball_x + ball_radius >= SIM_X_SIZE)
    {
      ball_dx = -ball_dx;
      ball_x += ball_dx;
    }
    if (ball_y - ball_radius <= 0 || ball_y + ball_radius >= SIM_Y_SIZE)
    {
      ball_dy = -ball_dy;
      ball_y += ball_dy;
    }


    previous_obstacle_y = obstacle_y;
    obstacle_y += obstacle_speed;
    if (obstacle_y + obstacle_height >= SIM_Y_SIZE)
    {
      previous_obstacle_x = obstacle_x;
      previous_obstacle_y = obstacle_y;
      obstacle_y = 0;
      obstacle_x = simRand() % (SIM_X_SIZE - obstacle_width);
    }

    if (ball_x + ball_radius > obstacle_x && ball_x - ball_radius < obstacle_x + obstacle_width &&
        ball_y + ball_radius > obstacle_y && ball_y - ball_radius < obstacle_y + obstacle_height)
    {
      ball_dy = -ball_dy;
    }

    draw_circle(ball_x, ball_y, ball_radius, WHITE);

    draw_rectangle(previous_obstacle_x, previous_obstacle_y, obstacle_width, obstacle_height, BLACK);
    draw_rectangle(obstacle_x, obstacle_y, obstacle_width, obstacle_height, RED);

    simFlush();
  }
}

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
  // start from left-bottom angle
  for (int i = x; i < x + width; i++)
    {
      for (int j = y; j < y + height; j++)
      {
        simPutPixel(i, j, argb);
      }
    }
}

void app()
{
  int ball_x = SIM_X_SIZE / 2;
  int ball_y = SIM_Y_SIZE / 2;
  int ball_dx = 2;
  int ball_dy = 2;
  int ball_radius = 10;
  int previous_ball_x = ball_x;
  int previous_ball_y = ball_y;


  const int obstacle_width = 200;
  const int obstacle_height = 200;
  int obstacle_x = simRand() % (SIM_X_SIZE - obstacle_width);
  int obstacle_y = simRand() % (SIM_Y_SIZE - obstacle_height);
  const int obstacle_life = 40;
  int obstacle_timer = obstacle_life; // to draw rectangle on first iteration

  int running = 1;

  while (running)
  {
    previous_ball_x = ball_x;
    previous_ball_y = ball_y;
    
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


    if (obstacle_timer >= obstacle_life)
    {
      draw_rectangle(obstacle_x, obstacle_y, obstacle_width, obstacle_height, BLACK);
      obstacle_x = simRand() % (SIM_X_SIZE - obstacle_width);
      obstacle_y = simRand() % (SIM_Y_SIZE - obstacle_height);
      draw_rectangle(obstacle_x, obstacle_y, obstacle_width, obstacle_height, RED);
      obstacle_timer = 0;
    }
    obstacle_timer++;

    if (ball_x + ball_radius > obstacle_x && ball_x - ball_radius < obstacle_x + obstacle_width &&
        ball_y + ball_radius > obstacle_y && ball_y - ball_radius < obstacle_y + obstacle_height)
    {
      ball_dy = -ball_dy;
    }

    draw_circle(previous_ball_x, previous_ball_y, ball_radius, BLACK);
    draw_circle(ball_x, ball_y, ball_radius, WHITE);


    simFlush();
  }
}

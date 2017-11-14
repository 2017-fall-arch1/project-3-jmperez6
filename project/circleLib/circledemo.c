#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include "abCircle.h"

AbRect rect5 = {abRectGetBounds, abRectCheck, {100,15}};; /**< 10x10 rectangle */
AbRect rect30 = {abRectGetBounds, abRectCheck, {100,40}};; /**< 10x10 rectangle */
//AbRect rect40 = {abRectGetBounds, abRectCheck, {40,40}};; /**< 10x10 rectangle */


u_int bgColor = COLOR_CYAN;

Layer layer4 = {		/**< Layer with an orange circle */
  (AbShape *)&circle6,
  {(screenWidth/2)-4, (screenHeight/2)+18}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_MAGENTA,
  0,
};

Layer layer3 = {		/**< Layer with an orange circle */
  (AbShape *)&circle6,
  {(screenWidth/2)+20, (screenHeight/2)+18}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_MAGENTA,
  &layer4,
};

Layer layer2 = {		/**< Layer with an orange circle */
  (AbShape *)&circle14,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_PINK,
  &layer3,
};

Layer layer1 = {		/**< Layer with a red square */
  (AbShape *)&rect5,
  {screenWidth/2, screenHeight/2 + screenHeight/4}, /**< center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_GREEN,
  &layer2,
};

Layer layer0 = {		/**< Layer with a red square */
  (AbShape *)&rect30,
  {screenWidth/2, screenHeight}, /**< center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_CHOCOLATE,
  &layer1,
};


int
main()
{
  configureClocks();
  lcd_init();

  clearScreen(COLOR_GRAY);
  //drawString5x7(20,20, "hello", COLOR_BLACK, COLOR_MAGENTA);

  layerDraw(&layer0);

}

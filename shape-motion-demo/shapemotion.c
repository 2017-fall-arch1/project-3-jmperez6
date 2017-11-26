/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GREEN_LED BIT6

 
#define KirbyCenterWidth screenWidth/2
#define KirbyCenterHeight screenHeight/2


AbRect rect10 = {abRectGetBounds, abRectCheck, {10,10}}; /**< 10x10 rectangle */
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2-10, screenHeight/2-10}
};

AbRect rectGrass = {abRectGetBounds, abRectCheck, {200, 10}};; /**< 10x10 rectangle */
AbRect rectGround = {abRectGetBounds, abRectCheck, {200, 40}};; /**< 10x10 rectangle */

u_int bgColor = COLOR_GRAY;

Layer apple2;
Layer mapple2;

Layer apple = {
  (AbShape *) &circle5,
  {screenWidth+50,10},/**< top right corner */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  0,
};


Layer layer6 = {		/**< GROUND LAYER */
  (AbShape *)&rectGround,
  {(screenWidth), (screenHeight)}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_CHOCOLATE,
  &apple,
};

Layer layer5 = {		/**< GRASS LAYER */
  (AbShape *)&rectGrass,
  {(screenWidth), (screenHeight)-50}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_GREEN,
  &layer6,
};



Layer layer4 = {		/**< KIRBY'S LEFT FOOT LAYER */
  (AbShape *)&circle6,
  {KirbyCenterWidth-50, KirbyCenterHeight+10}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_MAGENTA,
  &layer5,
};




Layer layer3 = {		/**< KIRBY'S BODY LAYER */
  (AbShape *)&circle14,
  {KirbyCenterWidth-40, KirbyCenterHeight}, /**< center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_PINK,
  &layer4,
};

Layer layer2 = {		/**< KIRBY'S SCLERA */
  (AbShape *)&circle4,
  {KirbyCenterWidth-32, KirbyCenterHeight-8}, /**< center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_BLACK,
  &layer3,
};

Layer layer1 = {		/**< KIRBY'S PUPIL */
  (AbShape *)&circle2,
  {KirbyCenterWidth-32, KirbyCenterHeight-10}, /**< center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_WHITE,
  &layer2,
};

Layer layer0 = {		/**< KIRBY'S RIGHT FOOT LAYER */
  (AbShape *)&circle6,
  {(KirbyCenterWidth)-30, (KirbyCenterHeight)+10}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* next & last pos */
  COLOR_MAGENTA,
  &layer1,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GRAY,
  &layer0
};









/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */

MovLayer ml4 = { &layer4, {1,0}, 0 }; //feet moving side to side
MovLayer ml0 = { &layer0, {1,0}, &ml4 };

MovLayer ml1 = { &layer1, {0,-1}, 0}; /**< Body and eyes only move up and down*/
MovLayer ml2 = { &layer2, {0,-1}, &ml1 };
MovLayer ml3 = { &layer3, {0,-1}, &ml2 }; 

MovLayer mapple = { &apple, {-1,0}, 0 };








void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



Region kirbyfence = {{KirbyCenterWidth-56, 1}, {KirbyCenterWidth-24, 94}};
Region fence = {{1,0}, {screenWidth, 0}}; /**< Create a fence region */



/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


void HorizontalAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
      if ((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) ||
	  (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0]) ) {
	int velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
	newPos.axes[0] += (2*velocity);
      }	/**< if outside of fence */
    ml->layer->posNext = newPos;
  } /**< for ml */
}



void VerticalAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
      if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||
	  (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) {
	int velocity = ml->velocity.axes[1] = -ml->velocity.axes[1];
	newPos.axes[1] += (2*velocity);
      }	/**< if outside of fence */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void BodyJump(MovLayer *ml, Region *fence){
  Vec2 newPos;
  Region shapeBoundary;
  int velocity =  ml->velocity.axes[1];
  //Check first if body will collide, if so, flip velocity
  vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
  abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
  if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||
      (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) {
        return;
  }/**< if outside of fence */
  //Then just move up or down the body together.
  for (; ml; ml = ml->next){
    ml->velocity.axes[1] = velocity;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    newPos.axes[1] += (2*velocity);
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void Gravity(MovLayer *ml, Region *fence){
  Vec2 newPos;
  Region shapeBoundary;
  int velocity =  ml->velocity.axes[1];
  
  vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
  abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
  if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||
      (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) {
    return;
  }/**< if outside of fence */
  //Then just move up or down the body together.
  for (; ml; ml = ml->next){
    ml->velocity.axes[1] = velocity;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    newPos.axes[1] += (2*velocity);
    ml->layer->posNext = newPos;
  } /**< for ml */
}


/*
char* toArray(int number){
  int n = log10(number) + 1;
  int i;
  char *numberArray = calloc(n, sizeof(char));
  for ( i = 0; i < n; ++i, number /= 10){
    numberArray[i] = number % 10;
  }
  return numberArray;
}*/





int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
static char str[5];
static int pts = 0;
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  
  layerDraw(&layer0);


  //layer1.abShape -> getBounds = layer3.abShape -> getBounds;
  //layer2.abShape -> getBounds = layer3.abShape -> getBounds;
    

  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  char points[8] = {'P', 'o', 'i', 'n', 't', 's', ':'};
  points[7] = 0;
  drawString5x7(screenWidth/2-50, screenHeight-20, points, COLOR_BLACK, COLOR_WHITE);
  for (int i = 0; i < 4; i++)
        str[i] = ' ';
  str[3] = 0 + '0';
  str[4] = 0;
  drawString5x7(screenWidth/2, screenHeight-20, str, COLOR_BLACK, COLOR_WHITE);

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    //u_int switches = p2sw_read(), i;
    int c = 3;
    int temp = pts;
    while(temp > 0){
      if(c < 0){
	break;
      }
      int digit = temp % 10;
      str[c] = digit + '0';
      temp /= 10;
      c -= 1;
    }
    drawString5x7(screenWidth/2, screenHeight-20, str, COLOR_BLACK, COLOR_WHITE);    
    movLayerDraw(&ml0, &layer0);
    movLayerDraw(&ml3, &layer1);
    movLayerDraw(&mapple, &apple);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler(){
  static short count = 0;
  static short obsCount = 0;
  Region bodyBounds;
  Region appleBounds;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  obsCount ++;
  if (count == 5) {
    layerGetBounds(ml3.layer, &bodyBounds);
    layerGetBounds(mapple.layer, &appleBounds);
    int bool1 = (appleBounds.topLeft.axes[1] <= bodyBounds.topLeft.axes[1] & appleBounds.botRight.axes[1] >= bodyBounds.botRight.axes[1] & appleBounds.topLeft.axes[0] <= bodyBounds.botRight.axes[0]);
    int bool2 = (appleBounds.botRight.axes[1] <= bodyBounds.topLeft.axes[1] & appleBounds.topLeft.axes[0] >= bodyBounds.topLeft.axes[0] & appleBounds.botRight.axes[0] <= bodyBounds.botRight.axes[0]);
    int bool3 = (appleBounds.topLeft.axes[1] >= bodyBounds.botRight.axes[1] & appleBounds.topLeft.axes[0] >= bodyBounds.topLeft.axes[0] & appleBounds.botRight.axes[0] <= bodyBounds.botRight.axes[0]);
    u_int buttons = p2sw_read(), i;
    char btn[5];
    for (i = 0; i < 4; i++)
      btn[i] = (buttons & (1<<i)) ? ' ' : '1'+i;
    btn[4] = 0;
    if (btn[1] == '2'){
      ml3.velocity.axes[1] = -1;
      if(bool1 || bool2 || bool3){
        mapple.layer -> pos.axes[0] = screenWidth+50;
        pts += 1;
        //*str = toArray(pts);
        int c = 3;
        int temp = pts;
        while(temp > 0){
	  if(c < 0){break;}
	  int digit = temp % 10;
	  str[c] = digit + '0';
	  temp /= 10;
	  c -= 1;
        }
      }
      BodyJump(&ml3, &kirbyfence);
    }
    if (btn[3] == '4'){
      pts += 1;
      //*str = toArray(pts);
      int c = 3;
      int temp = pts;
      while(temp > 0){
	if(c < 0){break;}
	int digit = temp % 10;
	str[c] = digit + '0';
	temp /= 10;
	c -= 1;
      }
    }
    //if button is not pressed, call Gravity.
    else if (btn[1] != '2'){
      ml3.velocity.axes[1] = 1;
      Gravity(&ml3, &kirbyfence);
    }
    
    if(bool1 || bool2 || bool3){
      mapple.layer -> pos.axes[0] = screenWidth+50;
      pts += 1;
      //*str = toArray(pts);
      int c = 3;
      int temp = pts;
      while(temp > 0){
	if(c < 0){break;}
	int digit = temp % 10;
	str[c] = digit + '0';
	temp /= 10;
	c -= 1;
      }
    }
    
    HorizontalAdvance(&ml0, &kirbyfence);
    HorizontalAdvance(&mapple, &fence);
    redrawScreen = 1;
    count = 0;
  }
    
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}

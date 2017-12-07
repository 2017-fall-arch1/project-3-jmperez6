/** \file shapemotion.c
 *  \brief This is a simple catch game where the main
 *  character is Kirby and you're trying to catch the
 *  apples that appear from the right side.
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

/*#define AppleBody circle5
#define AppleLeg circle2
#define AppleBound circle5.getBounds
#define AppleCheck circle5.check
*/
 
#define KirbyCenterWidth screenWidth/2
#define KirbyCenterHeight screenHeight/2

//AbApple apple5 = {AppleBound, AppleCheck, AppleBody, AppleLeg, AppleLeg};
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

/*
Layer brickwall = {
  (AbShape *) &rightArrow,
  {screenWidth+50, 10},
  {0,0}, {0,0},
  COLOR_FIREBRICK,
  0,
};*/

/*
Layer applerightstump = {
  (AbShape *) &circle2,
  {screenWidth+52,15},
  {0,0}, {0,0},	
  COLOR_RED,
  0,
};

Layer appleleftstump = {
  (AbShape *) &circle2,
  {screenWidth+48,15},
  {0,0}, {0,0},	
  COLOR_RED,
  &applerightstump,
};
*/
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

//MovLayer mwall = { &brickwall, {-1,0}, 0 };
MovLayer mapple = { &apple, {-3,0}, 0 };
//MovLayer mapplels = { &appleleftstump, {-3,0}, &mapple };
//MovLayer mapplers = { &applerightstump, {-3,0}, &mapplels };








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
Region fence = {{-10,-10}, {screenWidth+30, screenHeight+30}}; /**< Create a fence region */



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

int BodyJump(MovLayer *ml, Region *fence){
  Vec2 newPos;
  Region shapeBoundary;
  int velocity =  ml->velocity.axes[1];
  //Check first if body will collide, if so, flip velocity
  vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
  abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
  if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||
      (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) {
        return 0;
  }/**< if outside of fence */
  //Then just move up or down the body together.
  for (; ml; ml = ml->next){
    ml->velocity.axes[1] = velocity;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    newPos.axes[1] += (2*velocity);
    ml->layer->posNext = newPos;
  } /**< for ml */
  return velocity;
}

void FeetJump(MovLayer *ml, int Bodyvelocity){
  Vec2 newPos;
  //Then just move up or down the body together.
  for (; ml; ml = ml->next){
    ml->velocity.axes[1] = Bodyvelocity;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    newPos.axes[1] += (Bodyvelocity);
    ml->layer->posNext = newPos;
  } /**< for ml */
}

int Gravity(MovLayer *ml, Region *fence){
  Vec2 newPos;
  Region shapeBoundary;
  int velocity =  ml->velocity.axes[1];
  
  vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
  abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
  if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||
      (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) {
    return 0;
  }/**< if outside of fence */
  //Then just move up or down the body together.
  for (; ml; ml = ml->next){
    ml->velocity.axes[1] = velocity;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    newPos.axes[1] += (2*velocity);
    ml->layer->posNext = newPos;
  } /**< for ml */
  return velocity;
}


void buzzer_init(){
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */

   
}


void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}



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
  buzzer_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  
  layerDraw(&layer0);
    

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
    //movLayerDraw(&mwall, &wall);
  }
}
static int obsCount = 0;
//static int levelchange = 0;
static int level = 8;
static int max = (screenHeight/2)+10;
/** Watchdog timer interrupt handler. 8 interrupts/sec */
void wdt_c_handler(){
  //InterruptGame();
  static short count = 0;
  
  
  int applehit = 0;
  int wallhit = 0;
  Region bodyBounds;
  
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  obsCount ++;
  CanInterrupt(count);
  if (count == level) {
    layerGetBounds(ml3.layer, &bodyBounds);
    int bool1 = (mapple.layer -> pos.axes[1] >= bodyBounds.topLeft.axes[1] & mapple.layer -> pos.axes[1] <= bodyBounds.botRight.axes[1] & mapple.layer -> pos.axes[0] <= bodyBounds.botRight.axes[0]);
    u_int buttons = p2sw_read(), i;
    char btn[5];
    for (i = 0; i < 4; i++)
      btn[i] = (buttons & (1<<i)) ? ' ' : '1'+i;
    btn[4] = 0;
    if (btn[1] == '2'){ 
      ml3.velocity.axes[1] = -1;
      if(bool1 & !applehit){
	buzzer_set_period(80);
	
        int currentpos = mapple.layer -> pos.axes[1];
        int randpos = (currentpos + (obsCount)) % max;
	mapple.layer -> posNext.axes[1] = randpos;
        mapple.layer -> posNext.axes[0] = screenWidth+50;
        pts += 1;
	applehit = 1;
        int c = 3;
        int temp = pts;
        while(temp > 0){
	  if(c < 0){break;}
	  int digit = temp % 10;
	  str[c] = digit + '0';
	  temp /= 10;
	  c -= 1;
        }
	buzzer_set_period(0);
      }
      int v;
      v = BodyJump(&ml3, &kirbyfence);
      FeetJump(&ml0, v);
    }
    
    //if button is not pressed, call Gravity.
    else if (btn[1] != '2'){
      ml3.velocity.axes[1] = 1;
      int v;
      v = Gravity(&ml3, &kirbyfence);
      FeetJump(&ml0, v);
    }
    
    if(bool1 & !applehit){
      //int max = (screenHeight/2);
      int currentpos = mapple.layer -> pos.axes[1];
      int randpos = (currentpos + (obsCount)) % max;
      buzzer_set_period(80);
      mapple.layer -> posNext.axes[0] = screenWidth+50;
      mapple.layer -> posNext.axes[1] = randpos;
      
      pts += 1;
      applehit = 1;
      int c = 3;
      int temp = pts;
      while(temp > 0){
	if(c < 0){break;}
	int digit = temp % 10;
	str[c] = digit + '0';
	temp /= 10;
	c -= 1;
      }
      buzzer_set_period(0);
    }


    if(mapple.layer -> pos.axes[0] <= 2  & !applehit){
      //int max = (screenHeight/2);
      int currentpos = mapple.layer -> pos.axes[1];
      int randpos = (currentpos + (obsCount)) % max;
      buzzer_set_period(40);
      mapple.layer -> posNext.axes[0] = screenWidth+50;
      mapple.layer -> posNext.axes[1] = randpos;
      pts -= 1;
      int c = 3;
      int temp = pts;
      while(temp > 0){
	if(c < 0){break;}
	int digit = temp % 10;
	str[c] = digit + '0';
	temp /= 10;
	c -= 1;
      }
      buzzer_set_period(0);
    }
 
    /*if(mwall.layer -> pos.axes[0] <= 5  & !wallhit){
      mwall.layer -> posNext.axes[0] = screenWidth+50;
      //int max = (screenHeight-50);
      //int randpos = ( (max + 1) - 0) + 0;
      //mwall.layer -> posNext.axes[1] = randpos;
      }*/


    if(pts < 0){
      char gameover[12] = {'G', 'A', 'M', 'E', ' ', 'O', 'V', 'E', 'R', ' '};
      gameover[11] = 0;
      drawString5x7(screenWidth/2-15, screenHeight/2-15, gameover, COLOR_RED, COLOR_BLACK);
      return;
    }

    
    HorizontalAdvance(&ml0, &kirbyfence);
    HorizontalAdvance(&mapple, &fence);
    //HorizontalAdvance(&mwall, &fence);
    redrawScreen = 1;
    count = 0;
    
  }
    
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}

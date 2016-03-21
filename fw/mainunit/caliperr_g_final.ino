#include <EEPROM.h>
#include "U8glib.h"
#include <I2C.h>

U8GLIB_ST7920_128X64_1X u8g(10);

#define KEY_NONE 0
#define KEY_PREV 1
#define KEY_NEXT 2
#define KEY_SELECT 3
#define KEY_BACK 4

uint8_t uiKeyPrev = A0;
uint8_t uiKeyNext = A2;
uint8_t uiKeySelect = A1;

uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;

void uiStep(void) {
  uiKeyCodeSecond = uiKeyCodeFirst;
  if ( analogRead(uiKeyPrev) > 1000 )
    uiKeyCodeFirst = KEY_PREV;
  else if ( analogRead(uiKeyNext) > 1000 )
    uiKeyCodeFirst = KEY_NEXT;
  else if ( analogRead(uiKeySelect) > 1000 )
    uiKeyCodeFirst = KEY_SELECT;
  else 
    uiKeyCodeFirst = KEY_NONE;
  
  if ( uiKeyCodeSecond == uiKeyCodeFirst )
    uiKeyCode = uiKeyCodeFirst;
  else
    uiKeyCode = KEY_NONE;
}

#define AXES 1
uint16_t calipers[AXES] = {0x16};
float dro[AXES];
float last_dro[AXES] = {0};
uint8_t current_caliper = 0, main_redraw_required = 1;
float axis_multiplicator[AXES] = {0.01};
float coef_multiplicator[AXES] = {1};
char *info_line1[AXES] = {"mm"};
char *info_line2[AXES] = {"-"};
uint8_t digits[AXES] = {2};
int8_t precision[AXES] = {0};
const char *labels[AXES] = {"X"};

#define MENU_ITEMS 4
#define MENU_PARAMS 2

const char *menu_strings[MENU_ITEMS] = { "Os", "Koeficient", "Ulozit", "Zrusit" };
const char *menu_values[MENU_PARAMS][3] = {{"X","Y","Z"},{"1","1/2","2"}};
uint8_t menu_current = 0;
uint8_t submenu_current[MENU_ITEMS] = {0,0};
uint8_t submenu_sizes[MENU_ITEMS] = {3,3};
uint8_t menu_redraw_required = 0, in_menu = 0;
uint8_t last_key_code = KEY_NONE;


void drawMenu(void) {
  uint8_t i, h;
  u8g_uint_t w;

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  h = u8g.getFontAscent()-u8g.getFontDescent();
  w = u8g.getWidth();
  for( i = 0; i < MENU_ITEMS; i++ ) {
    u8g.setDefaultForegroundColor();
    if ( i == menu_current ) {
      u8g.drawBox(0, i*h, w, h);
      u8g.setDefaultBackgroundColor();
    }
    u8g.drawStr(1, i*h, menu_strings[i]);
    if(i<MENU_PARAMS){
      u8g.drawStr(70, i*h, menu_values[i][ submenu_current[i]]);
    }
  }
}


void drawMain(void) {
  uint8_t i, h;
  u8g_uint_t w;

  u8g.setFont(u8g_font_profont29);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  h = 22;
  w = u8g.getWidth();
  for( i = 0; i < AXES; i++ ) {
    u8g.setDefaultForegroundColor();
    u8g.setPrintPos(0, i*h);
    u8g.print(dro[i],digits[i]);
    if(i < 2)u8g.drawHLine(0, i*h + 21, 128);
  }

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();

  for( i = 0; i < AXES; i++ ) {
    u8g.setDefaultForegroundColor();
    u8g.setPrintPos(114, i*h);
    u8g.print(info_line1[i]);
    u8g.setPrintPos(114, i*h + 11);
    u8g.print(info_line2[i]);
  }
  
}

#define PARAMS 1

void save(void){
  EEPROM.update(submenu_current[0] * PARAMS, submenu_current[1]);
  //EEPROM.update(submenu_current[0] * PARAMS + 1, submenu_current[2]);
  //EEPROM.update(submenu_current[0] * PARAMS + 2, submenu_current[3]);
}

void load(void){
  submenu_current[1] = EEPROM.read(submenu_current[0] * PARAMS);
  //submenu_current[2] = EEPROM.read(submenu_current[0] * PARAMS + 1);
  //submenu_current[3] = EEPROM.read(submenu_current[0] * PARAMS + 2);
}

void setupAxes(void){
  uint8_t i,units,mult;
  for(i=0; i<=AXES; i++){
    //units = EEPROM.read(i * PARAMS);
    mult = EEPROM.read(i * PARAMS);
    /*
    switch (units){
      case 0:
        axis_multiplicator[i] = 0.01;
        info_line1[i] = "mm";
        digits[i] = 2;
        break;
      case 1:
        axis_multiplicator[i] = 0.001;
        info_line1[i] = "in";
        digits[i] = 3;
        break;      
    }
    */
    switch(mult){
      case 0:
        coef_multiplicator[i] = 1;
        precision[i] = 0;
        info_line2[i] = "-";
        break;
      case 1:
        coef_multiplicator[i] = 0.5;
        info_line2[i] = "/2";
        precision[i] = 1;
        break;
      case 2:
        coef_multiplicator[i] = 2;
        precision[i] = 0;
        info_line2[i] = "x2";
        break;
    }
  } 
}

#define LASTPARAM 1
#define SAVE 2
#define CANCEL 3

void controller(void) {
  if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
    return;
  }
  last_key_code = uiKeyCode;

  if(in_menu == 1){
    switch ( uiKeyCode ) {
      case KEY_NEXT:
        menu_current++;
        if ( menu_current >= MENU_ITEMS )
          menu_current = 0;
        menu_redraw_required = 1;
        break;
      case KEY_PREV:
        if ( menu_current == 0 )
          menu_current = MENU_ITEMS;
        menu_current--;
        menu_redraw_required = 1;
        break;
      case KEY_SELECT:
        if(menu_current <= LASTPARAM){
          submenu_current[menu_current]++;
          if( submenu_current[menu_current] >= submenu_sizes[menu_current])
            submenu_current[menu_current] = 0;
            menu_redraw_required = 1;      
        }
        switch(menu_current){
          case 0:
            load();
            break;
          case SAVE:
            save();
            setupAxes();
            in_menu = 0;
            main_redraw_required = 1;
            break;
          case CANCEL:
            load();
            in_menu = 0;
            main_redraw_required = 1;
            break;
        }
        break;
    }
  } else {
    if ( uiKeyCode != KEY_NONE ){
      in_menu = 1;
      switch ( uiKeyCode ) {
        case KEY_PREV:
          submenu_current[0] = 0;
          break;
        case KEY_SELECT:
          submenu_current[0] = 1;
          break;
        case KEY_NEXT:
          submenu_current[0] = 2;
          break;
      }
      load();
      menu_redraw_required = 1;
    }
  }
}

#if defined(FULLDECODE)
#else
int bcd2dec(uint8_t bcd)
{
  int dec=0;
  int mult;
  uint8_t i=0;
   for (mult=1; bcd && i<2; bcd=bcd>>4,mult*=10,i++)
   {
    uint8_t d=0; 
    d = (bcd & 0x0f); 
    if(d>=0 && d<10)dec += d * mult;
   }
  return dec;
}
#endif


float readCaliper(void){
  I2c.read(calipers[current_caliper],1,4); //read 6 bytes (x,y,z) from the device
  char data[4];
  long int g=0;
  float f=0;
  uint8_t i=0;
  while (I2c.available()) {
    char inChar = (char)I2c.receive();
    data[i] = inChar;
    i++;
  }

 #if defined(FULLDECODE)
 memcpy(&g, data, sizeof g); 
 f = g;

  #else
  int mx=1;
  for(i=0;i<3;i++,mx*=100){
    g += bcd2dec(data[i]) * mx;
  }
   f = g;
  if((data[3]) & (1<<(0)))f*=-1;
  if((data[3]) & (1<<(2))){
    axis_multiplicator[current_caliper] = 0.01;
    info_line1[current_caliper] = "mm";
    digits[current_caliper] = 2 + precision[current_caliper];  
  }else{
    axis_multiplicator[current_caliper] = 0.001;
    info_line1[current_caliper] = "in";
    
    if((data[3]) & (1<<(1))){
      f+=0.5;
      digits[current_caliper] = 4;
    } else {
      digits[current_caliper] = 3 + precision[current_caliper];
    }
  }
  #endif

 f = f*axis_multiplicator[current_caliper]*coef_multiplicator[current_caliper];
 
 Serial.print(labels[current_caliper]);
 Serial.print(f);
 Serial.print(";\n");
 return f;
}

void setup() {
  Serial.begin(9600);
  I2c.begin();
  I2c.pullup(0);
  I2c.timeOut(1000);
  menu_redraw_required = 1;     // force initial redraw
  main_redraw_required = 1;
  setupAxes();
}

void loop() {  

  uiStep();                                     // check for key press

  if(in_menu == 0){
    if(current_caliper >= AXES)current_caliper = 0;
    dro[current_caliper] = readCaliper();
    if(last_dro[current_caliper] != dro[current_caliper])main_redraw_required = 1;
    last_dro[current_caliper] = dro[current_caliper];
    current_caliper++;    
  }

  if (  in_menu == 1 && menu_redraw_required != 0 ) {
    u8g.firstPage();
    do  {
      drawMenu();
    } while( u8g.nextPage() );
    menu_redraw_required = 0;
  }

  if (  in_menu == 0 && main_redraw_required != 0 ) {
    u8g.firstPage();
    do  {
      drawMain();
    } while( u8g.nextPage() );
    main_redraw_required = 0;
  }

  controller();                            // update menu bar
  
}

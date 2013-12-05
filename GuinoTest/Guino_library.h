/*
  GUINO DASHBOARD TEMPLATE FOR THE ARDUINO. 
 Done by Mads Hobye as a part of Instructables (AIR Program) & Medea (PhD Student).
 Licens: Creative Commons — Attribution-ShareAlike
 
 It should be used with the GUINO Dashboard app.
 
 More info can be found here: www.hobye.dk
 
 # This is the Guino Protocol Library should only be edited if you know what you are doing.
 */

#include </Users/davidcranor/Documents/libmaple/libraries/EasyTransfer/EasyTransfer.h>

#define guino_executed -1
#define guino_init 0
#define guino_addSlider 1
#define guino_addButton 2
#define guino_iamhere 3
#define guino_addToggle 4
#define guino_addRotarySlider 5
#define guino_saveToBoard 6
#define guino_setFixedGraphBuffer 8
#define guino_clearLabel 7
#define guino_addWaveform 9
#define guino_addColumn 10
#define guino_addSpacer 11
#define guino_addMovingGraph 13
#define guino_buttonPressed 14
#define guino_addChar 15
#define guino_setMin 16
#define guino_setMax 17
#define guino_setValue 20
#define guino_addLabel 12
#define guino_large 0
#define guino_medium 1
#define guino_small 2
#define guino_setColor  21

void EEPROMWriteInt(int16_t p_address, int16_t p_value);
uint16_t EEPROMReadInt(int16_t p_address);
void guino_update();
void gInitEEprom();
void gSetColor(int16_t _red, int16_t _green, int16_t _blue);
void gGetSavedValue(int16_t item_number, int16_t *_variable);
void gGetSavedValue(int16_t item_number, int16_t *_variable);
void gBegin(int16_t _eepromKey);
int16_t gAddButton(char * _name);
void gAddColumn();
int16_t gAddLabel(const char * _name, int16_t _size);
int16_t gAddSpacer(int16_t _size);
int16_t gAddToggle(char * _name, int16_t * _variable);
int16_t gAddFixedGraph(char * _name,int16_t _min,int16_t _max,int16_t _bufferSize, int16_t * _variable, int16_t _size);
int16_t gAddMovingGraph(const char * _name,int16_t _min,int16_t _max, int16_t * _variable, int16_t _size);
void gUpdateLabel(int16_t _item, char * _text);
int16_t gAddRotarySlider(int16_t _min,int16_t _max, char * _name, int16_t * _variable);
int16_t gAddSlider(int16_t _min,int16_t _max, const char * _name, int16_t * _variable);
void gUpdateValue(int16_t _item);
void gUpdateValue(int16_t * _variable);
void gSendCommand(byte _cmd, byte _item, int16_t _value);
void gInit();


void gButtonPressed(int16_t id);
void gItemUpdated(int16_t id);

boolean guidino_initialized = false;

//create object
EasyTransfer ET; 

struct SEND_DATA_STRUCTURE
{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  uint8_t cmd;
  uint8_t item;
  int16_t value;
};

// Find a way to dynamically allocate memory
int16_t guino_maxGUIItems = 100;
int16_t guino_item_counter = 0;
int16_t *guino_item_values[100]; 
int16_t gTmpInt = 0; // temporary int for items without a variable
boolean internalInit = true; // boolean to initialize before connecting to serial

// COMMAND STRUCTURE

//give a name to the group of data
SEND_DATA_STRUCTURE guino_data;
int16_t eepromKey = 1234;


//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int16_t p_address, int16_t p_value)
{
  //byte lowByte = ((p_value >> 0) & 0xFF);
  //byte highByte = ((p_value >> 8) & 0xFF);

  //EEPROM.write(p_address, lowByte);
  //EEPROM.write(p_address + 1, highByte);
}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
uint16_t EEPROMReadInt(int16_t p_address)
{
  //byte lowByte = EEPROM.read(p_address);
  //byte highByte = EEPROM.read(p_address + 1);

  //return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
	return 1; //debug
}


void guino_update()
{

  while(Serial1.available())
  {

    if(ET.receiveData())
    {
      switch (guino_data.cmd) 
      {
      case guino_init:

        guino_item_counter = 0;
        guidino_initialized = true;
        gInit();
        break;
      case guino_setValue:
        *guino_item_values[guino_data.item] = guino_data.value;
        guino_data.cmd = guino_executed;
        gItemUpdated(guino_data.item);
        break;
      case guino_buttonPressed:
        gButtonPressed(guino_data.item);
        break;
      case guino_saveToBoard:
        {

          gInitEEprom();
          for (int16_t i =0; i < guino_item_counter;i++)
          {
            EEPROMWriteInt(i*2+2, *guino_item_values[i]);
          }
        }
        break;
      }
    }
  }
}

void gInitEEprom()
{
  if(EEPROMReadInt(0) != eepromKey)
  {
    EEPROMWriteInt(0, eepromKey);
    for (int16_t i =1; i <guino_maxGUIItems;i++)
    {
      EEPROMWriteInt(i*2+2, -3276);
    }
  }

}

void gSetColor(int16_t _red, int16_t _green, int16_t _blue)
{
  gSendCommand(guino_setColor, 0, _red);
  gSendCommand(guino_setColor, 1, _green);
  gSendCommand(guino_setColor, 2, _blue);
}

void gGetSavedValue(int16_t item_number, int16_t *_variable)
{

  if(EEPROMReadInt(0) == eepromKey && internalInit)
  {

    int16_t tmpVar =  EEPROMReadInt((item_number)*2+2);
    if(tmpVar != -3276)
      *_variable = tmpVar;
  }

}

void gBegin(int16_t _eepromKey)
{

  // Sets all pointers to a temporary value just to make sure no random memory pointers.
  for(int16_t i = 0; i < guino_maxGUIItems; i++)
  {
    guino_item_values[i] = &gTmpInt;
  }
  eepromKey = _eepromKey;

  gInit(); // this one needs to run twice only way to work without serial connection.
  internalInit = false;

  //***********TESTING USB SERIAL
  Serial1.begin(115200);
  

  //ET.begin(details(guino_data), &Serial1);  //Old version
  //ET.begin( (uint8_t*)&guino_data, 255, &Serial1);  //This one works, but buffer is 255 big.
  ET.begin( (uint8_t*)&guino_data, sizeof(guino_data), &Serial1);




  gSendCommand(guino_executed, 0, 0);
  gSendCommand(guino_executed, 0, 0);
  gSendCommand(guino_executed, 0, 0);
  gSendCommand(guino_iamhere, 0, 0); 

}
int16_t gAddButton(char * _name)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    gSendCommand(guino_addButton,(byte)guino_item_counter,0);
    for (uint16_t i = 0; i < strlen(_name); i++){
      gSendCommand(guino_addChar,(byte)guino_item_counter,(int16_t)_name[i]);
    }
    guino_item_counter++;
    return guino_item_counter-1;
  }
  return -1;
}


void gAddColumn()
{

  gSendCommand(guino_addColumn,0,0);

}




int16_t gAddLabel(const char * _name, int16_t _size)
{
  if(guino_maxGUIItems > guino_item_counter)
  { 
    gSendCommand(guino_addLabel,(byte)guino_item_counter,_size);

    for (uint16_t i = 0; i < strlen(_name); i++){
      gSendCommand(guino_addChar,(byte)guino_item_counter,(int16_t)_name[i]);
    }

    guino_item_counter++;

    return guino_item_counter-1;
  }
  return -1;


} 
int16_t gAddSpacer(int16_t _size)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    gSendCommand(guino_addSpacer,(byte)guino_item_counter,_size);

    guino_item_counter++;
    return guino_item_counter-1;
  }
  return -1;

}   



int16_t gAddToggle(char * _name, int16_t * _variable)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    guino_item_values[guino_item_counter] =_variable ;
    gGetSavedValue(guino_item_counter, _variable);
    gSendCommand(guino_addToggle,(byte)guino_item_counter,*_variable);

    for (uint16_t i = 0; i < strlen(_name); i++){
      gSendCommand(guino_addChar,(byte)guino_item_counter,(int16_t)_name[i]);
    }

    guino_item_counter++;

    return guino_item_counter-1;


  }
  return -1;
}   

int16_t gAddFixedGraph(char * _name,int16_t _min,int16_t _max,int16_t _bufferSize, int16_t * _variable, int16_t _size)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    gAddLabel(_name,guino_small);
    guino_item_values[guino_item_counter] =_variable ;
    gGetSavedValue(guino_item_counter, _variable);
    gSendCommand(guino_addWaveform,(byte)guino_item_counter,_size);
    gSendCommand(guino_setMax,(byte)guino_item_counter,_max);
    gSendCommand(guino_setMin,(byte)guino_item_counter,_min);
    gSendCommand(guino_setFixedGraphBuffer,(byte)guino_item_counter,_bufferSize);


    guino_item_counter++;

    return guino_item_counter-1;
  }
  return -1;
}

int16_t gAddMovingGraph(const char * _name,int16_t _min,int16_t _max, int16_t * _variable, int16_t _size)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    gAddLabel(_name,guino_small);
    guino_item_values[guino_item_counter] =_variable ;
    gGetSavedValue(guino_item_counter, _variable);
    gSendCommand(guino_addMovingGraph,(byte)guino_item_counter,_size);
    gSendCommand(guino_setMax,(byte)guino_item_counter,_max);
    gSendCommand(guino_setMin,(byte)guino_item_counter,_min);


    guino_item_counter++;

    return guino_item_counter-1;
  }
  return -1;


}   


void gUpdateLabel(int16_t _item, char * _text)
{

  gSendCommand(guino_clearLabel,(byte)_item,0);
  for (uint16_t i = 0; i < strlen(_text); i++){
    gSendCommand(guino_addChar,(byte)_item,(int16_t)_text[i]);
  }
}



int16_t gAddRotarySlider(int16_t _min,int16_t _max, char * _name, int16_t * _variable)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    guino_item_values[guino_item_counter] =_variable ;
    gGetSavedValue(guino_item_counter, _variable);
    gSendCommand(guino_addRotarySlider,(byte)guino_item_counter,*_variable);
    gSendCommand(guino_setMax,(byte)guino_item_counter,_max);
    gSendCommand(guino_setMin,(byte)guino_item_counter,_min);
    for (uint16_t i = 0; i < strlen(_name); i++){
      gSendCommand(guino_addChar,(byte)guino_item_counter,(int16_t)_name[i]);
    }

    guino_item_counter++;
    gUpdateValue(_variable);
    return guino_item_counter-1;
  }
  return -1;

}

int16_t gAddSlider(int16_t _min,int16_t _max, const char * _name, int16_t * _variable)
{
  if(guino_maxGUIItems > guino_item_counter)
  {
    guino_item_values[guino_item_counter] =_variable ;
    gGetSavedValue(guino_item_counter, _variable);
    gSendCommand(guino_addSlider,(byte)guino_item_counter,*_variable);
    gSendCommand(guino_setMax,(byte)guino_item_counter,_max);
    gSendCommand(guino_setMin,(byte)guino_item_counter,_min);
    for (uint16_t i = 0; i < strlen(_name); i++){
      gSendCommand(guino_addChar,(byte)guino_item_counter,(int16_t)_name[i]);
    }

    guino_item_counter++;
    gUpdateValue(_variable);
    return guino_item_counter-1;
  }
  return -1;

}

void gUpdateValue(int16_t _item)
{

  gSendCommand(guino_setValue,_item, *guino_item_values[_item]); 
}


void gUpdateValue(int16_t * _variable)
{

  int16_t current_id = -1;
  for(int16_t i = 0; i < guino_item_counter; i++)
  {

    if(guino_item_values[i] == _variable)
    {

      current_id = i;
      gUpdateValue(current_id);
    }
  }
  // if(current_id != -1)

}



void gSendCommand(byte _cmd, byte _item, int16_t _value)
{
  if(!internalInit && (guidino_initialized || guino_executed || _cmd == guino_iamhere)  )
  {
    guino_data.cmd = _cmd;
    guino_data.item = _item;
    guino_data.value = _value;
    ET.sendData();
  }

}





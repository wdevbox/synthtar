#include <avr/pgmspace.h>
//MIDI command DEFs
#define VS1053_BANK_MELODY 0x79 //documentation pg 31
#define MIDI_NOTE_ON 0x90 //midi standard
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0
//PIN DEFINITIONS
#define VS1053_RESET 20
#define COL_1 2
#define COL_2 3
#define ROW_1 9
#define ROW_2 10
#define ROW_3 11
#define ROW_4 12
//MIDI DEFINITIONS
#define DEFAULT_ROOT 52 //default root note
#define DEFAULT_INST 30 //default instrument
//function + class declarations...
void initVS1053();
void midiInitStuff();
void midiSetInstrument(uint8_t, uint8_t);
void midiSetChannelVolume(uint8_t, uint8_t);
void midiSetChannelBank(uint8_t, uint8_t);
void midiNoteOn(uint8_t,uint8_t, uint8_t);
void midiNoteOff(uint8_t ,uint8_t, uint8_t);
void checkInputs();
void buttonTest();
void checkNeckInputs(int);
void checkBodyInputs(int);
class String_t;
//global vars
int tunings[4] = {0,4,7,11};
int global_root = DEFAULT_ROOT;
int fretCount = 0;
int transposition_factor = 0;
int instrument = DEFAULT_INST;
int vol = 30;
int previous_states = 0;
int prev_state = 0;
//there's another one after the class definition, i'll be able to move it after I move the class definition to an included file

//class definition, may move to other file later once I figure out how to do that
class String_t{//need to be able to adjust volume
  public :
  bool isPlayingSound; uint8_t note; uint8_t tuning; //something has isPlaying reserved in teensy...
  //uint8_ts here reduces mem usage by quite a bit
  String_t(int r, int t, int i){
    note = r + t; //because by default on boot the fretCount is 0
    tuning = t;
    isPlayingSound = false;
    }
    
  void changeNote(int root, int fret){//not just root and tuning, also neck combination. passes in root because then i can use a loop to call it
    if(isPlayingSound){//this is for if you change neck comb while body keys pressed
      stopPlaying();
      note = root + tuning + fret;
      startPlaying();
      }
    if(!isPlayingSound){//this isn't triggering for some reason
      note = root + tuning + fret;
      Serial.print("Note:" + note);
      }
    }
    void changeTuning(int tune){//rework this
        tuning = tune; //must be set because used in changeNote
        changeNote(global_root, fretCount);
      }
    bool startPlaying(){
      //send the midi command
     // if(isPlayingSound) return false; //couldn't start playing because it's already playing
      midiNoteOn(0,note,127); //start playing
      isPlayingSound = true; //set val to true
      return true;
      }
    bool stopPlaying(){
      //send the midi command
     // if(!isPlayingSound) return false; //couldn't stop playing because it wasn't playing
      midiNoteOff(0,note,0);//is playing, turn off, set playing to false, return true to indicate success
      isPlayingSound = false;
      return true;
      }
  };
//
String_t Strings[4] = {String_t(global_root,tunings[0],DEFAULT_INST),
         String_t(global_root,tunings[1],DEFAULT_INST),
         String_t(global_root,tunings[2],DEFAULT_INST),
         String_t(global_root,tunings[3],DEFAULT_INST)
  };
void setup() {
  // put your setup code here, to run once:
  initVS1053();
  initPeripherals();
  Serial3.begin(31250);
  Serial.begin(115200);
  midiInitStuff();
}

void loop() {
  // put your main code here, to run repeatedly:
  checkInputs();
  //buttonTest();
  delay(5);
}
//INIT FUNCTIONS
void initPeripherals(){//initialize all input pins, also in the future may be to initialize screen
  pinMode(COL_1, OUTPUT); digitalWrite(COL_1, LOW); //init column 1, pull it low
  pinMode(COL_2, OUTPUT); digitalWrite(COL_2, LOW); //init column 2, pull it low
  pinMode(ROW_1, INPUT); //init row 1
  pinMode(ROW_2, INPUT); //init row 2
  pinMode(ROW_3, INPUT); //init row 3
  pinMode(ROW_4, INPUT); //init row 4
  }
void midiInitStuff(){
    midiSetChannelBank(0, VS1053_BANK_MELODY); //set instrument bank to melody
    midiSetInstrument(0, instrument); //select instrument for channel 0, test if this needs the notes to stop first @ some point if there's notes playing
    midiSetChannelVolume(0,vol); //set volume
  }
void initVS1053(){
  pinMode(VS1053_RESET, OUTPUT);
  digitalWrite(VS1053_RESET, LOW);
  delay(10);
  digitalWrite(VS1053_RESET, HIGH);
  delay(10);
  }
//MIDI FUNCTIONS
void midiSetInstrument(uint8_t chan, uint8_t inst){
    if(chan>15)return; if(inst>127)return;
    Serial3.write(MIDI_CHAN_PROGRAM | chan);
    Serial3.write(inst);
  }
void midiSetChannelVolume(uint8_t chan, uint8_t vol){
  if(chan > 15)return; if(vol>127)return;
  Serial3.write(MIDI_CHAN_MSG | chan);
  Serial3.write(MIDI_CHAN_VOLUME);
  Serial3.write(vol);
  }
void midiSetChannelBank(uint8_t chan, uint8_t bank){
  if(chan > 15)return; if(bank > 127) return; //why 127? what is this bank? nothing in docs.

  Serial3.write(MIDI_CHAN_MSG | chan);
  Serial3.write((uint8_t)MIDI_CHAN_BANK);
  Serial3.write(bank);
  }
void midiNoteOn(uint8_t chan,uint8_t n, uint8_t vel){
  if (chan > 15) return; if (n > 127) return; if (vel > 127) return;
  Serial3.write(MIDI_NOTE_ON | chan);
  Serial3.write(n);
  Serial3.write(vel);
  }
void midiNoteOff(uint8_t chan,uint8_t n, uint8_t vel){
  if (chan > 15) return;  if (n > 127) return;  if (vel > 127) return;
  Serial3.write(MIDI_NOTE_OFF | chan);
  Serial3.write(n);
  Serial3.write(vel);
  }
  //HELPER FUNCTIONS
void transpose(){
  global_root = DEFAULT_ROOT + transposition_factor;
  for(int i = 0; i < 4; i++){//update the note playing (if there is one playing), or update the note that it would play FOR EACH STRING
    Strings[i].changeNote(global_root, fretCount);
    }
  }
void tranUp(){
  transposition_factor++;
  transpose();
  }
void tranDown(){
  transposition_factor--;
  transpose();
  }
void changeInst(int inst){
  instrument = inst;
  midiSetInstrument(0,instrument);//test this at some point!!! 
  }
  //BUTTON FUNCTIONS
int checkMatrix(){//can store this output as global "last state" for all inputs in a single variable, then check if it changes on the next run!
  int inputs = 0;
  int num_cols = 2; int num_rows = 4;
  int cols[num_cols] = {COL_1, COL_2}; int rows[num_rows] = {ROW_1, ROW_2, ROW_3, ROW_4};
  for(int col = 0; col < num_cols; col++){//in this configuration, col1, row1 will be the rightmost bit 
    digitalWrite(cols[col], HIGH);//pull column high
    for(int row = 0; row < num_rows; row++){
      inputs |= digitalRead(rows[row])<<(col*num_rows+row);
      }
    digitalWrite(cols[col], LOW);//pull column low
    }
   // Serial.write(inputs);
  return inputs;
  }
void checkNeckInputs(int in){//neck inputs are col1, rows 1-4, bits are inverted
  int sum = 0;
  sum += (in&(0b1));//BUTTON1, 0b1
  sum += (in&(0b10));//BUTTON2, 0b10
  sum += (in&(0b100));//BUTTON3, 0b100
  sum += (in&(0b1000));//BUTTON4, 0b1000
  if(sum != fretCount){//neck state has changed, change the note
  //  Serial.print("Setting to " + sum);
    fretCount = sum;
    //for(String_t str : Strings){
      Strings[0].changeNote(global_root, fretCount);
     // }
    }
  }
void checkBodyInputs(int in){
  int pastval = 0; int presentval = 0;
  for(int i = 0; i < 4; i++){//check each button individually so you can trigger strings individually //MAYBE IMPLEMENT THE MIDI SHORTHAND THAT WILL SAVE A FEW BYTES
    presentval = in&(16<<i); //0b10000, 5th bit. col 2, row 1. decimal value is 16, then 32, then 64
    pastval = previous_states&(16<<i);//to compare to previous state, you need to get it first...
    if(presentval != pastval){//if there was a change....
        if(presentval == 0){//button low, released, stop playing note
          Strings[i].stopPlaying();
          }
        else{//button high, pressed, start playing note
          Strings[i].startPlaying();
          }
      }
    }
  }
/*  void buttonTest(){//just test one of the buttons to see if something there is causing a/the problem
    digitalWrite(COL_1, HIGH);
    if(prev_state != digitalRead(ROW_4)){
      if(prev_state){//high to low, button released
        Strings[0].stopPlaying();
        prev_state = 0;
        }
      else{//low to high, button pressed
         Strings[0].startPlaying();
         prev_state = 1;
        }
      }
      //digitalWrite(COL_1, LOW);
    }*/
void checkInputs(){
  int button_states = checkMatrix();
      checkNeckInputs(button_states); //we always check our neck inputs
  if(button_states != previous_states){//there was a change, process it
      //checkNeckInputs(button_states);
      checkBodyInputs(button_states);
      Serial.print(button_states);
    previous_states = button_states;//update the new states
    }
  }
  String intToHexString(int in){
    //std::stringstream sstream;
    //sstream << std::hex << in;
    //std::string result = sstream.str();
    //return result;
    }

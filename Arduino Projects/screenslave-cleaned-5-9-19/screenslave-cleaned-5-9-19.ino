//written for synthtar - 4-29-19

//originally conceived because i only had one of the hd44780 lcds without an i2c backpack and i didn't want to spend more money...
//now i'm kind of glad i went with it because my pwm circuit shat itself, i can just use this to control the pwm brightness

//program that accepts serial data from a master and writes to an i2c screen
//saves processing time on main cpu, because string operations and similar are costly
//has the benefit of also being able to control additional functions
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//Global vars...
  int tim = 500;
LiquidCrystal_I2C lcd(0x27,16,2);
char line1[17] = "Welcome to      ";
char line2[17] = "Synthtar        ";
byte startReceived = 0;
byte currentCommand = 0;
byte dataCount = 0;
byte incomingByte = 0;
byte desiredBytes = 0;
byte receivedBytes = 0;
byte dataBuffer[128]; //128 byte buffer for related data :)  
int power_led = 7;
int m1_led = 8;
int m2_led = 9; //amp light is controlled by physical switch
int pwm_pin = 5;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);//begin serial comms
  //set up serial listening???
  //also inits for talking to the screen!
  pinMode(power_led, OUTPUT);
  digitalWrite(power_led, 0);
  pinMode(m1_led, OUTPUT);
  digitalWrite(m1_led, 0);
  pinMode(m2_led, OUTPUT);
  digitalWrite(m2_led, 0);
  pinMode(pwm_pin, OUTPUT);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0,1);//move to line 2
  lcd.print(line2);
}

void loop() {
//ON BYTE RECEIVE!
  while(Serial.available() > 0){//there's stuff in the buffer to deal with!
    incomingByte = Serial.read();
    if(startReceived != 254){ //if you haven't gotten a start signal yet...
      //wait for the start signal!
        startReceived = checkByte(incomingByte); //this func will set the startReceived flag if the incoming byte is a start signal. should probably rename it
      }
    else{
      if(currentCommand == 0){//if we don't have a command already
          checkCommand(incomingByte); //make this an assignment at some point... like currentCommand = checkCommand... 
          } //right now it assigns it inside the function
      else{
        if(receivedBytes < desiredBytes-1){//don't have enough data... grab more!
                dataBuffer[receivedBytes] = incomingByte; //save the byte for later... tasty
                receivedBytes++;
              }
            else{//you got all the bytes (except 1), hooray!    //needed to split off the last byte to receive because otherwise it wouldn't trigger until the next byte came in... no good!
              dataBuffer[receivedBytes] = incomingByte;
              //you have the command and the data, now you can interpret/run it!
                runCommand(currentCommand, dataBuffer);
                //clear the data buffer (optional)
                receivedBytes = 0; //clear received bytes
                currentCommand = 0; //clear current command
                startReceived = 0; //wait for the next command to start
              }
        }    
      }
    }
  delay(5); 
  }

  //todo... maybe an intro sequence/animation?
byte checkByte(byte input){
  switch(input){
    case '>':
      return 254;
      //start character. this would set the state to be waiting for a command
    default: 
      return 0;
    }
  }
void checkCommand(byte input){
  switch(input){
      case 'b': currentCommand = 'b'; desiredBytes = 1; break; //little b for brightness... sets the pwm width to control LED brightness... set out of 255 by default. can be remapped from 1-100 pretty easily 
    case 'A': currentCommand = 'A'; desiredBytes = 4; break; //screen A... prints instrument number, volume, base note (midi/common (if have time)) and transposition. Transposition can be 0-32, 16 as center. 
    case 'B': currentCommand = 'B'; desiredBytes = 4; break; //screen B... prints current tuning, volume, transposition, and root note
    case 'T': currentCommand = 'T'; desiredBytes = 6; break; //screen T, for configuring tunings... needs to know: which tuning, what's selected, and the 4 values. 6 bytes. 
    case 'S': currentCommand = 'S'; desiredBytes = 6; break; //s is for screen! wait for a screen ID and data!
    case 'L': currentCommand = 'L'; desiredBytes = 1; break; //L is for lights, waiting for info to update the indicators or brightness!
    case 'F': currentCommand = 'F'; desiredBytes = 32; break; //full text, allows you to write the full 16 lines.
    case '\n': currentCommand = 0; startReceived = 0; break; //resets command and start receive, basically the equivalent of exit
    default: Serial.println("Invalid command!"); currentCommand = 0; startReceived = 0; break; //unknown command/garbage data, go back to waiting!
    }
  }
void runCommand(byte cmd, byte* data){                                                                                                              //actually do shit lmao
  char lin1[17];
  char lin2[17];
  String line1;
  String line2;
  byte sel;
  switch(cmd){
    case 'b':
    analogWrite(5, map(data[0],0,100,0,255)); //set width of PWM on pin 5 to a val (input is a 0-100 percentage!) 
      break;
    case 'A':
      lcd.clear();
      line1 = "Inst:" + String(data[0]) + " Vol:" + String(data[1]);
      line2 = "base:" + getBase(data[2]) + "Trns:" + getTrans(data[3]); 
      lcd.print(line1);
      lcd.setCursor(0,1);
      lcd.print(line2);
      break;  
    case 'B':
      lcd.clear();
      line1 = "tuning:" + String(data[0]) + " Vol:" + String(data[1]);
      line2 = "trns:" + getTrans(data[2]) + " Rt:" + getBase(data[3]);
      lcd.print(line1);
      lcd.setCursor(0,1);
      lcd.print(line2);
      break; //test string...
      //>B0tKA -- tuning 0, vol 116, trns -5, Rt F4
    case 'S': 
       lcd.clear();
       sprintf(lin1,"%c%c%c%c%c%c",data[0],data[1],data[2],data[3],data[4],data[5]);
       Serial.write(lin1);
       sprintf(lin2, "string test");   //if you use all 16, you can't use the other line!!! BECAUSE IT NEEDS THE END TERMINATOR. MAKE THE BUFFER ONE LONGER, 17
       lcd.print(lin1);
       lcd.setCursor(0,1);
       lcd.print(lin2);
    break; //screen command 1! prints the 3 characters sent after the >S (hopefully)
    case 'T': //test string... >T12PTW[
    //tuning 1, 2nd one selected, 0, 4, 7, 11
    //tuning...
    //what's selected...
    //4 values...
    sel = data[1]-'0';
    line1="t:"+String(data[0]-'0');
    if(sel == 1){line1+="*";} else line1+=" ";
    line1+="1:"+getTrans(data[2]) + " ";
    line1+="2:"+getTrans(data[3]);
    if(sel == 2){line1+="*";}
    line2+="   ";
    if(sel == 3){line2+="*";} else line2+=" ";
    line2+="3:"+getTrans(data[4]) + " ";
    line2+="4:"+getTrans(data[5]);
    if(sel == 4){line2+="*";} 
    lcd.clear();
    lcd.print(line1);
    lcd.setCursor(0,1);
    lcd.print(line2);
   
      break;
    case 'L': 
      if(data[0]&1){//first bit is set, set LED 1!
        digitalWrite(power_led, 1);
        }
      if(data[0]&2)digitalWrite(m1_led, 1); //same for 2nd bit...
      if(data[0]&4)digitalWrite(m2_led, 1); //same for 3rd...
    break; //light command! update the LEDs!
    case 'F': 
      lcd.clear();
      sprintf(lin1,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
      sprintf(lin2,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27],data[28],data[29],data[30],data[31]);
      lcd.print(lin1);
      lcd.setCursor(0,1);
      lcd.print(lin2);
      break;
    default: break;
    }
    //todo... make a function that formats these strings so i don't have to do it manually...
  }

  String getTrans(byte input){
    return String((int)input-80); //was 0-32, with 16 as midpoint. gives 16 on either side.
    //is now 64-96 with 80 as midpoint!
    //64-96, d-`. just need to remember to account for this on the sending end... ADD 80
    }
//test input will be:
//>A - screen a
//t - instrument 116
//0 - volume 48
//< - 60 - should be C4 or something
//K - transposition... make it -5, so 75, K
  //MIDI LOOKUP TABLE FROM BYTE TO GET COMMON NOTE NAME FOR SCREEN? IF WE HAVE PROGRAM SPACE, SURE <--- this could be realllly good
    String getBase(byte input){//is there a way to solve for it? that would trade processing time for storage...
    switch(input){
      case 12: return "C0 ";
      case 13: return "C#0";
      case 14: return "D0 ";
      case 15: return "D#0";
      case 16: return "E0 ";
      case 17: return "F0 ";
      case 18: return "F#0";
      case 19: return "G0 ";
      case 20: return "Ab0";
      case 21: return "A0 ";
      case 22: return "Bb0";
      case 23: return "B0 ";
      case 24: return "C1 ";
      case 25: return "C#1";
      case 26: return "D1 ";
      case 27: return "D#1";
      case 28: return "E1 ";
      case 29: return "F1 ";
      case 30: return "F#1";
      case 31: return "G1 ";
      case 32: return "Ab1";
      case 33: return "A1 ";
      case 34: return "Bb1";
      case 35: return "B1 ";
      case 36: return "C2 ";
      case 37: return "C#2";
      case 38: return "D2 ";
      case 39: return "D#2";
      case 40: return "E2 ";
      case 41: return "F2 ";
      case 42: return "F#2";
      case 43: return "G2 ";
      case 44: return "Ab2";
      case 45: return "A2 ";
      case 46: return "Bb2";
      case 47: return "B2 ";
      case 48: return "C3 ";
      case 49: return "C#3";
      case 50: return "D3 ";
      case 51: return "D#3";
      case 52: return "E3 ";
      case 53: return "F3 ";
      case 54: return "F#3";
      case 55: return "G3 ";
      case 56: return "Ab3";
      case 57: return "A3 ";
      case 58: return "Bb3";
      case 59: return "B3 ";
      case 60: return "C4 ";
      case 61: return "C#4";
      case 62: return "D4 ";
      case 63: return "D#4";
      case 64: return "E4 ";
      case 65: return "F4 ";
      case 66: return "F#4";
      case 67: return "G4 ";
      case 68: return "Ab4";
      case 69: return "A4 ";
      case 70: return "Bb4";
      case 71: return "B4 ";
      case 72: return "C5 ";
      case 73: return "C#5";
      case 74: return "D5 ";
      case 75: return "D#5";
      case 76: return "E5 ";
      case 77: return "F5 ";
      case 78: return "F#5";
      case 79: return "G5 ";
      case 80: return "Ab5";
      case 81: return "A5 ";
      case 82: return "Bb5";
      case 83: return "B5 ";
      case 84: return "C6 ";
      case 85: return "C#6";
      case 86: return "D6 ";
      case 87: return "D#6";
      case 88: return "E6 ";
      case 89: return "F6 ";
      case 90: return "F#6";
      case 91: return "G6 ";
      case 92: return "Ab6";
      case 93: return "A6 ";
      case 94: return "Bb6";
      case 95: return "B6 ";
      case 96: return "C7 ";
      case 97: return "C#7";
      case 98: return "D7 ";
      case 99: return "D#7";
      case 100: return "E7 ";
      case 101: return "F7 ";
      case 102: return "F#7";
      case 103: return "G7 ";
      case 104: return "Ab7";
      case 105: return "A7 ";
      case 106: return "Bb7";
      case 107: return "B7 ";
      case 108: return "C8 ";
      case 109: return "C#8";
      case 110: return "D8 ";
      case 111: return "D#8";
      case 112: return "E8 ";
      case 113: return "F8 ";
      case 114: return "F#8";
      case 115: return "G8 ";
      case 116: return "Ab8";
      case 117: return "A8 ";
      default: return "NA ";
      }
    }

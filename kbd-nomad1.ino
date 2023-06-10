#include <Keyboard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
//The (-1) parameter means that your OLED display doesn’t have a RESET pin. 
//If your OLED display does have a RESET pin, it should be connected to a GPIO. 
//In that case, you should pass the GPIO number as a parameter.
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define HANDEDNESS_PIN 18
#define KEY_NULL 0xFF
#define KEY_LAYER_UP 0xFE
#define KEY_LAYER_DOWN 0xFD

#define KEY_ASCII_QUOTE 0x27
#define KEY_ASCII_DOUBLEQUOTE 0x22
#define KEY_ASCII_COMMA 0x2C
#define KEY_ASCII_BACKSLASH 0x5C

#define RH_KBD_ADDRESS 0x01 // arbitrary

#define INSTRUCTION_NULL 0xFF
#define INSTRUCTION_ACTIVELAYER_BASE 0xFE
#define INSTRUCTION_ACTIVELAYER_LOWER 0xFD
#define INSTRUCTION_ACTIVELAYER_UPPER 0xFC
#define INSTRUCTION_CAPSLOCK_ON 0xFB
#define INSTRUCTION_CAPSLOCK_OFF 0xFA

bool debugRHKBD = false;  // flag to use serial coms on RH KBD for debugging

byte rows[] = {15, 14, 16, 10};
const int rowCount = sizeof(rows)/sizeof(rows[0]);

byte cols[] = {4, 5, 6, 7, 8, 9};
const int colCount = sizeof(cols)/sizeof(cols[0]);

byte keys[colCount][rowCount];
byte keyRHKBD = KEY_NULL;
bool keyDown = false;
bool tapShift = false;
bool tapCmd = false;
bool tapOpt = false;
bool tapCtrl = false;
bool capslock = false;
char activeLayer = 'b';
int leftHandBoard = 0;

int RXLED = 17; // The RX LED has a defined Arduino pin
int TXLED = 30; // The TX LED has a defined Arduino pin

//define the symbols on the buttons of the keypads
char baseMapLeft[rowCount][colCount] = {
  {KEY_TAB,'q','w','e','r','t'},
  {KEY_LEFT_CTRL,'a','s','d','f','g'},
  {KEY_LEFT_SHIFT,'z','x','c','v','b'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_LEFT_GUI,KEY_LAYER_UP,KEY_RETURN}
};

// This is back to front because I must have wired up the columns backwards
char baseMapRight[rowCount][colCount] = {
  {KEY_ESC,'p','o','i','u','y'},
  {KEY_ASCII_QUOTE,KEY_ASCII_DOUBLEQUOTE,'l','k','j','h'},
  {KEY_RIGHT_ALT,'/','.',KEY_ASCII_COMMA,'m','n'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_BACKSPACE,KEY_LAYER_DOWN,' '}
};

char upperMapLeft[rowCount][colCount] = {
  {KEY_TAB,'!','@','#','$','%'},
  {KEY_LEFT_CTRL,'1','2','3','4','5'},
  {KEY_NULL,'6','7','8','9','0'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_LEFT_GUI,KEY_LAYER_UP,KEY_RETURN}
};

// This is back to front because I must have wired up the columns backwards
char upperMapRight[rowCount][colCount] = {
  {KEY_ASCII_BACKSLASH,')','(','*','&','^'},
  {'|',']','[','`','=','-'},
  {'?','}','{','~','+','_'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_DELETE,KEY_LAYER_DOWN,' '}
};

char lowerMapLeft[rowCount][colCount] = {
  {KEY_NULL,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5},
  {KEY_NULL,'1','2','3','4','5'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,'!'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,KEY_LAYER_UP,KEY_NULL}
};

// This is back to front because I must have wired up the columns backwards
char lowerMapRight[rowCount][colCount] = {
  {KEY_F11,KEY_F10,KEY_F9,KEY_F8,KEY_F7,KEY_F6},
  {KEY_NULL,KEY_NULL,KEY_RIGHT_ARROW,KEY_UP_ARROW,KEY_DOWN_ARROW,KEY_LEFT_ARROW},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,';',':'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_DELETE,KEY_LAYER_DOWN,' '}
};

void setup() {
  // which keyboard half are we running in?
  // pin high = left
  // pin low = right
  pinMode(HANDEDNESS_PIN, INPUT_PULLUP);
  leftHandBoard = digitalRead(HANDEDNESS_PIN);

  if (leftHandBoard || debugRHKBD) {
	  Serial.begin(9600);
    while (!Serial) ;

    // for(int x=0; x<rowCount; x++) {
    // 	Serial.print(rows[x]); Serial.println(" as input-pullup");
    // 	pinMode(rows[x], INPUT_PULLUP);
    // }

    // for (int x=0; x<colCount; x++) {
    // 	Serial.print(cols[x]); Serial.println(" as input");
    // 	pinMode(cols[x], INPUT);
    // }
  }

  if (leftHandBoard) {
    Wire.begin();
    Keyboard.begin();
  } else {
    Wire.begin(RH_KBD_ADDRESS);
    Wire.onReceive(receiveEventFromMaster);
    Wire.onRequest(requestEventFromMaster);
  }

  pinMode(RXLED, OUTPUT); // Set RX LED as an output
  pinMode(TXLED, OUTPUT); // Set TX LED as an output
  digitalWrite(RXLED, LOW);
  digitalWrite(TXLED, LOW);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (leftHandBoard) {
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();

    displayKeymap(activeLayer, true);
    displayDivider();
    displayKeymap(activeLayer, false);
  }
}

void receiveEventFromMaster(int bytes) {
  byte instruction = Wire.read();    // read one character from the I2C

  switch (instruction) {
    case INSTRUCTION_ACTIVELAYER_BASE:
      activeLayer = 'b';
      break;
    case INSTRUCTION_ACTIVELAYER_LOWER:
      activeLayer = 'l';
      break;
    case INSTRUCTION_ACTIVELAYER_UPPER:
      activeLayer = 'u';
      break;
    case INSTRUCTION_CAPSLOCK_ON:
      capslock = true;
      break;
    case INSTRUCTION_CAPSLOCK_OFF:
      capslock = false;
      break;
  }
}

void requestEventFromMaster() {
  Wire.write(keyRHKBD);
  keyRHKBD = KEY_NULL;
}

void displayLayer(char layer) {
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(115, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  switch (layer) {
    case 'u':
      display.print("U");
      break;
    case 'l':
      display.print("L");
      break;
    default:
      display.print("B");
  }

  display.display();
}

void displayKeymap(char layer, bool left) {
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
    for (int colIndex=0; colIndex < colCount; colIndex++) {	
      if (left) {
        display.setCursor(colIndex * 10, rowIndex * 11);
      } else {
        display.setCursor(128 - 10 - colIndex * 10, rowIndex * 11);
      }

      switch (layer) {
        case 'u':
          if (left) {
            display.print(upperMapLeft[rowIndex][colIndex]);
          } else {
            display.print(upperMapRight[rowIndex][colIndex]);
          }
          break;
        case 'l':
          if (left) {
            display.print(lowerMapLeft[rowIndex][colIndex]);
          } else {
            display.print(lowerMapRight[rowIndex][colIndex]);
          }
          break;
        default:
          if (left) {
            display.print(baseMapLeft[rowIndex][colIndex]);
          } else {
            display.print(baseMapRight[rowIndex][colIndex]);
          }
      }
    }
  }

  display.display();
}

void displayDivider() {
  display.drawLine(63, 0, 63, display.height()-1, SSD1306_WHITE);
  display.display();
}

void readMatrix() {
	// iterate the rows
	for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
		// row: set to output to low
		byte curRow = rows[rowIndex];
		pinMode(curRow, OUTPUT);
		digitalWrite(curRow, LOW);

		// col: interate through the columns
		for (int colIndex=0; colIndex < colCount; colIndex++) {
			byte curCol = cols[colIndex];
			pinMode(curCol, INPUT_PULLUP);
			keys[colIndex][rowIndex] = digitalRead(curCol);
			pinMode(curCol, INPUT);
		}
		// disable the row
		pinMode(curRow, INPUT);
	}
}

byte checkForKeypress() {
  byte key = KEY_NULL;

	for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
		for (int colIndex=0; colIndex < colCount; colIndex++) {	
      if (keys[colIndex][rowIndex] == 0) {
        switch (activeLayer) {
          case 'u':
            if (leftHandBoard) {
              key = upperMapLeft[rowIndex][colIndex];
            } else {
              key = upperMapRight[rowIndex][colIndex];
            }
            break;
          case 'l':
            if (leftHandBoard) {
              key = lowerMapLeft[rowIndex][colIndex];
            } else {
              key = lowerMapRight[rowIndex][colIndex];
            }
            break;
          default:
            if (leftHandBoard) {
              key = baseMapLeft[rowIndex][colIndex];
            } else {
              key = baseMapRight[rowIndex][colIndex];
            }
        }
      }
    }
  }

  return key;
}

void printMatrix() {
	for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
		if (rowIndex < 10)
			Serial.print(F("0"));
		Serial.print(rowIndex); Serial.print(F(": "));

		for (int colIndex=0; colIndex < colCount; colIndex++) {	
      if (keys[colIndex][rowIndex]) {
        // Because of the way the diodes are wired, a '1' in the key matrix means NOT pressed.
        // A '0' in the key matrix means pressed (because the keypress pulls the column to 
        // ground via the row, which is tied to ground during that rows scan)
			  Serial.print('-');
      } else {
			  Serial.print('!');
      }
			if (colIndex < colCount-1)
				Serial.print(F(", "));
		}	
		Serial.println("");
	}
}

void loop() {
  byte key = KEY_NULL;
  byte rhKey;

  readMatrix();
  key = checkForKeypress();

  if (leftHandBoard) {
    Wire.requestFrom(RH_KBD_ADDRESS, 1);
    while (Wire.available()) { // peripheral may send less than requested
      rhKey = Wire.read();
    }
    if (rhKey != KEY_NULL) {
      key = rhKey;
    }
  } else {
    if (debugRHKBD) {
      Serial.println(keyRHKBD);
    }
  }

  // was a key pressed
  if (key != KEY_NULL) {
    if (!keyDown) {
      //printMatrix();
      keyDown = true;

      if (!leftHandBoard) {
        //this is the key value we will send to the master (left hand board) when it asks for it
        keyRHKBD = key;
      } else {
        bool transmitKey = false;

        if (rhKey != KEY_NULL) {
          Serial.print("RH-KBD: ");
          Serial.println(rhKey);
        } else {
          Serial.print("LH-KBD: ");
          Serial.println(key);
        }

        switch (key) {
          case KEY_LEFT_SHIFT:
          case KEY_RIGHT_SHIFT:
            if (capslock) {
              capslock = false;
            } else if (tapShift) {
              capslock = true;
              tapShift = false;
            } else {
              tapShift = true;
            }

            Wire.beginTransmission(RH_KBD_ADDRESS);
            if (capslock) {
              Wire.write(INSTRUCTION_CAPSLOCK_ON);
            } else {
              Wire.write(INSTRUCTION_CAPSLOCK_OFF);
            }
            Wire.endTransmission();

            break;
          case KEY_LEFT_GUI:
          case KEY_RIGHT_GUI:
            if (tapCmd) {
              tapCmd = false;
            } else {
              tapCmd = true;
            }
            break;
          case KEY_LAYER_UP:
          case KEY_LAYER_DOWN:
            byte instruction;

            if (key == KEY_LAYER_UP) {
              if (activeLayer == 'b') {
                activeLayer = 'u';
                instruction = INSTRUCTION_ACTIVELAYER_UPPER;
              }
              if (activeLayer == 'l') {
                activeLayer = 'b';
                instruction = INSTRUCTION_ACTIVELAYER_BASE;
              }
            }
            if (key == KEY_LAYER_DOWN) {
              if (activeLayer == 'b') {
                activeLayer = 'l';
                instruction = INSTRUCTION_ACTIVELAYER_LOWER;
              }
              if (activeLayer == 'u') {
                activeLayer = 'b';
                instruction = INSTRUCTION_ACTIVELAYER_BASE;
              }
            }

            Wire.beginTransmission(RH_KBD_ADDRESS);
            Wire.write(instruction);
            Wire.endTransmission();
            Serial.print("Layer: ");
            Serial.println(activeLayer);

            display.clearDisplay();
            //displayLayer(activeLayer);
            displayKeymap(activeLayer, true);
            displayDivider();
            displayKeymap(activeLayer, false);
            break;
          default:
            transmitKey = true;
        }
  
        if (transmitKey) {
          if (tapShift || capslock || tapCmd || tapOpt || tapCtrl) {
            if (tapShift || capslock) {
              if (key >= 97 && key <= 122) {
                // only use shift if its an alpha key
                Keyboard.press(KEY_LEFT_SHIFT); 
              }
            } else if (tapCmd) {
              Keyboard.press(KEY_LEFT_GUI); 
            } else if (tapOpt) {
              Keyboard.press(KEY_LEFT_ALT); 
            } else if (tapCtrl) {
              Keyboard.press(KEY_LEFT_CTRL); 
            }

            Keyboard.press(key); 
            Keyboard.releaseAll();
          } else {
            Keyboard.write(key);
          }

          tapShift = false;
          tapCmd = false;
          tapOpt = false;
          tapCtrl = false;
        }
      }
    }
  } else {
    keyDown = false;
  }
}
#include "Keyboard.h"

byte rows[] = {15, 14, 16, 10};
const int rowCount = sizeof(rows)/sizeof(rows[0]);

byte cols[] = {4, 5, 6, 7, 8, 9};
const int colCount = sizeof(cols)/sizeof(cols[0]);

byte keys[colCount][rowCount];
bool keyDown = false;
bool tapShift = false;
bool capslock = false;
char activeLayer = 'b';
int leftHandBoard = 0;

int RXLED = 17; // The RX LED has a defined Arduino pin
int TXLED = 30; // The TX LED has a defined Arduino pin

#define HANDEDNESS_PIN 18
#define KEY_NULL 0xFF
#define KEY_LAYER_UPPER 0xFE
#define KEY_LAYER_LOWER 0xFD

#define KEY_ASCII_QUOTE 0x27
#define KEY_ASCII_DOUBLEQUOTE 0x22
#define KEY_ASCII_COMMA 0x2C
#define KEY_ASCII_BACKSLASH 0x5C

//define the symbols on the buttons of the keypads
char baseMapLeft[rowCount][colCount] = {
  {KEY_TAB,'q','w','e','r','t'},
  {KEY_LEFT_CTRL,'a','s','d','f','g'},
  {KEY_LEFT_SHIFT,'z','x','c','v','b'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_LEFT_GUI,KEY_LAYER_UPPER,KEY_RETURN}
};

// This is back to front because I must have wired up the columns backwards
char baseMapRight[rowCount][colCount] = {
  {KEY_ESC,'p','o','i','u','y'},
  {KEY_ASCII_QUOTE,KEY_ASCII_DOUBLEQUOTE,'l','k','j','h'},
  {KEY_RIGHT_ALT,'/','.',KEY_ASCII_COMMA,'m','n'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_BACKSPACE,KEY_LAYER_LOWER,' '}
};

char upperMapLeft[rowCount][colCount] = {
  {KEY_TAB,'!','@','#','$','%'},
  {KEY_LEFT_CTRL,'1','2','3','4','5'},
  {KEY_NULL,'6','7','8','9','0'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_LEFT_GUI,KEY_LAYER_UPPER,KEY_RETURN}
};

// This is back to front because I must have wired up the columns backwards
char upperMapRight[rowCount][colCount] = {
  {KEY_ASCII_BACKSLASH,')','(','*','&','^'},
  {'|',']','[','`','=','-'},
  {'?','}','{','~','+','_'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_DELETE,KEY_LAYER_LOWER,' '}
};

char lowerMapLeft[rowCount][colCount] = {
  {KEY_NULL,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5},
  {KEY_NULL,'1','2','3','4','5'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,KEY_LAYER_UPPER,KEY_NULL}
};

// This is back to front because I must have wired up the columns backwards
char lowerMapRight[rowCount][colCount] = {
  {KEY_F11,KEY_F10,KEY_F9,KEY_F8,KEY_F7,KEY_F6},
  {KEY_NULL,KEY_NULL,KEY_RIGHT_ARROW,KEY_UP_ARROW,KEY_DOWN_ARROW,KEY_LEFT_ARROW},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_NULL,';',':'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_DELETE,KEY_LAYER_LOWER,' '}
};

void setup() {
	Serial.begin(115200);
  while (!Serial) ;

  Keyboard.begin();

	for(int x=0; x<rowCount; x++) {
		Serial.print(rows[x]); Serial.println(" as input-pullup");
		pinMode(rows[x], INPUT_PULLUP);
	}

	for (int x=0; x<colCount; x++) {
		Serial.print(cols[x]); Serial.println(" as input");
		pinMode(cols[x], INPUT);
	}

  // which keyboard half are we running in?
  // pin high = left
  // pin low = right
  pinMode(HANDEDNESS_PIN, INPUT_PULLUP);
  leftHandBoard = digitalRead(HANDEDNESS_PIN);

  pinMode(RXLED, OUTPUT); // Set RX LED as an output
  pinMode(TXLED, OUTPUT); // Set TX LED as an output
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
	readMatrix();

  // was a key pressed
  byte key = checkForKeypress();
  if (key != KEY_NULL) {
    if (!keyDown) {
      //printMatrix();

      bool transmitKey = false;


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
          
          break;
        case KEY_LAYER_UPPER:
          if (activeLayer == 'b') {
            activeLayer = 'u';
            digitalWrite(RXLED, HIGH);
          } else {
            activeLayer = 'b';
            digitalWrite(RXLED, LOW);
            digitalWrite(TXLED, LOW);
          }
          break;
        case KEY_LAYER_LOWER:
          if (activeLayer == 'b') {
            activeLayer = 'l';
            digitalWrite(TXLED, HIGH);
          } else {
            activeLayer = 'b';
          }
          break;
        default:
          transmitKey = true;
      }
 
      if (transmitKey) {
        if (tapShift || capslock) {
          if (key >= 97 && key <= 122) {
            // only use shift if its an alpha key
            Keyboard.press(KEY_LEFT_SHIFT); 
          }
          Keyboard.press(key); 
          Keyboard.releaseAll();
        } else {
          Keyboard.write(key);
        }
        Serial.println(key);

        tapShift = false;
      }

      keyDown = true;
    }
  } else {
    keyDown = false;
  }
}
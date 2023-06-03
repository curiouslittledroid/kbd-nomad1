#include "Keyboard.h"

byte rows[] = {15, 14, 16, 10};
const int rowCount = sizeof(rows)/sizeof(rows[0]);

byte cols[] = {4, 5, 6, 7, 8, 9};
const int colCount = sizeof(cols)/sizeof(cols[0]);

byte keys[colCount][rowCount];
bool keyDown = false;

#define KEY_NULL 0xFF
#define KEY_LAYER 0xFE

//define the symbols on the buttons of the keypads
char baseMapLeft[rowCount][colCount] = {
  {KEY_TAB,'q','w','e','r','t'},
  {KEY_LEFT_CTRL,'a','s','d','f','g'},
  {KEY_LEFT_SHIFT,'z','x','c','v','b'},
  {KEY_NULL,KEY_NULL,KEY_NULL,KEY_LEFT_GUI,KEY_LAYER,KEY_RETURN}
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
  bool result = false;
  byte key = KEY_NULL;

	for (int rowIndex=0; rowIndex < rowCount; rowIndex++) {
		for (int colIndex=0; colIndex < colCount; colIndex++) {	
      if (keys[colIndex][rowIndex] == 0) {
        result = true;
        // Serial.print(colIndex);
        // Serial.print(':');
        // Serial.println(rowIndex);
      
        // which key
        key = baseMapLeft[rowIndex][colIndex];
        //Serial.println(key);
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
 
      Keyboard.write(key);
      Keyboard.releaseAll();
      Serial.println(key);
      keyDown = true;
    }
  } else {
    keyDown = false;
  }
}
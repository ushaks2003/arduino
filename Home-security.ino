#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Keypad.h>

// RFID setup
#define RST_PIN 9
#define SS_PIN 10
MFRC522 rfid(SS_PIN, RST_PIN);

// I2C LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change the I2C address (0x27) if needed

// Servo setup
Servo doorServo;
const int servoPin = A2; // Connect the servo signal pin to pin 7

// RGB LED setup
const int redPin = A0;    // Connect to the red pin of the RGB LED
const int greenPin = A1;  // Connect to the green pin of the RGB LED
const int bluePin = A3;   // Connect to the blue pin of the RGB LED

// List of authorized UIDs
const String authorizedUIDs[] = {
  "7382772A", // Replace with your first RFID UID
  "9390F413", // Replace with your second RFID UID (keychain)
  "3F2F013"  // Add more UIDs as needed
};
const int totalUIDs = sizeof(authorizedUIDs) / sizeof(authorizedUIDs[0]);

// PIN Setup
const String correctPIN = "1234";  // Replace with your desired PIN
String enteredPIN = "";

// Keypad Setup
const byte ROW_NUM    = 4;  // four rows
const byte COLUMN_NUM = 4;  // four columns
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte pin_rows[ROW_NUM] = {9, 8, 7, 6};  // Row pin numbers
byte pin_columns[COLUMN_NUM] = {5, 4, 3, 2};  // Column pin numbers
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_columns, ROW_NUM, COLUMN_NUM);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);
  
  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();

  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();
  lcd.print("System Ready...");
  delay(2000);
  lcd.clear();
  lcd.print("Scan Card or Pin");

  // Initialize Servo
  doorServo.attach(servoPin);
  doorServo.write(90); // Start with door locked (0 degrees)

  // Initialize RGB LED pins
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  turnOffRGB(); // Turn off RGB LED initially
}

void loop() {
  // Check for RFID tag
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String readUID = getUID();
    lcd.clear();
    lcd.print("Card Scanned:");
    delay(1000);
    lcd.clear();
    lcd.print(readUID);

    if (isAuthorized(readUID)) {
      lcd.setCursor(0, 1);
      lcd.print("Access Granted!");
      setRGBColor(0, 255, 0); // Green for unlock
      unlockDoor(); // Trigger servo to unlock door
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Access Denied!");
      setRGBColor(255, 0, 0); // Red for access denied
      delay(3000);
    }
    turnOffRGB(); // Turn off RGB LED after indicating status
    lcd.clear();
    lcd.print("Scan Card or Enter Pin");
    rfid.PICC_HaltA(); // Halt the card after reading
  }

  // Check for PIN input
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);  // Print the key pressed to the Serial Monitor

    if (key == '#') {
      // Trim and compare the entered PIN with the correct PIN
      enteredPIN.trim(); // Remove any extra spaces or non-numeric characters
      Serial.print("Entered PIN (trimmed): ");
      Serial.println(enteredPIN);  // Print the entered PIN to Serial Monitor for debugging

      if (enteredPIN == correctPIN) {
        lcd.clear();
        lcd.print("Pin Correct!");
        setRGBColor(0, 255, 0); // Green for unlock
        unlockDoor(); // Trigger servo to unlock door
      } else {
        lcd.clear();
        lcd.print("Incorrect PIN!");
        setRGBColor(255, 0, 0); // Red for access denied
        delay(2000);
      }
      turnOffRGB(); // Turn off RGB LED after indicating status
      enteredPIN = ""; // Reset PIN after entry
      lcd.clear();
      lcd.print("Scan Card or Enter Pin");
    } else if (key == '*') {
      enteredPIN = ""; // Clear PIN input
      lcd.clear();
      lcd.print("Enter Pin: ");
    } else {
      enteredPIN += key; // Add digit to entered PIN
      lcd.clear();
      lcd.print("Enter Pin: ");
      
      // Display asterisks for entered PIN
      String displayPIN = "";
      for (int i = 0; i < enteredPIN.length(); i++) {
        displayPIN += "*";
      }
      
      lcd.setCursor(0, 1);
      lcd.print(displayPIN); // Show entered PIN as asterisks
    }
  }
}

// Function to get UID from RFID
String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// Function to check if UID is authorized
bool isAuthorized(String uid) {
  for (int i = 0; i < totalUIDs; i++) {
    if (authorizedUIDs[i] == uid) {
      return true;
    }
  }
  return false;
}

// Function to unlock the door using the servo motor
void unlockDoor() {
  lcd.clear();
  lcd.print("Door Unlocked");
  doorServo.write(0); // Unlock door (90 degrees)
  delay(5000);         // Wait 5 seconds
  doorServo.write(90);  // Lock door again (0 degrees)
  lcd.clear();
  lcd.print("Door Locked");
}

// Function to set RGB LED color
void setRGBColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// Function to turn off RGB LED
void turnOffRGB() {
  setRGBColor(0, 0, 0);
}

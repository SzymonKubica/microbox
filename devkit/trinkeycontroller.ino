#include "Adafruit_seesaw.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>

/*
 * This sketch is targeting the adafruit trinkey controller to read input from
 * the stemma qt seesaw controller and send it via HID events to the microbox
 * emulator the idea here is to allow for replicating the console input while
 * using the emulator to achieve a tight feedback loop and get a feel for what
 * the game is going to feel like befre flashing it into the target console.
 */

Adafruit_USBD_HID usb_hid;

uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

#define BUTTON_X 6
#define BUTTON_Y 2
#define BUTTON_A 5
#define BUTTON_B 1
#define BUTTON_SELECT 0
#define BUTTON_START 16

/**
 * The joystick reports the current position using two potentiometers. Those
 * are read using analog pins that return values in range 0-1023. The two
 * constants below control how quickly the joystick registers input.
 */
#define HIGH_THRESHOLD 900
#define LOW_THRESHOLD 100

uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) |
                       (1UL << BUTTON_START) | (1UL << BUTTON_A) |
                       (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

// How many internal neopixels do we have? some boards have more than one!
#define NUMPIXELS 1

Adafruit_seesaw ss(&Wire);

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// the setup routine runs once when you press reset:
void setup()
{
        Serial.begin(115200);

        usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
        usb_hid.begin();

        delay(1000); // wait for host

#if defined(NEOPIXEL_POWER)
        // If this board has a power control pin, we must set it to output and
        // high in order to enable the NeoPixels. We put this in an #if defined
        // so it can be reused for other boards without compilation errors
        pinMode(NEOPIXEL_POWER, OUTPUT);
        digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

        pixels.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
        pixels.setBrightness(20); // not so bright

        Serial.println("Setting up seesaw I2C interface...");

        if (!ss.begin(0x50)) {
                Serial.println("ERROR! seesaw not found");
        }
        Serial.println("seesaw started");
        uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
        if (version != 5743) {
                Serial.print("Wrong firmware loaded? ");
                Serial.println(version);
        }

        Serial.println("Found Product 5743");

        ss.pinModeBulk(button_mask, INPUT_PULLUP);
        ss.setGPIOInterrupts(button_mask, 1);
        pinMode(38, INPUT);
}

void poll_directional_input()
{
        // Reverse x/y values to match joystick orientation
        int x = ss.analogRead(14);
        int y = ss.analogRead(15);

        Serial.println("x: " + String(x) + " y: " + String(y));

        if (x < LOW_THRESHOLD) {
                uint8_t keycode[6] = {HID_KEY_ARROW_RIGHT};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
        if (x > HIGH_THRESHOLD) {
                uint8_t keycode[6] = {HID_KEY_ARROW_LEFT};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
        if (y < LOW_THRESHOLD) {
                uint8_t keycode[6] = {HID_KEY_ARROW_UP};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
        if (y > HIGH_THRESHOLD) {
                uint8_t keycode[6] = {HID_KEY_ARROW_DOWN};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
};

void poll_action_input()
{

        uint32_t buttons = ss.digitalReadBulk(button_mask);
        // reject impossible results due to I2C delays caused by heavy SPI load.
        if (buttons == 0 || buttons == 0xFFFFFFFF)
                return;

        if (!(buttons & (1UL << BUTTON_A))) {
                uint8_t keycode[6] = {HID_KEY_R};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
        if (!(buttons & (1UL << BUTTON_B))) {
                uint8_t keycode[6] = {HID_KEY_G};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
        if (!(buttons & (1UL << BUTTON_Y))) {
                uint8_t keycode[6] = {HID_KEY_B};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
        if (!(buttons & (1UL << BUTTON_X))) {
                uint8_t keycode[6] = {HID_KEY_Y};
                usb_hid.keyboardReport(0, 0, keycode);
                delay(100);
                usb_hid.keyboardRelease(0);
                return;
        }
}

void loop()
{

        while (true) {
                if (usb_hid.ready()) {
                        poll_directional_input();
                        poll_action_input();
                        delay(50);
                }
        }
}

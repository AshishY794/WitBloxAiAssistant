/*
 * Sonar → ESP32 boot trigger (GPIO 0)
 *
 * Wiring:
 *   OUT (pin 5) → ESP32 boot button wire (GPIO 0)
 *   GND         → ESP32 GND
 *
 * CRITICAL:
 *   Boot button needs LOW = press.
 *   If OUT stays HIGH the whole time someone is near → ESP32 will NEVER trigger.
 *   We pulse LOW once when person ENTERS, then stay HIGH.
 *   LED is ON only while person is in range, OFF when they leave.
 */

#include <Arduino.h>

#define TRIG_PIN 9
#define ECHO_PIN 8
#define LED 2
#define OUT 5

float duration;
int threshold;

bool objectPresent = false;
int clearCount = 0;
int hitCount = 0;
unsigned long lastTriggerMs = 0;

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
    unsigned long start = micros();
    while (digitalRead(pin) != state) {
        if (micros() - start > timeout) return 0;
    }
    unsigned long pulseStart = micros();
    while (digitalRead(pin) == state) {
        if (micros() - pulseStart > timeout) return 0;
    }
    return micros() - pulseStart;
}

float readDistanceOnce()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(3);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH, 25000);
    if (duration == 0) return 999;

    float d = duration * 0.034 / 2;
    if (d < 1 || d > 100) return 999;
    return d;
}

float readDistance()
{
    float sum = 0;
    int valid = 0;
    for (int i = 0; i < 3; i++) {
        float d = readDistanceOnce();
        if (d < 999) {
            sum += d;
            valid++;
        }
        delay(15);
    }
    if (valid == 0) return 999;
    return sum / valid;
}

// Must go LOW then HIGH — this is the real "button press"
void clickBootButton()
{
    digitalWrite(OUT, HIGH);
    delay(5);
    digitalWrite(OUT, LOW);    // press
    delay(150);                // hold long enough for ESP32 to see it
    digitalWrite(OUT, HIGH);   // release
}

void setup()
{
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LED, OUTPUT);
    pinMode(OUT, OUTPUT);

    digitalWrite(OUT, HIGH);   // released
    digitalWrite(LED, LOW);    // off until detect
}

void loop()
{
    float distance = readDistance();
    int pot = analogRead(A0);
    threshold = map(pot, 0, 1023, 1, 80);

    bool near = (distance >= 1 && distance <= threshold);

    if (near) {
        hitCount++;
        clearCount = 0;
    } else {
        clearCount++;
        hitCount = 0;
    }

    // Need a few hits in a row before "arrived" (debounce)
    if (!objectPresent && hitCount >= 2) {
        objectPresent = true;
        digitalWrite(LED, HIGH);

        // ONE click for ESP32 when someone arrives
        if (millis() - lastTriggerMs > 2500) {
            clickBootButton();
            lastTriggerMs = millis();
        }
    }

    // Need several clears before OFF (stops flicker staying ON)
    if (objectPresent && clearCount >= 4) {
        objectPresent = false;
        digitalWrite(LED, LOW);
        digitalWrite(OUT, HIGH);  // ensure released / OFF idle
    }

    // While standing in range: LED ON, OUT stays HIGH (not pressed)
    if (objectPresent) {
        digitalWrite(LED, HIGH);
        digitalWrite(OUT, HIGH);
    }

    delay(40);
}

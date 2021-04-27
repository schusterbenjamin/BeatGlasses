#include <Arduino.h>
#include <arduinoFFT.h>
#include <analogWrite.h>

void setColorAndBrightness(uint16_t h, double s, double b);
void hsv2rgb(uint16_t h, uint32_t s, uint32_t v, int color[3]);

//FFT-MODE
#define SAMPLES 128        // Must be a power of 2
#define SAMPLING_FREQ 8500 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define NUM_BANDS 8        // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE 800          // Used as a crude noise filter, values below this are ignored
#define FFT_THRESHOLD 3500

//Color-Fade-For-FFT-Mode
#define UPPER_LIMIT 33000
#define LOWER_LIMIT 8000
static int FFT_COLOR_FADING_STEP = 40;

//Envelope-Mode
const int ENV_THRESHOLD = 50;
#define NUM_VALS 20

//FADE-Mode
#define COLOR_FADING_STEP 30
int BRIGHTNES_FADING_SPEED = 1;

//TICK-Mode
#define TICK_COLOR_HUE 24000
#define TICK_SPEED 12 //times per second

//Microphone pins
#define AUDIO_IN_PIN 32 // Signal in on this pin
#define ENVELOPE_IN_PIN 33
#define GATE_IN_PIN 25

//LED stuff
#define LED_RED 27
#define LED_GREEN 12
#define LED_BLUE 13
#define LED_POWER 14

//Color
#define MAX_HUE_VALUE 34000
#define MAX_SATURATION_VALUE 255
#define MAX_BRIGHTNESS_VALUE 255
#define MIN_BRIGHTNESS_VALUE 20
#define DIMMING 1337
const int BRIGHTNESS_DIMMING = MAX_BRIGHTNESS_VALUE / 4;

//Modes
#define FFT_MODE 0
#define ENVELOPE_MODE 1
#define TICK_MODE 2
#define FADE_MODE 3
#define BUTTON_IN 34
static int BUTTON_PRESSED = 0;
static int BUTTON_PRESSED_OLD = 0;

int mode = ENVELOPE_MODE;

//variables used in different modes
double brightness = MIN_BRIGHTNESS_VALUE;
int current_hue = (UPPER_LIMIT + LOWER_LIMIT) / 2;

//Debug mode
#define VERBOSE 0

// Sampling and FFT stuff
unsigned int sampling_period_us;
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

void fft();
void tick();
void envelope();
void fade();
void makeBands();

void setup()
{
    pinMode(LED_POWER, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(AUDIO_IN_PIN, INPUT);
    pinMode(ENVELOPE_IN_PIN, INPUT);
    pinMode(GATE_IN_PIN, INPUT);
    pinMode(BUTTON_IN, INPUT);

    Serial.begin(9600);
    sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
}

void loop()
{
    digitalWrite(LED_POWER, HIGH);

    BUTTON_PRESSED = digitalRead(BUTTON_IN);

    if (BUTTON_PRESSED_OLD == 1 && BUTTON_PRESSED == 0)
    {
        switch (mode)
        {
        case FFT_MODE:
            mode = ENVELOPE_MODE;
            break;
        case ENVELOPE_MODE:
            mode = TICK_MODE;
            break;
        case TICK_MODE:
            mode = FADE_MODE;
            break;
        case FADE_MODE:
            current_hue = (UPPER_LIMIT + LOWER_LIMIT) / 2;
            mode = FFT_MODE;
            break;
        }
    }

    BUTTON_PRESSED_OLD = BUTTON_PRESSED;

    switch (mode)
    {
    case FFT_MODE:
        fft();
        break;
    case ENVELOPE_MODE:
        envelope();
        break;
    case TICK_MODE:
        tick();
        break;
    case FADE_MODE:
        fade();
        break;
    }
}

void fft()
{
    makeBands();

    int val = bandValues[0];

    Serial.println(current_hue);

    current_hue += FFT_COLOR_FADING_STEP;
    if (current_hue > UPPER_LIMIT)
        FFT_COLOR_FADING_STEP *= -1;
    if (current_hue < LOWER_LIMIT)
        FFT_COLOR_FADING_STEP *= -1;

    if (val > FFT_THRESHOLD)
    {
        setColorAndBrightness(current_hue, MAX_SATURATION_VALUE, MAX_BRIGHTNESS_VALUE);
    }
    else
    {
        setColorAndBrightness(current_hue, MAX_SATURATION_VALUE, DIMMING);
    }
}

void envelope()
{
    Serial.println(analogRead(ENVELOPE_IN_PIN));

    int val = analogRead(ENVELOPE_IN_PIN);

    if (val > ENV_THRESHOLD)
    {
        setColorAndBrightness(current_hue, MAX_SATURATION_VALUE, MAX_BRIGHTNESS_VALUE);
    }
    else
    {
        setColorAndBrightness(current_hue, MAX_SATURATION_VALUE, DIMMING);
    }
}

void tick()
{
    delay(1000 / TICK_SPEED);

    setColorAndBrightness(TICK_COLOR_HUE, MAX_SATURATION_VALUE, MAX_BRIGHTNESS_VALUE);

    delay(1000 / TICK_SPEED);

    setColorAndBrightness(TICK_COLOR_HUE, MAX_SATURATION_VALUE, MIN_BRIGHTNESS_VALUE);
}
void fade()
{

    current_hue += COLOR_FADING_STEP;
    if (current_hue > MAX_HUE_VALUE)
        current_hue = 0;

    brightness += BRIGHTNES_FADING_SPEED;

    if (brightness > MAX_BRIGHTNESS_VALUE || brightness < 7)
    {
        BRIGHTNES_FADING_SPEED *= -1;
        brightness += BRIGHTNES_FADING_SPEED;
    }

    setColorAndBrightness(current_hue, MAX_SATURATION_VALUE, brightness);

    delay(10);
}

void makeBands()
{
    if (VERBOSE)
        Serial.print("[");
    //Reset bandValues[]
    for (int i = 0; i < NUM_BANDS; i++)
    {
        if (VERBOSE)
        {
            if (i != NUM_BANDS - 1)
            {
                Serial.print(bandValues[i]);
                Serial.print(", ");
            }
            else
            {
                Serial.print(bandValues[i]);
                Serial.println("]");
            }
        }

        bandValues[i] = 0;
    }

    // Sample the audio pin
    for (int i = 0; i < SAMPLES; i++)
    {
        newTime = micros();
        vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 9.7uS on an ESP32
        // Serial.println(vReal[i]);
        vImag[i] = 0;
        while ((micros() - newTime) < sampling_period_us)
        { /* chill */
        }
    }

    // Compute FFT
    FFT.DCRemoval();
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(FFT_FORWARD);
    FFT.ComplexToMagnitude();

    // Analyse FFT results
    for (int i = 2; i < (SAMPLES / 2); i++)
    { // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
        if (vReal[i] > NOISE)
        { // Add a crude noise filter

            // if (NUM_BANDS == 8)
            // {
            if (i <= 3)
                bandValues[0] += (int)vReal[i];
            if (i > 3 && i <= 6)
                bandValues[1] += (int)vReal[i];
            if (i > 6 && i <= 13)
                bandValues[2] += (int)vReal[i];
            if (i > 13 && i <= 27)
                bandValues[3] += (int)vReal[i];
            if (i > 27 && i <= 55)
                bandValues[4] += (int)vReal[i];
            if (i > 55 && i <= 112)
                bandValues[5] += (int)vReal[i];
            if (i > 112 && i <= 229)
                bandValues[6] += (int)vReal[i];
            if (i > 229)
                bandValues[7] += (int)vReal[i];
            //}
            //else if (NUM_BANDS == 16){if (i <= 2)bandValues[0] += (int)vReal[i];if (i > 2 && i <= 3)bandValues[1] += (int)vReal[i];if (i > 3 && i <= 5)bandValues[2] += (int)vReal[i];if (i > 5 && i <= 7)bandValues[3] += (int)vReal[i];if (i > 7 && i <= 9)bandValues[4] += (int)vReal[i];if (i > 9 && i <= 13)bandValues[5] += (int)vReal[i];if (i > 13 && i <= 18)bandValues[6] += (int)vReal[i];if (i > 18 && i <= 25)bandValues[7] += (int)vReal[i];if (i > 25 && i <= 36)bandValues[8] += (int)vReal[i];if (i > 36 && i <= 50)bandValues[9] += (int)vReal[i];if (i > 50 && i <= 69)bandValues[10] += (int)vReal[i];if (i > 69 && i <= 97)bandValues[11] += (int)vReal[i];if (i > 97 && i <= 135)bandValues[12] += (int)vReal[i];if (i > 135 && i <= 189)bandValues[13] += (int)vReal[i];if (i > 189 && i <= 264)bandValues[14] += (int)vReal[i];if (i > 264)bandValues[15] += (int)vReal[i];}
        }
    }
}

void fadeCurrentHue()
{
}

void setColorAndBrightness(uint16_t h, double s, double b)
{
    if (b != DIMMING)
    {
        brightness = b;
    }
    else
    {
        brightness -= BRIGHTNESS_DIMMING;
        if (brightness < MIN_BRIGHTNESS_VALUE)
            brightness = MIN_BRIGHTNESS_VALUE;
    }

    int rgb[3];
    hsv2rgb(h, s, brightness, rgb);

    analogWrite(LED_RED, (int)(255 - (rgb[0])));
    analogWrite(LED_GREEN, (int)(255 - (rgb[1])));
    analogWrite(LED_BLUE, (int)(255 - (rgb[2])));
}

//################################# Color Conversion #############################################

void hsv2rgb(uint16_t h, uint32_t s, uint32_t v, int color[3])
{

    if (h >= MAX_HUE_VALUE)
        h -= MAX_HUE_VALUE;
    int16_t ph = h / 6;
    ph = ph % 2000;
    ph -= 1000;
    ph = 1000 - abs(ph);
    if (h == 0)
        ph = 0;
    uint32_t c = v * s;
    uint32_t x2 = c * ph;
    uint8_t x = x2 / 255000;
    uint8_t r, g, b;
    int32_t m = v * (255 - s) / 255;
    x += m;
    if (0 <= h && h < 6000)
    {
        r = v;
        g = x;
        b = m;
    } //rgb = [c, x, 0];
    if (6000 <= h && h < 12000)
    {
        r = x;
        g = v;
        b = m;
    } //rgb = [x, c, 0];
    if (12000 <= h && h < 18000)
    {
        r = m;
        g = v;
        b = x;
    } //rgb = [0, c, x];
    if (18000 <= h && h < 24000)
    {
        r = m;
        g = x;
        b = v;
    } //rgb = [0, x, c];
    if (24000 <= h && h < 30000)
    {
        r = x;
        g = m;
        b = v;
    } //rgb = [x, 0, c];
    if (30000 <= h && h < 36000)
    {
        r = v;
        g = m;
        b = x;
    } //rgb = [c, 0, x];

    color[0] = r;
    color[1] = g;
    color[2] = b;
}
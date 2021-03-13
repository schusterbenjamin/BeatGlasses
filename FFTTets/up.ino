#include <arduinoFFT.h>

//FFT-MODE
#define SAMPLES 64         // Must be a power of 2
#define SAMPLING_FREQ 4300 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE 100      // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define NUM_BANDS 8        // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE 200          // Used as a crude noise filter, values below this are ignored
#define FFT_THRESHOLD 500

//Envelope-Mode
#define ENV_THRESHOLD 100
#define NUM_VALS 50

//Microphone pins
#define AUDIO_IN_PIN A5 // Signal in on this pin
#define ENVELOPE_IN_PIN A6
#define GATE_IN_PIN A7

//LED pins
#define LED_RED 3
#define LED_GREEN 5
#define LED_BLUE 6

//Modes
#define FFT_MODE 0
#define ENVELOPE_MODE 1
#define TICK_MODE 2
#define FADE_MODE 3

int mode = 0;

//Debug mode
#define VERBOSE 0

// Sampling and FFT stuff
unsigned int sampling_period_us;
int oldBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

void fft();
void tick();
void envelope();
void fade();
int getBands();

void setup()
{
    Serial.begin(115200);
    sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
}

int brightness = 255;

void loop()
{
    switch (mode)
    {
    case 0:
        fft();
        break;
    case 1:
        envelope();
        break;
    case 2:
        tick();
        break;
    case 3:
        fade();
        break;
    }
}

void fft()
{
    if (getBands(NULL))
    {
        Serial.println("Error in fft()");
    }

    if (bandValues[0] > FFT_THRESHOLD)
    {
        setColor(0, 100, 0);
    }
    else
    {
        setColor(0, 5, 0);
    }
}

void envelope()
{
    int avg = 0;

    for (int i = 0; i < NUM_VALS; i++)
    {
        avg += analogRead(ENVELOPE_IN_PIN);
    }
    avg /= NUM_VALS;

    if (VERBOSE)
        Serial.println(avg);

    if (avg > ENV_THRESHOLD)
        setColor(0, 160, 0);
    else
        setColor(0, 6, 0);
}

void tick()
{
}
void fade()
{
}

//bV is array in which the bandValues are stored
//avgEnv is the average of the envelope input of taking SAMPLES times the an input value
int getBands(int *avgEnv)
{
    if (VERBOSE)
        Serial.print("[");
    // Reset bandValues[]
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

            if (NUM_BANDS == 8)
            {
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
            }
            else if (NUM_BANDS == 16)
            {
                //16 bands, 12kHz top band
                if (i <= 2)
                    bandValues[0] += (int)vReal[i];
                if (i > 2 && i <= 3)
                    bandValues[1] += (int)vReal[i];
                if (i > 3 && i <= 5)
                    bandValues[2] += (int)vReal[i];
                if (i > 5 && i <= 7)
                    bandValues[3] += (int)vReal[i];
                if (i > 7 && i <= 9)
                    bandValues[4] += (int)vReal[i];
                if (i > 9 && i <= 13)
                    bandValues[5] += (int)vReal[i];
                if (i > 13 && i <= 18)
                    bandValues[6] += (int)vReal[i];
                if (i > 18 && i <= 25)
                    bandValues[7] += (int)vReal[i];
                if (i > 25 && i <= 36)
                    bandValues[8] += (int)vReal[i];
                if (i > 36 && i <= 50)
                    bandValues[9] += (int)vReal[i];
                if (i > 50 && i <= 69)
                    bandValues[10] += (int)vReal[i];
                if (i > 69 && i <= 97)
                    bandValues[11] += (int)vReal[i];
                if (i > 97 && i <= 135)
                    bandValues[12] += (int)vReal[i];
                if (i > 135 && i <= 189)
                    bandValues[13] += (int)vReal[i];
                if (i > 189 && i <= 264)
                    bandValues[14] += (int)vReal[i];
                if (i > 264)
                    bandValues[15] += (int)vReal[i];
            }
            else
            {
                return -1;
            }
        }
    }
    return 0;
}

void setColor(int r, int g, int b)
{
    analogWrite(LED_RED, 255 - r);
    analogWrite(LED_GREEN, 255 - g);
    analogWrite(LED_BLUE, 255 - b);
}
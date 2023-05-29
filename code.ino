#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>


#define READ_PIN A5
#define LED_PIN 5
#define NUM_LEDS 30

#define MIC_LOW 190
#define MIC_HIGH 400 // variar o valor para obter vários efeitos de sensibilidade

#define SAMPLE_SIZE 20
#define LONG_TERM_SAMPLES 250
#define BUFFER_DEVIATION 400
#define BUFFER_SIZE 3

#define LAMP_ID 1

CRGB leds[NUM_LEDS];

struct averageCounter *samples;
struct averageCounter *longTermSamples;
struct averageCounter* sanityBuffer;

float globalHue;
float globalBrightness = 250;
int hueOffset = 120;
float fadeScale = 1.3;
float hueIncrement = 0.7;

struct averageCounter{
  uint16_t *samples;
  uint16_t sample_size;
  uint8_t counter;

  averageCounter(uint16_t size) {
	counter = 0;
	sample_size = size;
	samples = (uint16_t*) malloc(sizeof(uint16_t) * sample_size);
  }

  bool setSample(uint16_t val) {
	if (counter < sample_size) {
  	samples[counter++] = val;
  	return true;
	}
	else {
  	counter = 0;
  	return false;
	}
  }

  int computeAverage() {
	int accumulator = 0;
	for (int i = 0; i < sample_size; i++) {
  	accumulator += samples[i];
	}
	return (int)(accumulator / sample_size);
  }

};

void setup()
{
  globalHue = 0;
  samples = new averageCounter(SAMPLE_SIZE);
  longTermSamples = new averageCounter(LONG_TERM_SAMPLES);
  sanityBuffer	= new averageCounter(BUFFER_SIZE);
 
  while(sanityBuffer->setSample(250) == true) {}
  while (longTermSamples->setSample(200) == true) {}

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

  Serial.begin(115200); // Start the Serial communication to send messages to the computer
  FastLED.setBrightness(250);
  fill_rainbow(leds, NUM_LEDS, 0, 255 / NUM_LEDS);
  FastLED.show();
  delay(1500);
  delay(2);
  Serial.println('\n');
}

void loop()
{
	 uint32_t analogRaw = analogRead(READ_PIN);
	soundReactive(analogRaw);
  Serial.println(analogRaw);
  delay(0);
 
}

void soundReactive(int analogRaw) {

 int sanityValue = sanityBuffer->computeAverage();
 if (!(abs(analogRaw - sanityValue) > BUFFER_DEVIATION)) {
	sanityBuffer->setSample(analogRaw);
 }
  analogRaw = fscale(MIC_LOW, MIC_HIGH, MIC_LOW, MIC_HIGH * 4, analogRaw, 0.4);

  if (samples->setSample(analogRaw))
	return;
    
  uint16_t longTermAverage = longTermSamples->computeAverage();
  uint16_t useVal = samples->computeAverage();
  longTermSamples->setSample(useVal);

  int diff = (useVal - longTermAverage);
  if (diff > 5)
  {
	if (globalHue < 235)
	{
  	globalHue += hueIncrement;
	}
  }
  else if (diff < -5)
  {
	if (globalHue > 2)
	{
  	globalHue -= hueIncrement;
	}
  }


  int curshow = fscale(MIC_LOW, MIC_HIGH, 0, (float)NUM_LEDS, (float)useVal, 0);
  //int curshow = map(useVal, MIC_LOW, MIC_HIGH, 0, NUM_LEDS)

  for (int i = 0; i < NUM_LEDS; i++)
  {
	if (i < curshow)
	{
  	leds[i] = CHSV(globalHue + hueOffset + (i * 2), 255, 255);
	}
	else
	{
  	leds[i] = CRGB(leds[i].r / fadeScale, leds[i].g / fadeScale, leds[i].b / fadeScale);
	}
    
  }
  delay(5);
  FastLED.show();
}

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve)
{

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  bool invFlag = 0;

  // condition curve parameter
  // limit range

  if (curve > 10)
	curve = 10;
  if (curve < -10)
	curve = -10;

  curve = (curve * -.1);  // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin)
  {
	inputValue = originalMin;
  }
  if (inputValue > originalMax)
  {
	inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin)
  {
	NewRange = newEnd - newBegin;
  }
  else
  {
	NewRange = newBegin - newEnd;
	invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal = zeroRefCurVal / OriginalRange; // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax)
  {
	return 0;
  }

  if (invFlag == 0)
  {
	rangedValue = (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else // invert the ranges
  {
	rangedValue = newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}

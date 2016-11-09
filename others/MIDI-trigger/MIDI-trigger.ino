// This code is placed in the public domain by its author, Ian Harvey
// October 2016.

// For Pro Micro / Leonardo
//#define MIDI_SERIAL Serial1

static void MIDI_setup();
static void MIDI_noteOn(int ch, int note, int velocity);
static void MIDI_noteOff(int ch, int note);

const int MIDI_CHANNEL=1;

const int NCHANNELS = 4;
const int inPins[NCHANNELS] = { A0, A1, A2, A3 };
const int midiNotes[NCHANNELS] = 
{
  // Follows General MIDI specs at https://www.midi.org/specifications/item/gm-level-1-sound-set
  36, // C3, Kick
  38, // D3, Snare
  42, // F#3, Closed hi-hat
  46, // A#3. Open hi-hat
};
const int thresholdLevel[NCHANNELS] = { 30, 30, 30, 30 }; // ADC reading to trigger; lower => more sensitive
const long int maxLevel[NCHANNELS] = { 400, 400, 400, 400 }; // ADC reading for full velocity; lower => more sensitive

static unsigned int vmax[NCHANNELS] = { 0 };
static unsigned int trigLevel[NCHANNELS];
static unsigned int counter[NCHANNELS] = { 0 };

static unsigned int CTR_NOTEON = 10; // Roughly 5ms sampling peak voltage
static unsigned int CTR_NOTEOFF = CTR_NOTEON + 30; // Duration roughly 15ms 
// 0 -> not triggered
// 1..CTR_NOTEON -> sampling note on
// CTR_NOTEON+1 .. CTR_NOTEOFF -> note off


static int statusPin = 2;

void setup() {
  // put your setup code here, to run once:
//  Serial.begin(115200);
  analogReference(DEFAULT);
  pinMode(statusPin, OUTPUT);
  digitalWrite(statusPin, LOW);
  
  for (int i = 0; i < NCHANNELS; i++)
  {
    pinMode(inPins[i], INPUT);
    analogRead(inPins[i]);
    trigLevel[i] = thresholdLevel[i];
  }

//  MIDI_setup();
}


void loop() {
  int ch;
  for (ch=0; ch < NCHANNELS; ch++)
  {
    unsigned int v = analogRead(inPins[ch]);
    
    if ( counter[ch] == 0 )
    {
      if ( v >= trigLevel[ch] )
      {
        Serial.println(v);
        vmax[ch] = v;
        counter[ch] = 1;
        digitalWrite(statusPin, HIGH);
        
      }
    }
    else
    {
      if ( v > vmax[ch] )
        vmax[ch] = v;
      counter[ch]++;
      
      if ( counter[ch] == CTR_NOTEON )
      {
        long int vel = ((long int)vmax[ch]*127)/maxLevel[ch];
        //Serial.println(vel);
        if (vel < 5) vel = 5;
        if (vel > 127) vel = 127;
//        MIDI_noteOn(MIDI_CHANNEL, midiNotes[ch], vel);
        trigLevel[ch] = vmax[ch];
      }
      else if ( counter[ch] >= CTR_NOTEOFF )
      {
//        MIDI_noteOff(MIDI_CHANNEL, midiNotes[ch]);
        counter[ch] = 0;
        digitalWrite(statusPin, LOW);
      }
    }

    // The signal from the piezo is a damped oscillation decaying with
    // time constant 8-10ms. Prevent false retriggering by raising 
    // trigger level when first triggered, then decaying it to the 
    // threshold over several future samples.
    trigLevel[ch] = ((trigLevel[ch] * 19) + (thresholdLevel[ch] * 1)) / 20;
  }

}

// MIDI Code
//
// See https://www.midi.org/specifications/item/table-1-summary-of-midi-message

//void MIDI_setup()
//{
//  MIDI_SERIAL.begin(31250);
//}
//
//void MIDI_noteOn(int ch, int note, int velocity)
//{
//  MIDI_SERIAL.write( 0x90+((ch-1) & 0xF) );
//  MIDI_SERIAL.write( note & 0x7F );
//  MIDI_SERIAL.write( velocity & 0x7F );
//}
//
//void MIDI_noteOff(int ch, int note)
//{
//  MIDI_SERIAL.write( 0x80+((ch-1) & 0xF) );
//  MIDI_SERIAL.write( note & 0x7F );
//  MIDI_SERIAL.write( 0x01 );
//}


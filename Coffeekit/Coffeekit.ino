#include <MIDIUSB.h>

// This code is placed in the public domain by its author, Ian Harvey
// October 2016.

const int MIDI_CHANNEL=0;

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

// On table: 1000, stuck to paper cup: 40
const int DEFAULT_THRESHOLD = 50;
const int thresholdLevel[NCHANNELS] = { DEFAULT_THRESHOLD, DEFAULT_THRESHOLD, DEFAULT_THRESHOLD, DEFAULT_THRESHOLD }; // ADC reading to trigger; lower => more sensitive
const int DEFAULT_MAX = 1024;
const long int maxLevel[NCHANNELS] = { DEFAULT_MAX, DEFAULT_MAX, DEFAULT_MAX, DEFAULT_MAX }; // ADC reading for full velocity; lower => more sensitive

static unsigned int vmax[NCHANNELS] = { 0 };
static unsigned int trigLevel[NCHANNELS];
static unsigned int counter[NCHANNELS] = { 0 };

static unsigned long triggerTime[NCHANNELS] = {0};

static unsigned int CTR_NOTEON = 10; // Roughly 5ms sampling peak voltage
static unsigned int CTR_NOTEOFF = CTR_NOTEON + 30; // Duration roughly 15ms 
// 0 -> not triggered
// 1..CTR_NOTEON -> sampling note on
// CTR_NOTEON+1 .. CTR_NOTEOFF -> note off

// Decaying the trigger value over several samples to prevent false retriggering
const unsigned int TRIGGER_DECAY = 20;


//static int statusPin = 2;

void setup() {
  // put your setup code here, to run once:
//  Serial.begin(115200);
  analogReference(DEFAULT);
//  pinMode(statusPin, OUTPUT);
//  digitalWrite(statusPin, LOW);
  
  for (int i = 0; i < NCHANNELS; i++)
  {
    pinMode(inPins[i], INPUT);
    analogRead(inPins[i]);
    trigLevel[i] = thresholdLevel[i];
  }

  MIDI_setup();
}


void loop() {
  int ch;
  for (ch=0; ch < NCHANNELS; ch++)
//  for (ch=0; ch < 1; ch++)
  {
    unsigned int v = analogRead(inPins[ch]);
    
    if ( counter[ch] == 0 )
    {
      if ( v >= trigLevel[ch] )
      {
        vmax[ch] = v;
        counter[ch] = 1;
//        triggerTimes[ch] = System.
//        digitalWrite(statusPin, HIGH);
        
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
        
        // TODO: Map measured values to 1-127
        if (vel < 5) vel = 5;
        if (vel > 127) vel = 127;
//        Serial.println(vel);
//        MIDI_noteOn(MIDI_CHANNEL, midiNotes[ch], vel);
        noteOn(MIDI_CHANNEL, midiNotes[ch], 64);
        trigLevel[ch] = vmax[ch];
      }
      else if ( counter[ch] >= CTR_NOTEOFF )
      {
//        MIDI_noteOff(MIDI_CHANNEL, midiNotes[ch]);
        noteOff(MIDI_CHANNEL, midiNotes[ch], 64);
        counter[ch] = 0;
//        digitalWrite(statusPin, LOW);
      }
    }

    MidiUSB.flush();

    // The signal from the piezo is a damped oscillation decaying with
    // time constant 8-10ms. Prevent false retriggering by raising 
    // trigger level when first triggered, then decaying it to the 
    // threshold over several future samples.
    trigLevel[ch] = ((trigLevel[ch] * (TRIGGER_DECAY - 1)) + (thresholdLevel[ch] * 1)) / TRIGGER_DECAY;
  }

}

// MIDI Code

// See https://www.midi.org/specifications/item/table-1-summary-of-midi-message


// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  Serial.println(pitch);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  Serial.println("Off ");
}


void MIDI_setup() {
  Serial.begin(115200);
}


// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

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



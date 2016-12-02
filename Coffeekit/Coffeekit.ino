#include <MIDIUSB.h>
#include <SoftwareSerial.h>

#define LED_PIN 13
#define resetMIDI 4 // Tied to VS1053 (Sound module on instrument shield) Reset line

SoftwareSerial mySerial(2, 3); // RX, TX

const int MIDI_CHANNEL=0;

const int NCHANNELS = 4;
const int inPins[NCHANNELS] = { A0, A1, A2, A3};// A4, A5 };
const int midiNotes[NCHANNELS] = 
{
  // Follows General MIDI specs at https://www.midi.org/specifications/item/gm-level-1-sound-set
  36, // C3, Kick
  38, // D3, Snare
  42, // F#3, Closed hi-hat
  48 // A#3. Open hi-hat
//  48, // ?
//  56  // ?
};

// On table: 1000, stuck to paper cup: 40
const int DEFAULT_THRESHOLD = 50;
const int thresholdLevel[NCHANNELS] = 
{ 
  // ADC reading to trigger; lower => more sensitive
  DEFAULT_THRESHOLD, // Channel 0
  DEFAULT_THRESHOLD, // Channel 1
  DEFAULT_THRESHOLD, // Channel 2
  DEFAULT_THRESHOLD // Channel 3
//  DEFAULT_THRESHOLD, // Channel 4
//  10  // Channel 5, larger diaphragm less sensitive
}; 
const int DEFAULT_MAX = 1024;
// Set weaker mics to lower max levels
const long int maxLevel[NCHANNELS] = 
{ 
  // ADC reading for full velocity; lower => more sensitive
  DEFAULT_MAX, 
  DEFAULT_MAX, 
  500, 
  DEFAULT_MAX
//  DEFAULT_MAX,
//  500 // Larger diaphragm less sensititve 
}; 
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
  pinMode(LED_PIN, OUTPUT);
  
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

  MIDISetup();
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
        noteOn(MIDI_CHANNEL, midiNotes[ch], vel);
        turnLEDOn();
        trigLevel[ch] = vmax[ch];
      }
      else if ( counter[ch] >= CTR_NOTEOFF )
      {
//        MIDI_noteOff(MIDI_CHANNEL, midiNotes[ch]);
        int fakeVelocity = 64; // Need to feed the note off function with a velocity for some reason
        noteOff(MIDI_CHANNEL, midiNotes[ch], fakeVelocity);
        turnLEDOff();
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

void MIDISetup() {
//  usbMIDISetup();
  midiShieldSetup();
}

void noteOn(byte channel, byte pitch, byte velocity) {
//  midiUSBNoteOn(channel, pitch, velocity);
  midiShieldNoteOn(channel, pitch, velocity);
}

void noteOff(byte channel, byte pitch, byte velocity) {
//  midiUSBNoteOff(channel, pitch, velocity);
  midiShieldNoteOff(channel, pitch, velocity);
}


// ******************************************************
// MIDI USB Code
// ******************************************************
// See https://www.midi.org/specifications/item/table-1-summary-of-midi-message


// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void midiUSBNoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
//  Serial.println(pitch);
}

void midiUSBNoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
//  Serial.println("Off ");
}


void usbMIDISetup() {
  Serial.begin(115200);
}


// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void midiUSBControlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void turnLEDOn() {
  digitalWrite(LED_PIN, HIGH);
}

void turnLEDOff() {
  digitalWrite(LED_PIN, LOW);
}

// ****************************************************
// Sparkfun Instrument Shield
// ****************************************************
void midiShieldSetup() {
  // Setup soft serial for MIDI control
  mySerial.begin(31250);
  
  // Reset the VS1053
  pinMode(resetMIDI, OUTPUT);
  digitalWrite(resetMIDI, LOW);
  delay(100);
  digitalWrite(resetMIDI, HIGH);
  delay(100);
  talkMIDI(0xB0, 0x07, 120); // 0xB0 is channel message, set channel volume to near max (127)

  talkMIDI(0xB0, 0, 0x78); // Bank select drums
  int instrument = 0; // For the drum bank, instrument number does not matter. Needs to be set, though
  talkMIDI(0xC0, 0, 0); // Set instrument number. 0xC0 is a 1 data byte command
}

//Send a MIDI note-on message.  Like pressing a piano key
//channel ranges from 0-15
void midiShieldNoteOn(byte channel, byte note, byte attack_velocity) {
  talkMIDI( (0x90 | channel), note, attack_velocity);
}

//Send a MIDI note-off message.  Like releasing a piano key
void midiShieldNoteOff(byte channel, byte note, byte release_velocity) {
  talkMIDI( (0x80 | channel), note, release_velocity);
}

//Plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that data values are less than 127
void talkMIDI(byte cmd, byte data1, byte data2) {
//  digitalWrite(ledPin, HIGH);
  mySerial.write(cmd);
  mySerial.write(data1);

  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  if( (cmd & 0xF0) <= 0xB0)
    mySerial.write(data2);

//  digitalWrite(ledPin, LOW);
}

// *****************************************************************

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





// Thanks to Ian Harvey for code base


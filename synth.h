// pin bits
#define PIN_2_ON  B00000100
#define PIN_3_ON  B00001000
#define PIN_4_ON  B00010000
#define PIN_5_ON  B00100000
#define PIN_6_ON  B01000000
#define PIN_7_ON  B10000000
#define PIN_2_OFF B11111011
#define PIN_3_OFF B11110111
#define PIN_4_OFF B11101111
#define PIN_5_OFF B11011111
#define PIN_6_OFF B10111111
#define PIN_7_OFF B01111111

// array of speaker pins
#define NUM_VOICES 6
byte speakerOn[NUM_VOICES]  = {PIN_2_ON,PIN_3_ON,PIN_4_ON,PIN_5_ON,PIN_6_ON,PIN_7_ON};
byte speakerOff[NUM_VOICES] = {PIN_2_OFF,PIN_3_OFF,PIN_4_OFF,PIN_5_OFF,PIN_6_OFF,PIN_7_OFF};

// macros for tweaking the speaker pins
#define setSpeakerOutputs() DDRD  |= B11111100 // sets speaker pins as outputs, leaves the rest unchanged
#define setSpeakersOff()    PORTD &=   B11111111 // sets speaker pins LOW and leaves the rest unchanged
#define clickSpeaker(s)     PORTD ^=   speakerOn[s] // toggles specified speaker pin and leave the rest unchanged

// halt the system
void stop(char* msg) {
  Serial.println(msg);
  Serial.println("Halting");
  for (;;);
}

// voice data
struct SoundEvent {
  int periodDuration;  // pitch
  // this is calculated when event starts
  unsigned long periodStartTime;  // when period starts
};

void printSoundEvent(char *name, SoundEvent *p) {
  Serial.print("event: "); Serial.print(name); Serial.print(", address: "); Serial.println((int)p);
  Serial.print("periodDuration: "); Serial.print(p->periodDuration);
  Serial.print("  periodStartTime: "); Serial.println(p->periodStartTime);
}

// sound event that indicates end of event list
SoundEvent NO_SOUND = {0,0};

// events for each voice
struct SoundEvent voiceEvents[NUM_VOICES];

// status flags about playing notes and songs
boolean synthOnFlag = false;  // if we are to monitor note playing
boolean playingSound = false;  // if we are currently playing at least one voice
boolean playingSong = false;  // if we are currently playing a song

// start synthesizer
void synthOn() {
  setSpeakerOutputs();
  setSpeakersOff();
  synthOnFlag = true;
  playingSound = false;
  playingSong = false;

  // clear any sound events for the voices
  for (byte i=0; i<NUM_VOICES; i++) {
    voiceEvents[i] = NO_SOUND;
  }
}

// stop synthesizer
void synthOff() {
  synthOnFlag = false;
  playingSound = false;
  playingSong = false;
  setSpeakersOff();
}

// called repeatedly to create the notes for the 6 voices
void synthTick() {
  playingSound = false;  // assume we are not playing anything
  unsigned long now = micros();  // current time used in note calculations
  //Serial.print("now: "); Serial.println(now);

  // loop through all voices
  SoundEvent *pEvent = voiceEvents;  // point to voice 0 event
  for (byte voice=0; voice<NUM_VOICES; ++voice) {
    //Serial.print("voice "); Serial.println(voice);

    // point to event we will use
    //printSoundEvent("pEvent", pEvent);
    
    // see if we ignore the voice - no sound
    if (pEvent->periodDuration == 0) {
      //Serial.println("nothing to play for this voice");
      continue;  // try next voice
    } // ignore voice

    // if we get here, we must be playing a note
    playingSound = true;

    // see if period ends - time to click speaker
    if (now > pEvent->periodStartTime + pEvent->periodDuration) {
      // time to click and reset the period
      //Serial.print("click "); Serial.println(voice);
      clickSpeaker(voice);
      pEvent->periodStartTime += pEvent->periodDuration;
    }
    ++pEvent;  // point to next voice event  
  } // do all voices
} // tick

//-----------------------------------------------
// Start playing a note on a particular channel
//-----------------------------------------------
void playNote (byte voice, byte note) {
  unsigned int frequency2; /* frequency times 2 */

  if (note>127) note=127;

  // point to the event that controls this voice
  SoundEvent *pEvent = voiceEvents + voice;

  // frequencies are stored in program memory
  frequency2 = pgm_read_word (noteFrequencies2_PGM + note);

  // setting the voice period duration will start playing the note
  pEvent->periodDuration = 1000000/frequency2;
  pEvent->periodStartTime = micros();  // note period starts now
}

//-----------------------------------------------
// Stop playing a note on a particular channel
//-----------------------------------------------
void stopNote (byte voice) {
  // point to the event that controls this voice
  SoundEvent *pEvent = voiceEvents + voice;

  // clearing the voice period duration will stop playing the note
  pEvent->periodDuration = 0;

}

unsigned long songDelayLength, songDelayStart;  // no more note data until after this time
const byte *songStart = 0;  // start of song data
const byte *songCursor = 0;  // current position in song data

// called to start playing a song
void playSong (const byte *score) {
  songStart = score;
  songCursor = score;
  playingSong = true;
  songDelayLength = 0;  // no dealy, start reading data immediately
  songDelayStart = millis();  // no dealy, start reading data immediately
  synthOn();
}

// info about song data
#define CMD_PLAYNOTE  0x90  /* play a note: low nibble is generator #, note is next byte */
#define CMD_STOPNOTE  0x80  /* stop a note: low nibble is generator # */
#define CMD_RESTART 0xe0  /* restart the score from the beginning */
#define CMD_STOP  0xf0  /* stop playing */
/* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit big-endian number of msec to wait */

// called repeatedly to coordinate the notes of a song
void songTick() {
  // we are pointed at the song data where we continue playing
  byte cmd, opcode, chan;
  unsigned int duration;

  unsigned long now = millis();  // current time used in song delays
  if (songDelayStart + songDelayLength > now) {
    return;  // not time to read more data
  }

  for (;;) { // loop until we get a delay
    //Serial.print("songCursor "); Serial.println((unsigned int)songCursor - (unsigned int)songStart);
    //Serial.print("readByte "); Serial.println(pgm_read_byte(songCursor), HEX);
    cmd = pgm_read_byte(songCursor++);
    if (cmd < 0x80) { /* wait count in msec. */
      //Serial.print("readByte "); Serial.println(pgm_read_byte(songCursor));
      duration = ((unsigned)cmd << 8) | (pgm_read_byte(songCursor++));
      songDelayLength = duration;  // song delay
      songDelayStart = now;
      //Serial.print("delay "); Serial.println(duration);
      return;
    }

    opcode = cmd & 0xf0;
    chan = cmd & 0x0f;
  
    if (opcode == CMD_STOPNOTE) { /* stop note */
      //Serial.print("stopNote "); Serial.println(chan);
      stopNote (chan);
    } else if (opcode == CMD_PLAYNOTE) { /* play note */
      //Serial.print("readByte "); Serial.println(pgm_read_byte(songCursor), HEX);
      byte note = pgm_read_byte(songCursor++);
      //Serial.print("playNote "); Serial.print(chan); Serial.print(" "); Serial.println(note);
      playNote (chan, note);
    } else if (opcode == CMD_RESTART) { /* restart score */
      //Serial.println("restart");
      songCursor = songStart;
    } else if (opcode == CMD_STOP) { /* stop score */
      //Serial.println("song end");
      synthOff();
      songCursor--;  // stay on the stop command
      return;
    } // which opcode
    
  } // loop forever
} // songTick

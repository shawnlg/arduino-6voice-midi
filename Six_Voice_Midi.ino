#include "notes.h"
#include "synth.h"
#include "midi.h"

void setup() {
  Serial.begin(9600);
/*
  synthOn();
  playNote(0,62);
  playNote(1,66);
  playNote(2,57);
  playNote(3,38);
  playNote(4,76);
  playNote(5,79);
*/
  playSong(score2);
}

void loop() {
  if (synthOnFlag) {
    synthTick();
    songTick();
  }

  if (playingSound) {
    // can only do short-duration stuff here or sound gets messed up
  } else {
    // we are in a pause in the music, so we can do longer stuff here
  }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
}



# arduino-6voice-midi
Code plays up to 6 concurrent voices

This code runs on the Arduino Nano and probably any other type of Arduino.  It does not use any special library.

I had some specific goals when writing this code

1. Be able to drive a speaker directly without an amplifier
2. Be able to have any number of voices not dependent on hardware timers
3. Be able to run on an ATTiny85
4. Be able to import existing music.

With this in mind, I used the music format created in the Len Shustek in his *miditones* github repository.  He created a C program that takes a midi file and creates a simplified data structure that can be copied into an Arduin sketch.  He also has code to play this format on the Arduino, but it uses 1 timer per voice, so I did not use it.

# Hooking Up the Speaker

Each voice toggles one of the Arduino pins.  Pins 2-7 are used, so it produces 6 concurrent sounds.  Connect a 150 ohm resistor to each of the pins and connect all the other ends of the resistors to a speaker wire.  Connect the other speadker wire to ground.

# Importing and Playing Music

1. Go here to download the utility to convert MIDI files:
https://github.com/LenShustek/miditones/
2. Follow the instructions for creating the data structure needed for the Arduino.
3. Add the data to your sketch in the main file or in an include.
4. include notes.h and synth.h from this repository in your sketch.
5. In your setup() function, have this piece of code:

```
playSong(score2);
```

6. In your loop() method, have this code:

```
if (synthOnFlag) {
  synthTick();
  songTick();
}
```
7. If you want any of your code to execute, you will need to add it in the loop below the previous code like this:
```
if (playingSound) {
  // can only do short-duration stuff here or sound gets messed up
} else {
  // we are in a pause in the music, so we can do longer stuff here
}
```

The code is very CPU intensive, so not much else can be running while the song is playing.

#ifndef SONG_H
#define SONG_H

#define Song const uint8_t PROGMEM

Song music[] = {                // total song in bytes = 31 
                                // setup bytes 11
  0x03,                         // Number of tracks
  0x00, 0x00,                   // Address of track 0
  0x03, 0x00,                   // Address of track 1
  0x09, 0x00,                   // Address of track 2
  0x01,                         // Channel 0 entry track (PULSE)
  0x00,                         // Channel 1 entry track (SQUARE)
  0x06,                         // Channel 2 entry track (TRIANGLE)
  0x03,                         // Channel 3 entry track (NOISE)

  //"Track 0"                   // ticks = 0, bytes = 3
  0x40, 0,                      // FX: SET VOLUME: volume = 0
  0xFE,                         // RETURN

  //"Track 1"                   // ticks = 3072, bytes = 6
  0x40, 63,                     // FX: SET VOLUME: volume = 63
  0xFD, 31, 2,                  // REPEAT: count = 32 - track = 2 (32 * 96 ticks)
  0xFE,                         // RETURN

  //"Track2"                    // ticks = 96, bytes = 11
  0x00 + 25,                    // NOTE ON: note = 25 (delay 1 tick)
  0x50,  0x80 +2,               // FX: SET GLISSANDO: ticks = 2;
  0x9F + 32,                    // DELAY: 32 ticks
  0x50,  0x00 +2,               // FX: SET GLISSANDO: ticks = 2;
  0x9F + 32,                    // DELAY: 32 ticks
  0x51,                         // FX: GLISSANDO OFF
  0x00,                         // NOTE OFF (delay 1 tick)
  0x9F + 30,                    // DELAY: 30 ticks
  0xFE,                         // RETURN
  
};

#endif

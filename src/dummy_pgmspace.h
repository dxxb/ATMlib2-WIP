#define PROGMEM
#define pgm_read_word(ptr) (uint16_t)({uint8_t *__p = (uint8_t*)(ptr); ((uint16_t)(*(__p+1))<<8)|(*(__p));})
#define pgm_read_byte(ptr) ((uint8_t)*((uint8_t*)ptr))
#define memcpy_P(a,b,c) memcpy((a),(b),(c))

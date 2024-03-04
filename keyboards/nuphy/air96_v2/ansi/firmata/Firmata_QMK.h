
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Declare your wrapper functions here
void firmata_initialize(const char* firmware);
void firmata_begin(void);

void firmata_recv(uint8_t c);
void firmata_send_command(int command, const unsigned char* data, int length);

void firmata_process(void);

#ifdef __cplusplus
}
#endif


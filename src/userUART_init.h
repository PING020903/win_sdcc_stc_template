#ifndef _USERUART_INIT_H_
#define _USERUART_INIT_H_

#include <stdint.h>

void userUART_init(void);
void userUART_WriteByte(uint8_t c);
void userUART_WriteString(const char *s);
void userUART_FlushTx(void);
uint8_t userUART_Available(void);
int16_t userUART_ReadByte(void);

#endif /* _USERUART_INIT_H_ */

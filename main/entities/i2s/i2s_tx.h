#ifndef _I2S_TX_H_
#define _I2S_TX_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void i2s_tx_init();
void i2s_tx_release();
size_t i2s_tx_write(void* buffer, size_t bytes, size_t xTicksToWait);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _I2S_TX_H_

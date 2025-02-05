/****************************************************************************
 * drivers/modem/altmdm_dev.h
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __DRIVERS_MODEM_ALTMDM_DEV_H
#define __DRIVERS_MODEM_ALTMDM_DEV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/modem/altmdm.h>

#include "altmdm_spi.h"
#include "altmdm_sys.h"

#if defined(CONFIG_MODEM_ALTMDM)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct altmdm_dev_s
{
  FAR char                 *path;     /* Registration path */
  FAR struct spi_dev_s     *spi;
  struct altmdm_spi_dev_s  spidev;
  struct altmdm_sys_lock_s lock;
  int                      poweron;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: altmdm_spi_init
 *
 * Description:
 *   Initialize ALTMDM driver.
 *
 ****************************************************************************/

int altmdm_spi_init(FAR struct altmdm_dev_s *priv);

/****************************************************************************
 * Name: altmdm_spi_uninit
 *
 * Description:
 *   Uninitialize ALTMDM driver.
 *
 ****************************************************************************/

int altmdm_spi_uninit(FAR struct altmdm_dev_s *priv);

/****************************************************************************
 * Name: altmdm_spi_enable
 *
 * Description:
 *   Enable ALTMDM SPI driver.
 *
 ****************************************************************************/

int altmdm_spi_enable(FAR struct altmdm_dev_s *priv);

/****************************************************************************
 * Name: altmdm_spi_disable
 *
 * Description:
 *   Disable ALTMDM SPI driver.
 *
 ****************************************************************************/

int altmdm_spi_disable(FAR struct altmdm_dev_s *priv);

/****************************************************************************
 * Name: altmdm_spi_read
 *
 * Description:
 *   ALTMDM SPI driver read method.
 *
 ****************************************************************************/

ssize_t altmdm_spi_read(FAR struct altmdm_dev_s *priv,
                        FAR const char *buffer, size_t readlen);

/****************************************************************************
 * Name: altmdm_spi_write
 *
 * Description:
 *   ALTMDM SPI driver write method.
 *
 ****************************************************************************/

ssize_t altmdm_spi_write(FAR struct altmdm_dev_s *priv,
                         FAR const char *buffer, size_t witelen);

/****************************************************************************
 * Name: altmdm_spi_readabort
 *
 * Description:
 *   Abort the read process.
 *
 ****************************************************************************/

int altmdm_spi_readabort(FAR struct altmdm_dev_s *priv);


/****************************************************************************
 * Name: altmdm_spi_sleepmodem
 *
 * Description:
 *   Make ALTMDM sleep.
 *
 ****************************************************************************/

int altmdm_spi_sleepmodem(FAR struct altmdm_dev_s *priv);

#ifdef CONFIG_MODEM_ALTMDM_PROTCOL_V2_1

/****************************************************************************
 * Name: altmdm_spi_setreceiverready
 *
 * Description:
 *   Set receiver ready notification.
 *
 ****************************************************************************/

int altmdm_spi_setreceiverready(FAR struct altmdm_dev_s *priv);

/****************************************************************************
 * Name: altmdm_spi_isreceiverready
 *
 * Description:
 *   Check already notified or not by altmdm_spi_setreceiverready.
 *
 ****************************************************************************/

int altmdm_spi_isreceiverready(FAR struct altmdm_dev_s *priv);

/****************************************************************************
 * Name: altmdm_spi_clearreceiverready
 *
 * Description:
 *   Clear receiver ready notification.
 *
 ****************************************************************************/

int altmdm_spi_clearreceiverready(FAR struct altmdm_dev_s *priv);

#endif

/****************************************************************************
 * Name: altmdm_spi_gpioreadyisr
 *
 * Description:
 *   Interrupt handler for SLAVE_REQUEST GPIO line.
 *
 ****************************************************************************/

int altmdm_spi_gpioreadyisr(int irq, FAR void *context, FAR void *arg);

#endif
#endif /* __DRIVERS_MODEM_ALTMDM_DEV_H */

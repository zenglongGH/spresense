/****************************************************************************
 * modules/lte/altcom/api/lte/lte_getpinset.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmd_getpinset.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"
#include "altcom_callbacks.h"
#include "altcombs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define REQ_DATA_LEN (0)
#define RES_DATA_LEN (sizeof(struct apicmd_cmddat_getpinsetres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getpinset_status_chg_cb
 *
 * Description:
 *   Notification status change in processing get PIN set.
 *
 * Input Parameters:
 *  new_stat    Current status.
 *  old_stat    Preview status.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static int32_t getpinset_status_chg_cb(int32_t new_stat, int32_t old_stat)
{
  if (new_stat < ALTCOM_STATUS_POWER_ON)
    {
      DBGIF_LOG2_INFO("getpinset_status_chg_cb(%d -> %d)\n",
        old_stat, new_stat);
      altcomcallbacks_unreg_cb(APICMDID_GET_PINSET);

      return ALTCOM_STATUS_REG_CLR;
    }

  return ALTCOM_STATUS_REG_KEEP;
}

/****************************************************************************
 * Name: getpinset_parse_response
 *
 * Description:
 *   Parse PIN settings from response buffer.
 *
 * Input Parameters:
 *  resp    Pointer to response buffer.
 *  pinset  Pointer to store PIN settings.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void getpinset_parse_response(
  FAR struct apicmd_cmddat_getpinsetres_s *resp,
  FAR lte_getpin_t *pinset)
{
  pinset->enable            = resp->active;
  pinset->status            = resp->status;
  pinset->pin_attemptsleft  = resp->pin_attemptsleft;
  pinset->puk_attemptsleft  = resp->puk_attemptsleft;
  pinset->pin2_attemptsleft = resp->pin2_attemptsleft;
  pinset->puk2_attemptsleft = resp->puk2_attemptsleft;
}

/****************************************************************************
 * Name: getpinset_job
 *
 * Description:
 *   This function is an API callback for get PIN set.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void getpinset_job(FAR void *arg)
{
  int32_t                                 ret;
  int32_t                                 result;
  FAR struct apicmd_cmddat_getpinsetres_s *data;
  lte_getpin_t                            pinset;
  get_pinset_cb_t                         callback;

  data = (FAR struct apicmd_cmddat_getpinsetres_s *)arg;

  ret = altcomcallbacks_get_unreg_cb(APICMDID_GET_PINSET,
    (void **)&callback);

  if ((ret == 0) && (callback))
    {
      result = (int32_t)data->result;
      getpinset_parse_response(data, &pinset);
      callback(result, &pinset);
    }
  else
    {
      DBGIF_LOG_ERROR("Unexpected!! callback is NULL.\n");
    }

  /* In order to reduce the number of copies of the receive buffer,
   * bring a pointer to the receive buffer to the worker thread.
   * Therefore, the receive buffer needs to be released here. */

  altcom_free_cmd((FAR uint8_t *)arg);

  /* Unregistration status change callback. */

  altcomstatus_unreg_statchgcb(getpinset_status_chg_cb);
}

/****************************************************************************
 * Name: lte_getpinset_impl
 *
 * Description:
 *   Get Personal Identification Number settings.
 *
 * Input Parameters:
 *   pinset    PIN settings information.
 *   callback  Callback function to notify when getting the PIN setting is
 *             completed.
 *             If the callback is NULL, operates with synchronous API,
 *             otherwise operates with asynchronous API.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

static int32_t lte_getpinset_impl(lte_getpin_t *pinset,
                                  get_pinset_cb_t callback)
{
  int32_t                              ret;
  FAR uint8_t                         *reqbuff    = NULL;
  FAR uint8_t                         *presbuff   = NULL;
  struct apicmd_cmddat_getpinsetres_s  resbuff;
  uint16_t                             resbufflen = RES_DATA_LEN;
  uint16_t                             reslen     = 0;
  int                                  sync       = (callback == NULL);

  /* Check input parameter */

  if (!pinset && !callback)
    {
      DBGIF_LOG_ERROR("Input argument is NULL.\n");
      return -EINVAL;
    }

  /* Check LTE library status */

  ret = altcombs_check_poweron_status();
  if (0 > ret)
    {
      return ret;
    }

  if (sync)
    {
      presbuff = (FAR uint8_t *)&resbuff;
    }
  else
    {
      /* Setup API callback */

      ret = altcombs_setup_apicallback(APICMDID_GET_PINSET, callback,
                                       getpinset_status_chg_cb);
      if (0 > ret)
        {
          return ret;
        }
    }

  /* Allocate API command buffer to send */

  reqbuff = (FAR uint8_t *)apicmdgw_cmd_allocbuff(APICMDID_GET_PINSET,
                                                  REQ_DATA_LEN);
  if (!reqbuff)
    {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Send API command to modem */

  ret = apicmdgw_send(reqbuff, presbuff,
                      resbufflen, &reslen, SYS_TIMEO_FEVR);
  altcom_free_cmd(reqbuff);

  if (0 > ret)
    {
      goto errout;
    }

  ret = 0;

  if (sync)
    {
      ret = (LTE_RESULT_OK == resbuff.result) ? 0 : -EPROTO;
      if (0 == ret)
        {
          /* Parse PIN settings */

          getpinset_parse_response(&resbuff, pinset);
        }
    }

  return ret;

errout:
  if (!sync)
    {
      altcombs_teardown_apicallback(APICMDID_GET_PINSET,
                                    getpinset_status_chg_cb);
    }
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_get_pinset_sync
 *
 * Description:
 *   Get Personal Identification Number settings.
 *
 * Input Parameters:
 *   pinset    PIN settings information.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_get_pinset_sync(lte_getpin_t *pinset)
{
  return lte_getpinset_impl(pinset, NULL);
}

/****************************************************************************
 * Name: lte_get_pinset
 *
 * Description:
 *   Get Personal Identification Number settings.
 *
 * Input Parameters:
 *   callback  Callback function to notify when getting the PIN setting is
 *             completed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_get_pinset(get_pinset_cb_t callback)
{
  return lte_getpinset_impl(NULL, callback);
}

/****************************************************************************
 * Name: apicmdhdlr_getpinset
 *
 * Description:
 *   This function is an API command handler for get PIN set result.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_GET_PINSET_RES,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_getpinset(FAR uint8_t *evt, uint32_t evlen)
{
  return apicmdhdlrbs_do_runjob(evt,
    APICMDID_CONVERT_RES(APICMDID_GET_PINSET), getpinset_job);
}

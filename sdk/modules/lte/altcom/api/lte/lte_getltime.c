/****************************************************************************
 * modules/lte/altcom/api/lte/lte_getltime.c
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
#include "apicmd_ltime.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"
#include "altcom_callbacks.h"
#include "altcombs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define REQ_DATA_LEN (0)
#define RES_DATA_LEN (sizeof(struct apicmd_cmddat_getltimeres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getltime_status_chg_cb
 *
 * Description:
 *   Notification status change in processing get local time.
 *
 * Input Parameters:
 *  new_stat    Current status.
 *  old_stat    Preview status.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static int32_t getltime_status_chg_cb(int32_t new_stat, int32_t old_stat)
{
  if (new_stat < ALTCOM_STATUS_POWER_ON)
    {
      DBGIF_LOG2_INFO("getltime_status_chg_cb(%d -> %d)\n",
        old_stat, new_stat);
      altcomcallbacks_unreg_cb(APICMDID_GET_LTIME);

      return ALTCOM_STATUS_REG_CLR;
    }

  return ALTCOM_STATUS_REG_KEEP;
}

/****************************************************************************
 * Name: getltime_parse_response
 *
 * Description:
 *   Parse local time from response buffer.
 *
 * Input Parameters:
 *  resp       Pointer to response buffer.
 *  localtime  Pointer to store local time.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void getltime_parse_response(
  FAR struct apicmd_cmddat_getltimeres_s *resp,
  FAR lte_localtime_t *localtime)
{
  localtime->year   = resp->ltime.year;
  localtime->mon    = resp->ltime.month;
  localtime->mday   = resp->ltime.day;
  localtime->hour   = resp->ltime.hour;
  localtime->min    = resp->ltime.minutes;
  localtime->sec    = resp->ltime.seconds;
  localtime->tz_sec = ntohl(resp->ltime.timezone);
}

/****************************************************************************
 * Name: getltime_job
 *
 * Description:
 *   This function is an API callback for get local time.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void getltime_job(FAR void *arg)
{
  int32_t                                ret;
  int32_t                                result;
  FAR struct apicmd_cmddat_getltimeres_s *data;
  lte_localtime_t                        ltime;
  get_localtime_cb_t                     callback;

  data = (FAR struct apicmd_cmddat_getltimeres_s *)arg;

  ret = altcomcallbacks_get_unreg_cb(APICMDID_GET_LTIME,
    (void **)&callback);

  if ((ret == 0) && (callback))
    {
      result = (int32_t)data->result;
      getltime_parse_response(data, &ltime);
      callback(result, &ltime);
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

  altcomstatus_unreg_statchgcb(getltime_status_chg_cb);
}

/****************************************************************************
 * Name: lte_getlocaltime_impl
 *
 * Description:
 *   Get local time.
 *
 * Input Parameters:
 *   localtime  Local time.
 *   callback   Callback function to notify when getting local time is
 *              completed.
 *              If the callback is NULL, operates with synchronous API,
 *              otherwise operates with asynchronous API.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

static int32_t lte_getlocaltime_impl(lte_localtime_t *localtime,
                                     get_localtime_cb_t callback)
{
  int32_t                             ret;
  FAR uint8_t                        *reqbuff    = NULL;
  FAR uint8_t                        *presbuff   = NULL;
  struct apicmd_cmddat_getltimeres_s  resbuff;
  uint16_t                            resbufflen = RES_DATA_LEN;
  uint16_t                            reslen     = 0;
  int                                 sync       = (callback == NULL);

  /* Check input parameter */

  if (!localtime && !callback)
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

      ret = altcombs_setup_apicallback(APICMDID_GET_LTIME, callback,
                                       getltime_status_chg_cb);
      if (0 > ret)
        {
          return ret;
        }
    }

  /* Allocate API command buffer to send */

  reqbuff = (FAR uint8_t *)apicmdgw_cmd_allocbuff(APICMDID_GET_LTIME,
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
          /* Parse local time */

          getltime_parse_response(&resbuff, localtime);
        }
    }

  return ret;

errout:
  if (!sync)
    {
      altcombs_teardown_apicallback(APICMDID_GET_LTIME,
                                    getltime_status_chg_cb);
    }
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_get_localtime_sync
 *
 * Description:
 *   Get local time.
 *
 * Input Parameters:
 *   localtime  Local time.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_get_localtime_sync(lte_localtime_t *localtime)
{
  return lte_getlocaltime_impl(localtime, NULL);
}

/****************************************************************************
 * Name: lte_get_localtime
 *
 * Description:
 *   Get local time.
 *
 * Input Parameters:
 *   callback   Callback function to notify when getting local time is
 *              completed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_get_localtime(get_localtime_cb_t callback)
{
  return lte_getlocaltime_impl(NULL, callback);
}

/****************************************************************************
 * Name: apicmdhdlr_getltime
 *
 * Description:
 *   This function is an API command handler for get local time result.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_GET_LTIME_RES,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_getltime(FAR uint8_t *evt, uint32_t evlen)
{
  return apicmdhdlrbs_do_runjob(evt, APICMDID_CONVERT_RES(APICMDID_GET_LTIME),
    getltime_job);
}

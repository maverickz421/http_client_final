/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos.h"
#include "mgos_pppos.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

//****SPI2 Pin Defined Here****
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5
//****************************

//start my code-----------------------HTTP--------------------------------------//
static const char *url = "http://api.ipify.org";
static int exit_flag = 0;
/*
struct mg_str upload_fname(struct mg_connection *nc, struct mg_str fname)
{
  // Just return the same filename. Do not actually do this except in test!
  // fname is user-controlled and needs to be sanitized.

  fname.p = "/sdcard/addr.png";
  fname.len = 17;

  return fname;
}
*/
static void ev_handler(struct mg_connection *c, int ev, void *p, void *q)
{
  // LOG(LL_INFO, ("Inside ev_handler : %d", ev));

  // char buf[ev] ;
  // mg_send(c, buf, sizeof(buf));

  if (ev == MG_EV_HTTP_REPLY)
  {
    LOG(LL_INFO, ("This is MG_EV_HTTP_REPLY"));
    struct http_message *hm = (struct http_message *)p;
    c->flags |= MG_F_CLOSE_IMMEDIATELY;
    fwrite(hm->message.p, 1, (int)hm->message.len, stdout);
    putchar('\n');
    // LOG(LL_INFO, ("The message is : %s", hm->message.p));
    // mg_file_upload_handler(c, MG_EV_HTTP_PART_BEGIN, p, upload_fname, q);
    exit_flag = 1;
  }
  else if (ev == MG_EV_CLOSE)
  {
    LOG(LL_INFO, ("This is MG_EV_CLOSE"));
    exit_flag = 1;
  };
  (void)q;
}

//end my code-------------------------HTTP------------------------------------//

static void timer_cb(void *arg)
{
  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, NULL);

  /* --- normal http -->`*/

  /* format should be like this only -->`*/
  /* (struct mg_mgr, event_handler, void *user_data, const char *url, *extra_headers, *post_data) */
  nc = mg_connect_http(&mgr, ev_handler, NULL, url, NULL, NULL);
  LOG(LL_INFO, ("CONNECTING TO HTTP REMOTE SERVER"));

  while (exit_flag == 0)
  {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);
}

enum mgos_app_init_result mgos_app_init(void)
{
  mgos_set_timer(10000 /* ms */, MGOS_TIMER_REPEAT, timer_cb, NULL);

  //-----------SD-CARD----------------------//
  LOG(LL_INFO, ("%s", "SDMMC initialize !!!!!"));
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = PIN_NUM_MISO;
  slot_config.gpio_mosi = PIN_NUM_MOSI;
  slot_config.gpio_sck = PIN_NUM_CLK;
  slot_config.gpio_cs = PIN_NUM_CS;
  esp_vfs_fat_sdmmc_mount_config_t mount_config =
      {
          .format_if_mount_failed = false,
          .max_files = 5,
          .allocation_unit_size = 16 * 1024};

  sdmmc_card_t *card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      LOG(LL_INFO, ("%s", "Failed to mount filesystem.  If you want the card to be formatted, set format_if_mount_failed = true."));
    }
    else
    {
      LOG(LL_INFO, ("Failed to initialize the card (%s). ", esp_err_to_name(ret)));
    }
    return false;
  }

  sdmmc_card_print_info(stdout, card);
  //-----------SD-CARD----------------------//

  //start my code---------------------------HTTP----------------------------------//

  mgos_set_timer(3000, true, timer_cb, NULL);
  /*
  struct mg_connection *mg_connect_http( struct mg_mgr *mgr, MG_CB(mg_event_handler_t event_handler, void *user_data),
    const char *url, const char *extra_headers, const char *post_data);
  */

  /* --- http options --> SSL TLS and all --> */

  /* mg_connect_http_opt(&mgr, ev_handler, NULL, opts, url, NULL, NULL);  */
  /*
  struct mg_connection *mg_connect_http_opt(struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *url, const char *extra_headers, const char *post_data);
  */

  // mg_file_upload_handler(nc, MG_EV_HTTP_PART_BEGIN, p, upload_fname, q);

  //end my code-----------------------------HTTP--------------------------------//

  return MGOS_APP_INIT_SUCCESS;
}

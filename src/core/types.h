#pragma once

#include <stdint.h>

enum ShowMode {
  SHOW_MODE_NONE = -1,
  SHOW_MODE_VIDEO = 0,
  SHOW_MODE_WAVES,
  SHOW_MODE_RDFT,
  SHOW_MODE_NB
};

enum AvSyncType {
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER,
  AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
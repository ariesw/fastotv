/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include "client/utils.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <player/media/types.h>

namespace fastotv {
namespace client {
namespace {
class CallbackHolder {
 public:
  CallbackHolder(quit_callback_t cb) : is_quit(cb) {}
  static int download_interrupt_callback(void* user_data) {
    CallbackHolder* holder = static_cast<CallbackHolder*>(user_data);
    if (holder->is_quit()) {
      delete holder;
      return 1;
    }

    return 0;
  }

 private:
  const quit_callback_t is_quit;
};
}  // namespace

bool DownloadFileToBuffer(const common::uri::Url& uri, common::buffer_t* buff, quit_callback_t cb) {
  if (!uri.IsValid() || !buff || !cb) {
    return false;
  }

  const std::string url_str = fastoplayer::media::make_url(uri);
  if (url_str.empty()) {
    return false;
  }

  AVFormatContext* ic = avformat_alloc_context();
  if (!ic) {
    return false;
  }

  const char* in_filename = url_str.c_str();
  CallbackHolder* holder = new CallbackHolder(cb);
  ic->interrupt_callback.callback = CallbackHolder::download_interrupt_callback;
  ic->interrupt_callback.opaque = holder;
  int open_result = avformat_open_input(&ic, in_filename, NULL, NULL);
  if (open_result < 0) {
    avformat_free_context(ic);
    ic = NULL;
    return false;
  }
  AVPacket pkt;
  int ret = av_read_frame(ic, &pkt);
  if (ret < 0) {
    avformat_free_context(ic);
    ic = NULL;
    return false;
  }

  *buff = common::buffer_t(pkt.data, pkt.data + pkt.size);
  av_packet_unref(&pkt);
  avformat_free_context(ic);
  ic = NULL;
  return true;
}

}  // namespace client
}  // namespace fastotv

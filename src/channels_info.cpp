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

#include "channels_info.h"

#include <common/sprintf.h>

namespace fastotv {

ChannelsInfo::ChannelsInfo() : channels_() {}

void ChannelsInfo::AddChannel(const ChannelInfo& channel) {
  channels_.push_back(channel);
}

ChannelsInfo::channels_t ChannelsInfo::GetChannels() const {
  return channels_;
}

size_t ChannelsInfo::GetSize() const {
  return channels_.size();
}

bool ChannelsInfo::IsEmpty() const {
  return channels_.empty();
}

bool ChannelsInfo::Equals(const ChannelsInfo& chan) const {
  return channels_ == chan.channels_;
}

common::Error ChannelsInfo::DoSerialize(serialize_type* deserialized) const {
  json_object* jchannels = json_object_new_array();
  for (ChannelInfo url : channels_) {
    json_object* jurl = NULL;
    common::Error err = url.Serialize(&jurl);
    if (err) {
      continue;
    }
    json_object_array_add(jchannels, jurl);
  }

  *deserialized = jchannels;
  return common::Error();
}

common::Error ChannelsInfo::DeSerialize(const serialize_type& serialized, ChannelsInfo* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  channels_t chan;
  size_t len = json_object_array_length(serialized);
  for (size_t i = 0; i < len; ++i) {
    json_object* jurl = json_object_array_get_idx(serialized, i);
    ChannelInfo url;
    common::Error err = ChannelInfo::DeSerialize(jurl, &url);
    if (err) {
      continue;
    }
    chan.push_back(url);
  }

  (*obj).channels_ = chan;
  return common::Error();
}

}  // namespace fastotv

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

#pragma once

#include "client_server_types.h"

#include "chat_message.h"

#include "serializer/json_serializer.h"

namespace fastotv {

class RuntimeChannelInfo : public JsonSerializerEx {
 public:
  typedef std::vector<ChatMessage> messages_t;
  RuntimeChannelInfo();
  RuntimeChannelInfo(stream_id channel_id,
                     size_t watchers,
                     ChannelType type,
                     bool chat_enabled,
                     bool read_only,
                     const messages_t& msgs = messages_t());
  ~RuntimeChannelInfo();

  bool IsValid() const;

  static common::Error DeSerialize(const serialize_type& serialized, RuntimeChannelInfo* obj) WARN_UNUSED_RESULT;

  void SetChannelId(stream_id sid);
  stream_id GetChannelId() const;

  void SetWatchersCount(size_t count);
  size_t GetWatchersCount() const;

  void SetChatEnabled(bool en);
  bool IsChatEnabled() const;

  void SetChatReadOnly(bool ro);
  bool IsChatReadOnly() const;

  void AddMessage(const ChatMessage& msg);
  messages_t GetMessages() const;

  void SetChannelType(ChannelType ct);
  ChannelType GetChannelType() const;

  bool Equals(const RuntimeChannelInfo& inf) const;

 protected:
  virtual common::Error SerializeFields(json_object* obj) const override;

 private:
  stream_id channel_id_;
  size_t watchers_;
  ChannelType type_;
  bool chat_enabled_;
  bool chat_read_only_;
  messages_t messages_;
};

inline bool operator==(const RuntimeChannelInfo& left, const RuntimeChannelInfo& right) {
  return left.Equals(right);
}

}  // namespace fastotv

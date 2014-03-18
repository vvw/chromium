// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_INTERNAL_API_PUBLIC_EVENTS_PROTOCOL_EVENT_H
#define SYNC_INTERNAL_API_PUBLIC_EVENTS_PROTOCOL_EVENT_H

#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "sync/base/sync_export.h"

namespace syncer {

// SyncNetworkEvents represent a single client <-> server sync protocol event
// that recently took place. Sync protocol events occur when the client decides
// to send a sync protocol request (such as GetUpdates or Commit) to the server,
// and when the server responds. Note that the requests and responses themselves
// are modelled by {GetUpdates, Commit}x{Request,Response} objects.
//
// These objects are intended to be used for displaying information on
// about:sync.  They should be considered to be immutable and opaque.  No
// program behavior should depend on their contents.
//
// Each type of request can maintain its own set of additional metadata and have
// its own custom serialization routines.  For example, the "configure"
// GetUpdates request will include information about its "origin" in its debug
// info.
class SYNC_EXPORT ProtocolEvent {
 public:
  ProtocolEvent();
  virtual ~ProtocolEvent();

  // Returns the time when the request was sent or received.
  virtual base::Time GetTimestamp() const = 0;

  // Returns a string representing they type of the request.  Should be short.
  virtual std::string GetType() const = 0;

  // Returns a string representing details of the request.  May be verbose.  The
  // implementer is allowed to return lots of data separated by newlines.
  virtual std::string GetDetails() const = 0;

  // Returns a DictionaryValue representing the protobuf message associated with
  // this event.
  virtual scoped_ptr<base::DictionaryValue> GetProtoMessage() const = 0;

  // Need a virtual copy contructor to copy this object across threads.
  virtual scoped_ptr<ProtocolEvent> Clone() const = 0;

  // A static function that assembles the data exposed through the
  // ProtocolEvent's interface into a single DictionaryValue.
  static scoped_ptr<base::DictionaryValue> ToValue(
      const ProtocolEvent& event);
};

}  // namespace syncer

#endif  // SYNC_INTERNAL_API_PUBLIC_EVENTS_PROTOCOL_EVENT_H

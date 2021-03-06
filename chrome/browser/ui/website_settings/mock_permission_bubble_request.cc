// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/website_settings/mock_permission_bubble_request.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"

MockPermissionBubbleRequest::MockPermissionBubbleRequest()
    : granted_(false), cancelled_(false), finished_(false) {
  text_ = base::ASCIIToUTF16("test");
  accept_label_ = base::ASCIIToUTF16("button");
  deny_label_ = base::ASCIIToUTF16("button");
}

MockPermissionBubbleRequest::MockPermissionBubbleRequest(
    const std::string& text)
    : granted_(false), cancelled_(false), finished_(false) {
  text_ = base::UTF8ToUTF16(text);
  accept_label_ = base::ASCIIToUTF16("button");
  deny_label_ = base::ASCIIToUTF16("button");
}

MockPermissionBubbleRequest::MockPermissionBubbleRequest(
    const std::string& text, const std::string& accept_label,
    const std::string& deny_label)
    : granted_(false), cancelled_(false), finished_(false) {
  text_ = base::UTF8ToUTF16(text);
  accept_label_ = base::UTF8ToUTF16(accept_label);
  deny_label_ = base::UTF8ToUTF16(deny_label);
}

MockPermissionBubbleRequest::~MockPermissionBubbleRequest() {}

int MockPermissionBubbleRequest::GetIconID() const {
  return 0;
}

base::string16 MockPermissionBubbleRequest::GetMessageText() const {
  return text_;
}

base::string16 MockPermissionBubbleRequest::GetMessageTextFragment() const {
  return text_;
}

bool MockPermissionBubbleRequest::HasUserGesture() const {
  return false;
}

GURL MockPermissionBubbleRequest::GetRequestingHostname() const {
  return GURL("http://www.google.com");
}

void MockPermissionBubbleRequest::PermissionGranted() {
  granted_ = true;
}

void MockPermissionBubbleRequest::PermissionDenied() {
  granted_ = false;
}

void MockPermissionBubbleRequest::Cancelled() {
  granted_ = false;
  cancelled_ = true;
}

void MockPermissionBubbleRequest::RequestFinished() {
  finished_ = true;
}

bool MockPermissionBubbleRequest::granted() {
  return granted_;
}

bool MockPermissionBubbleRequest::cancelled() {
  return cancelled_;
}

bool MockPermissionBubbleRequest::finished() {
  return finished_;
}

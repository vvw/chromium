// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/media/media_stream.h"
#include "content/renderer/media/media_stream_impl.h"
#include "content/renderer/media/mock_media_stream_dependency_factory.h"
#include "content/renderer/media/mock_media_stream_dispatcher.h"
#include "content/renderer/media/mock_media_stream_video_source.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebMediaStream.h"
#include "third_party/WebKit/public/platform/WebMediaStreamSource.h"
#include "third_party/WebKit/public/platform/WebMediaStreamTrack.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebVector.h"

namespace content {

class MockMediaStreamVideoCapturerSource : public MockMediaStreamVideoSource {
 public:
  MockMediaStreamVideoCapturerSource(
      const StreamDeviceInfo& device,
      const SourceStoppedCallback& stop_callback,
      MediaStreamDependencyFactory* factory)
  : MockMediaStreamVideoSource(factory, false) {
    SetDeviceInfo(device);
    SetStopCallback(stop_callback);
  }
};

class MediaStreamImplUnderTest : public MediaStreamImpl {
 public:
  enum RequestState {
    REQUEST_NOT_STARTED,
    REQUEST_NOT_COMPLETE,
    REQUEST_SUCCEEDED,
    REQUEST_FAILED,
  };

  MediaStreamImplUnderTest(MediaStreamDispatcher* media_stream_dispatcher,
                           MediaStreamDependencyFactory* dependency_factory)
      : MediaStreamImpl(NULL, media_stream_dispatcher, dependency_factory),
        state_(REQUEST_NOT_STARTED),
        factory_(dependency_factory),
        video_source_(NULL) {
  }

  void RequestUserMedia() {
    blink::WebUserMediaRequest user_media_request;
    state_ = REQUEST_NOT_COMPLETE;
    requestUserMedia(user_media_request);
  }

  virtual void CompleteGetUserMediaRequest(
      const blink::WebMediaStream& stream,
      blink::WebUserMediaRequest* request_info,
      content::MediaStreamRequestResult result) OVERRIDE {
    last_generated_stream_ = stream;
    result_ = result;
    state_ = (result == MEDIA_DEVICE_OK ? REQUEST_SUCCEEDED : REQUEST_FAILED);
  }

  virtual blink::WebMediaStream GetMediaStream(
      const GURL& url) OVERRIDE {
    return last_generated_stream_;
  }

  virtual MediaStreamVideoSource* CreateVideoSource(
      const StreamDeviceInfo& device,
      const MediaStreamSource::SourceStoppedCallback& stop_callback) OVERRIDE {
    video_source_ = new MockMediaStreamVideoCapturerSource(device,
                                                           stop_callback,
                                                           factory_);
    return video_source_;
  }

  using MediaStreamImpl::OnLocalMediaStreamStop;
  using MediaStreamImpl::OnLocalSourceStopped;

  const blink::WebMediaStream& last_generated_stream() {
    return last_generated_stream_;
  }

  MockMediaStreamVideoCapturerSource* last_created_video_source() const {
    return video_source_;
  }

  RequestState request_state() const { return state_; }
  content::MediaStreamRequestResult error_reason() const { return result_; }

 private:
  blink::WebMediaStream last_generated_stream_;
  RequestState state_;
  content::MediaStreamRequestResult result_;
  MediaStreamDependencyFactory* factory_;
  MockMediaStreamVideoCapturerSource* video_source_;
};

class MediaStreamImplTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    // Create our test object.
    ms_dispatcher_.reset(new MockMediaStreamDispatcher());
    dependency_factory_.reset(new MockMediaStreamDependencyFactory());
    ms_impl_.reset(new MediaStreamImplUnderTest(ms_dispatcher_.get(),
                                                dependency_factory_.get()));
  }

  blink::WebMediaStream RequestLocalMediaStream() {
    ms_impl_->RequestUserMedia();
    FakeMediaStreamDispatcherComplete();
    StartMockedVideoSource();

    EXPECT_EQ(MediaStreamImplUnderTest::REQUEST_SUCCEEDED,
              ms_impl_->request_state());

    blink::WebMediaStream desc = ms_impl_->last_generated_stream();
    content::MediaStream* native_stream =
        content::MediaStream::GetMediaStream(desc);
    if (!native_stream) {
      ADD_FAILURE();
      return desc;
    }

    blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
    desc.audioTracks(audio_tracks);
    blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
    desc.videoTracks(video_tracks);

    EXPECT_EQ(1u, audio_tracks.size());
    EXPECT_EQ(1u, video_tracks.size());
    EXPECT_NE(audio_tracks[0].id(), video_tracks[0].id());
    return desc;
  }

  void FakeMediaStreamDispatcherComplete() {
    ms_impl_->OnStreamGenerated(ms_dispatcher_->request_id(),
                                ms_dispatcher_->stream_label(),
                                ms_dispatcher_->audio_array(),
                                ms_dispatcher_->video_array());
  }

  void StartMockedVideoSource() {
    MockMediaStreamVideoCapturerSource* video_source =
        ms_impl_->last_created_video_source();
    if (video_source->SourceHasAttemptedToStart())
      video_source->StartMockedSource();
  }

  void FailToStartMockedVideoSource() {
    MockMediaStreamVideoCapturerSource* video_source =
        ms_impl_->last_created_video_source();
    if (video_source->SourceHasAttemptedToStart())
      video_source->FailToStartMockedSource();
  }

 protected:
  scoped_ptr<MockMediaStreamDispatcher> ms_dispatcher_;
  scoped_ptr<MediaStreamImplUnderTest> ms_impl_;
  scoped_ptr<MockMediaStreamDependencyFactory> dependency_factory_;
};

TEST_F(MediaStreamImplTest, GenerateMediaStream) {
  // Generate a stream with both audio and video.
  blink::WebMediaStream mixed_desc = RequestLocalMediaStream();
}

// Test that the same source object is used if two MediaStreams are generated
// using the same source.
TEST_F(MediaStreamImplTest, GenerateTwoMediaStreamsWithSameSource) {
  blink::WebMediaStream desc1 = RequestLocalMediaStream();
  blink::WebMediaStream desc2 = RequestLocalMediaStream();

  blink::WebVector<blink::WebMediaStreamTrack> desc1_video_tracks;
  desc1.videoTracks(desc1_video_tracks);
  blink::WebVector<blink::WebMediaStreamTrack> desc2_video_tracks;
  desc2.videoTracks(desc2_video_tracks);
  EXPECT_EQ(desc1_video_tracks[0].source().id(),
            desc2_video_tracks[0].source().id());

  EXPECT_EQ(desc1_video_tracks[0].source().extraData(),
            desc2_video_tracks[0].source().extraData());

  blink::WebVector<blink::WebMediaStreamTrack> desc1_audio_tracks;
  desc1.audioTracks(desc1_audio_tracks);
  blink::WebVector<blink::WebMediaStreamTrack> desc2_audio_tracks;
  desc2.audioTracks(desc2_audio_tracks);
  EXPECT_EQ(desc1_audio_tracks[0].source().id(),
            desc2_audio_tracks[0].source().id());

  EXPECT_EQ(desc1_audio_tracks[0].source().extraData(),
            desc2_audio_tracks[0].source().extraData());
}

// Test that the same source object is not used if two MediaStreams are
// generated using different sources.
TEST_F(MediaStreamImplTest, GenerateTwoMediaStreamsWithDifferentSources) {
  blink::WebMediaStream desc1 = RequestLocalMediaStream();
  // Make sure another device is selected (another |session_id|) in  the next
  // gUM request.
  ms_dispatcher_->IncrementSessionId();
  blink::WebMediaStream desc2 = RequestLocalMediaStream();

  blink::WebVector<blink::WebMediaStreamTrack> desc1_video_tracks;
  desc1.videoTracks(desc1_video_tracks);
  blink::WebVector<blink::WebMediaStreamTrack> desc2_video_tracks;
  desc2.videoTracks(desc2_video_tracks);
  EXPECT_NE(desc1_video_tracks[0].source().id(),
            desc2_video_tracks[0].source().id());

  EXPECT_NE(desc1_video_tracks[0].source().extraData(),
            desc2_video_tracks[0].source().extraData());

  blink::WebVector<blink::WebMediaStreamTrack> desc1_audio_tracks;
  desc1.audioTracks(desc1_audio_tracks);
  blink::WebVector<blink::WebMediaStreamTrack> desc2_audio_tracks;
  desc2.audioTracks(desc2_audio_tracks);
  EXPECT_NE(desc1_audio_tracks[0].source().id(),
            desc2_audio_tracks[0].source().id());

  EXPECT_NE(desc1_audio_tracks[0].source().extraData(),
            desc2_audio_tracks[0].source().extraData());
}

TEST_F(MediaStreamImplTest, StopLocalMediaStream) {
  // Generate a stream with both audio and video.
  blink::WebMediaStream mixed_desc = RequestLocalMediaStream();

  // Stop generated local streams.
  ms_impl_->OnLocalMediaStreamStop(mixed_desc.id().utf8());
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
}

// This test that a source is not stopped even if the MediaStream is stopped if
// there are two MediaStreams using the same device. The source is stopped
// if there are no more MediaStreams using the device.
TEST_F(MediaStreamImplTest, StopLocalMediaStreamWhenTwoStreamUseSameDevices) {
  // Generate a stream with both audio and video.
  blink::WebMediaStream desc1 = RequestLocalMediaStream();
  blink::WebMediaStream desc2 = RequestLocalMediaStream();

  ms_impl_->OnLocalMediaStreamStop(desc2.id().utf8());
  EXPECT_EQ(0, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(0, ms_dispatcher_->stop_video_device_counter());

  ms_impl_->OnLocalMediaStreamStop(desc1.id().utf8());
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
}

// Test that the source is stopped even if there are two MediaStreams using
// the same source.
TEST_F(MediaStreamImplTest, StopSource) {
  // Generate a stream with both audio and video.
  blink::WebMediaStream desc1 = RequestLocalMediaStream();
  blink::WebMediaStream desc2 = RequestLocalMediaStream();

  // Stop the video source.
  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  desc1.videoTracks(video_tracks);
  ms_impl_->OnLocalSourceStopped(video_tracks[0].source());
  EXPECT_EQ(0, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());

  // Stop the audio source.
  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
  desc1.audioTracks(audio_tracks);
  ms_impl_->OnLocalSourceStopped(audio_tracks[0].source());
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
}

// Test that the MediaStreams are deleted if the owning WebFrame is deleted.
// In the unit test the owning frame is NULL.
TEST_F(MediaStreamImplTest, FrameWillClose) {
  // Test a stream with both audio and video.
  blink::WebMediaStream mixed_desc = RequestLocalMediaStream();

  // Test that the MediaStreams are deleted if the owning WebFrame is deleted.
  // In the unit test the owning frame is NULL.
  ms_impl_->FrameWillClose(NULL);
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
}

// This test what happens if a source to a MediaSteam fails to start.
TEST_F(MediaStreamImplTest, MediaSourceFailToStart) {
  ms_impl_->RequestUserMedia();
  FakeMediaStreamDispatcherComplete();
  FailToStartMockedVideoSource();
  EXPECT_EQ(MediaStreamImplUnderTest::REQUEST_FAILED,
            ms_impl_->request_state());
  EXPECT_EQ(MEDIA_DEVICE_TRACK_START_FAILURE,
            ms_impl_->error_reason());
  EXPECT_EQ(1, ms_dispatcher_->request_stream_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
}

// This test what happens if MediaStreamImpl is deleted while the sources of a
// MediaStream is being started.
TEST_F(MediaStreamImplTest, MediaStreamImplShutDown) {
  ms_impl_->RequestUserMedia();
  FakeMediaStreamDispatcherComplete();
  EXPECT_EQ(1, ms_dispatcher_->request_stream_counter());
  EXPECT_EQ(MediaStreamImplUnderTest::REQUEST_NOT_COMPLETE,
            ms_impl_->request_state());

  MockMediaStreamVideoCapturerSource* video_source =
      ms_impl_->last_created_video_source();
  // Hold on to a blink reference to the source to guarantee that its not
  // deleted when MediaStreamImpl is deleted.
  blink::WebMediaStreamSource blink_source = video_source->owner();
  ms_impl_.reset();
  video_source->StartMockedSource();
}

// This test what happens if the WebFrame is closed while the MediaStream is
// being generated by the MediaStreamDispatcher.
TEST_F(MediaStreamImplTest, ReloadFrameWhileGeneratingStream) {
  ms_impl_->RequestUserMedia();
  ms_impl_->FrameWillClose(NULL);
  EXPECT_EQ(1, ms_dispatcher_->request_stream_counter());
  EXPECT_EQ(0, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(0, ms_dispatcher_->stop_video_device_counter());
  EXPECT_EQ(MediaStreamImplUnderTest::REQUEST_NOT_COMPLETE,
            ms_impl_->request_state());
}

// This test what happens if the WebFrame is closed while the sources are being
// started.
TEST_F(MediaStreamImplTest, ReloadFrameWhileGeneratingSources) {
  ms_impl_->RequestUserMedia();
  FakeMediaStreamDispatcherComplete();
  EXPECT_EQ(1, ms_dispatcher_->request_stream_counter());
  ms_impl_->FrameWillClose(NULL);
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
  EXPECT_EQ(MediaStreamImplUnderTest::REQUEST_NOT_COMPLETE,
            ms_impl_->request_state());
}

// This test what happens if stop is called on a stream after the frame has
// been reloaded.
TEST_F(MediaStreamImplTest, StopStreamAfterReload) {
  blink::WebMediaStream mixed_desc = RequestLocalMediaStream();
  EXPECT_EQ(1, ms_dispatcher_->request_stream_counter());
  ms_impl_->FrameWillClose(NULL);
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
  ms_impl_->OnLocalMediaStreamStop(mixed_desc.id().utf8());
  EXPECT_EQ(1, ms_dispatcher_->stop_audio_device_counter());
  EXPECT_EQ(1, ms_dispatcher_->stop_video_device_counter());
}

}  // namespace content

#include <thread>

#include <common/application/application.h>
#include <common/threads/types.h>  // for condition_variable, mutex

extern "C" {
#include <libavdevice/avdevice.h>  // for avdevice_register_all
}

#include "client/core/events/events.h"
#include "client/core/sdl_utils.h"
#include "client/core/video_state.h"
#include "client/core/video_state_handler.h"

using namespace fasto::fastotv;
using namespace fasto::fastotv::client;
using namespace fasto::fastotv::client::core;

namespace {
struct DictionaryOptions {
  DictionaryOptions() : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {
    av_dict_set(&sws_dict, "flags", "bicubic", 0);
  }

  ~DictionaryOptions() {
    av_dict_free(&swr_opts);
    av_dict_free(&sws_dict);
    av_dict_free(&format_opts);
    av_dict_free(&codec_opts);
  }
  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;

 private:
  DISALLOW_COPY_AND_ASSIGN(DictionaryOptions);
};
}  // namespace

class FakeHandler : public VideoStateHandler {
  virtual ~FakeHandler() {}

  // audio
  virtual common::Error HandleRequestAudio(VideoState* stream,
                                           int64_t wanted_channel_layout,
                                           int wanted_nb_channels,
                                           int wanted_sample_rate,
                                           core::AudioParams* audio_hw_params,
                                           int* audio_buff_size) override {
    UNUSED(stream);
    UNUSED(wanted_channel_layout);
    UNUSED(wanted_nb_channels);
    UNUSED(wanted_sample_rate);
    UNUSED(audio_buff_size);

    core::AudioParams laudio_hw_params;
    if (!core::init_audio_params(3, 48000, 2, &laudio_hw_params)) {
      return common::make_error_value("Failed to init audio.", common::Value::E_ERROR);
    }

    *audio_hw_params = laudio_hw_params;
    return common::Error();
  }
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) override {
    UNUSED(audio_stream_ptr);
    UNUSED(src);
    UNUSED(len);
    UNUSED(volume);
  }

  // video
  virtual common::Error HandleRequestVideo(VideoState* stream,
                                           int width,
                                           int height,
                                           int av_pixel_format,
                                           AVRational aspect_ratio) override {
    UNUSED(stream);
    UNUSED(width);
    UNUSED(height);
    UNUSED(av_pixel_format);
    UNUSED(aspect_ratio);
    return common::Error();
  }

  virtual void HandleFrameResize(VideoState* stream,
                                 int width,
                                 int height,
                                 int av_pixel_format,
                                 AVRational sar) override {
    UNUSED(stream);
    UNUSED(width);
    UNUSED(height);
    UNUSED(av_pixel_format);
    UNUSED(sar);
  }

  virtual void HandleQuitStream(VideoState* stream, int exit_code, common::Error err) override {
    core::events::QuitStreamEvent* qevent =
        new core::events::QuitStreamEvent(stream, core::events::QuitStreamInfo(stream, exit_code, err));
    fApp->PostEvent(qevent);
  }
};

class FakeApplication : public common::application::IApplication {
 public:
  FakeApplication(int argc, char** argv) : common::application::IApplication(argc, argv), stop_(false) {}

  virtual int PreExecImpl() override { /* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();
    return EXIT_SUCCESS;
  }

  virtual int ExecImpl() override {
    const stream_id id = "unique";
    // wget http://download.blender.org/peach/bigbuckbunny_movies/big_buck_bunny_1080p_h264.mov
    const common::uri::Uri uri = common::uri::Uri("file://" PROJECT_TEST_SOURCES_DIR "/big_buck_bunny_1080p_h264.mov");
    core::AppOptions opt;
    DictionaryOptions* dict = new DictionaryOptions;
    const core::ComplexOptions copt(dict->swr_opts, dict->sws_dict, dict->format_opts, dict->codec_opts);
    VideoStateHandler* handler = new FakeHandler;
    VideoState* vs = new VideoState(id, uri, opt, copt);
    vs->SetHandler(handler);
    std::thread exec([vs]() { return vs->Exec(); });

    std::thread audio([this, vs]() {
      while (!stop_) {
        uint8_t stream_buff[8192];
        vs->UpdateAudioBuffer(stream_buff, sizeof(stream_buff), 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
      }
    });

    lock_t lock(stop_mutex_);
    while (!stop_) {
      std::cv_status interrupt_status = stop_cond_.wait_for(lock, std::chrono::milliseconds(20));
      if (interrupt_status == std::cv_status::no_timeout) {  // if notify
      } else {
        vs->TryToGetVideoFrame();
      }
    }

    vs->Abort();
    audio.join();
    exec.join();
    delete vs;
    delete handler;
    delete dict;
    return EXIT_SUCCESS;
  }

  virtual int PostExecImpl() override { return EXIT_SUCCESS; }

  virtual void PostEvent(event_t* event) override {
    events::Event* fevent = static_cast<events::Event*>(event);
    if (fevent->GetEventType() == core::events::RequestVideoEvent::EventType) {
      core::events::RequestVideoEvent* avent = static_cast<core::events::RequestVideoEvent*>(event);
      core::events::FrameInfo fr = avent->info();
      common::Error err = fr.stream_->RequestVideo(fr.width, fr.height, fr.av_pixel_format, fr.aspect_ratio);
      if (err && err->IsError()) {
        fApp->Exit(EXIT_FAILURE);
      }
    } else if (fevent->GetEventType() == core::events::QuitStreamEvent::EventType) {
      // events::QuitStreamEvent* qevent = static_cast<core::events::QuitStreamEvent*>(event);
      fApp->Exit(EXIT_SUCCESS);
    } else {
      NOTREACHED();
    }
  }
  virtual void SendEvent(event_t* event) override {
    UNUSED(event);
    NOTREACHED();
  }

  virtual void Subscribe(listener_t* listener, common::events_size_t id) override {
    UNUSED(listener);
    UNUSED(id);
  }
  virtual void UnSubscribe(listener_t* listener, common::events_size_t id) override {
    UNUSED(listener);
    UNUSED(id);
  }
  virtual void UnSubscribe(listener_t* listener) override { UNUSED(listener); }

  virtual void ShowCursor() override {}
  virtual void HideCursor() override {}

  virtual void ExitImpl(int result) override {
    UNUSED(result);
    lock_t lock(stop_mutex_);
    stop_ = true;
    stop_cond_.notify_one();
  }

  virtual common::application::timer_id_t AddTimer(uint32_t interval,
                                                   common::application::timer_callback_t cb,
                                                   void* user_data) override {
    UNUSED(interval);
    UNUSED(cb);
    UNUSED(user_data);
    return 1;
  }

  virtual bool RemoveTimer(common::application::timer_id_t id) override {
    UNUSED(id);
    return true;
  }

 private:
  typedef std::unique_lock<std::mutex> lock_t;
  std::condition_variable stop_cond_;
  std::mutex stop_mutex_;
  bool stop_;
};

int main(int argc, char** argv) {
#if defined(NDEBUG)
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#else
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#endif
#if defined(LOG_TO_FILE)
  std::string log_path = common::file_system::prepare_path("~/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif
  FakeApplication app(argc, argv);
  return app.Exec();
}

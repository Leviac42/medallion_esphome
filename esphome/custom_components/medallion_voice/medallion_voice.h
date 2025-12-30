#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
#include "esphome/core/automation.h"
#include "esphome/components/es8311/es8311.h"
#include <SPI.h>
#include <SdFat.h>
#include <WiFi.h>

namespace esphome {
namespace medallion_voice {

class MedallionVoiceComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_audio_codec(es8311::ES8311Component *codec) { this->audio_codec_ = codec; }
  void set_sd_cs_pin(InternalGPIOPin *pin) { this->sd_cs_pin_ = pin; }
  void set_upload_url(const std::string &url) { this->upload_url_ = url; }

  // Recording control
  bool start_recording();
  void stop_recording();
  bool is_recording() const { return this->recording_; }

  // Upload the last recording
  bool upload_recording();

  // Get status
  const char *get_status() const { return this->status_.c_str(); }
  const char *get_current_file() const { return this->current_file_.c_str(); }

 protected:
  bool init_sd_card_();
  void update_record_path_();
  void write_wav_header_(FsFile &file, uint32_t data_length);
  bool parse_url_(const std::string &url, std::string &host, uint16_t &port, std::string &path);

  es8311::ES8311Component *audio_codec_{nullptr};
  InternalGPIOPin *sd_cs_pin_{nullptr};
  std::string upload_url_;

  // SD card
  SdFs sd_;
  SPIClass sd_spi_{HSPI};
  bool sd_mounted_{false};

  // Recording state
  bool recording_{false};
  FsFile record_file_;
  std::string current_file_;
  uint32_t recorded_bytes_{0};
  uint16_t record_counter_{1};

  // Status
  std::string status_{"Ready"};

  // Audio buffer
  static constexpr size_t AUDIO_BUFFER_SIZE = 1024;
  uint8_t audio_buffer_[AUDIO_BUFFER_SIZE];
};

// Actions
template<typename... Ts> class StartRecordingAction : public Action<Ts...>, public Parented<MedallionVoiceComponent> {
 public:
  void play(Ts... x) override { this->parent_->start_recording(); }
};

template<typename... Ts> class StopRecordingAction : public Action<Ts...>, public Parented<MedallionVoiceComponent> {
 public:
  void play(Ts... x) override { this->parent_->stop_recording(); }
};

template<typename... Ts> class UploadAction : public Action<Ts...>, public Parented<MedallionVoiceComponent> {
 public:
  void play(Ts... x) override { this->parent_->upload_recording(); }
};

}  // namespace medallion_voice
}  // namespace esphome

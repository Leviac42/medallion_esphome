#include "medallion_voice.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"

namespace esphome {
namespace medallion_voice {

static const char *const TAG = "medallion_voice";

// WAV format constants
static const uint16_t WAV_NUM_CHANNELS = 2;
static const uint16_t WAV_BITS_PER_SAMPLE = 16;
static const uint32_t WAV_SAMPLE_RATE = 16000;

void MedallionVoiceComponent::setup() {
  ESP_LOGI(TAG, "Setting up Medallion Voice Recorder...");

  // Initialize SD card
  if (!this->init_sd_card_()) {
    ESP_LOGE(TAG, "Failed to initialize SD card");
    this->status_ = "SD Failed";
    // Don't mark failed - device can still function without SD
  } else {
    this->status_ = "Ready";
  }

  ESP_LOGI(TAG, "Medallion Voice Recorder initialized");
}

void MedallionVoiceComponent::loop() {
  if (!this->recording_) return;

  // Read audio data and write to SD card
  if (this->audio_codec_ != nullptr && this->record_file_) {
    size_t bytes_read = this->audio_codec_->read_samples(this->audio_buffer_, AUDIO_BUFFER_SIZE);
    if (bytes_read > 0) {
      size_t written = this->record_file_.write(this->audio_buffer_, bytes_read);
      if (written > 0) {
        this->recorded_bytes_ += written;
      }
    }
  }
}

void MedallionVoiceComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Medallion Voice Recorder:");
  LOG_PIN("  SD CS Pin: ", this->sd_cs_pin_);
  ESP_LOGCONFIG(TAG, "  Upload URL: %s", this->upload_url_.c_str());
  ESP_LOGCONFIG(TAG, "  SD Mounted: %s", this->sd_mounted_ ? "Yes" : "No");
}

bool MedallionVoiceComponent::init_sd_card_() {
  if (this->sd_cs_pin_ == nullptr) {
    ESP_LOGE(TAG, "SD CS pin not configured");
    return false;
  }

  uint8_t cs_pin = this->sd_cs_pin_->get_pin();
  
  // Configure CS pin
  this->sd_cs_pin_->setup();
  this->sd_cs_pin_->digital_write(true);

  // Initialize SPI for SD card (HSPI)
  // Pins are configured in the main YAML, we just use the CS
  this->sd_spi_.begin();
  delay(200);

  // Try different SPI speeds
  ESP_LOGD(TAG, "Attempting SD card mount at 400kHz...");
  SdSpiConfig cfg(cs_pin, SHARED_SPI, SD_SCK_MHZ(0.4), &this->sd_spi_);
  
  if (!this->sd_.begin(cfg)) {
    ESP_LOGD(TAG, "SHARED_SPI failed, trying DEDICATED_SPI...");
    SdSpiConfig cfg2(cs_pin, DEDICATED_SPI, SD_SCK_MHZ(0.4), &this->sd_spi_);
    if (!this->sd_.begin(cfg2)) {
      ESP_LOGD(TAG, "400kHz failed, trying 250kHz...");
      SdSpiConfig cfg3(cs_pin, DEDICATED_SPI, SD_SCK_MHZ(0.25), &this->sd_spi_);
      if (!this->sd_.begin(cfg3)) {
        ESP_LOGE(TAG, "SD card mount failed at all speeds");
        return false;
      }
    }
  }

  this->sd_mounted_ = true;

  // Print SD card info
  if (this->sd_.card()) {
    uint8_t card_type = this->sd_.card()->type();
    const char *type_str = "Unknown";
    if (card_type == SD_CARD_TYPE_SD1) type_str = "SD1";
    else if (card_type == SD_CARD_TYPE_SD2) type_str = "SD2";
    else if (card_type == SD_CARD_TYPE_SDHC) type_str = "SDHC/SDXC";
    
    uint64_t card_size = (this->sd_.card()->sectorCount() * 512ULL) / (1024ULL * 1024ULL);
    ESP_LOGI(TAG, "SD Card: %s, Size: %llu MB", type_str, card_size);
  }

  return true;
}

void MedallionVoiceComponent::update_record_path_() {
  char path[32];
  snprintf(path, sizeof(path), "/voice_%04u.wav", this->record_counter_++);
  this->current_file_ = path;
}

bool MedallionVoiceComponent::start_recording() {
  if (this->recording_) {
    ESP_LOGW(TAG, "Already recording");
    return false;
  }

  if (!this->sd_mounted_) {
    ESP_LOGE(TAG, "Cannot record: SD card not mounted");
    this->status_ = "SD Not Ready";
    return false;
  }

  if (this->audio_codec_ == nullptr) {
    ESP_LOGE(TAG, "Cannot record: Audio codec not configured");
    this->status_ = "No Audio";
    return false;
  }

  // Generate new filename
  this->update_record_path_();
  
  // Remove existing file if present
  const char *filename = this->current_file_.c_str();
  if (filename[0] == '/') filename++;  // Strip leading slash
  
  if (this->sd_.exists(filename)) {
    this->sd_.remove(filename);
  }

  // Open file for writing
  this->record_file_ = this->sd_.open(filename, O_WRONLY | O_CREAT | O_TRUNC);
  if (!this->record_file_) {
    ESP_LOGE(TAG, "Failed to open file for recording: %s", this->current_file_.c_str());
    this->status_ = "File Error";
    return false;
  }

  // Write placeholder WAV header (44 bytes)
  uint8_t header[44] = {0};
  this->record_file_.write(header, 44);
  this->recorded_bytes_ = 0;

  // Start audio capture
  if (!this->audio_codec_->start_recording()) {
    ESP_LOGE(TAG, "Failed to start audio codec");
    this->record_file_.close();
    this->status_ = "Codec Error";
    return false;
  }

  this->recording_ = true;
  this->status_ = "Recording";
  ESP_LOGI(TAG, "Recording started: %s", this->current_file_.c_str());
  return true;
}

void MedallionVoiceComponent::stop_recording() {
  if (!this->recording_) return;

  // Stop audio capture
  if (this->audio_codec_ != nullptr) {
    this->audio_codec_->stop_recording();
  }

  // Update WAV header with actual data length
  if (this->record_file_) {
    this->write_wav_header_(this->record_file_, this->recorded_bytes_);
    this->record_file_.flush();
    this->record_file_.close();
  }

  this->recording_ = false;
  this->status_ = "Saved";
  ESP_LOGI(TAG, "Recording stopped: %s (%lu bytes)", 
           this->current_file_.c_str(), (unsigned long)this->recorded_bytes_);
}

void MedallionVoiceComponent::write_wav_header_(FsFile &file, uint32_t data_length) {
  const uint32_t byte_rate = WAV_SAMPLE_RATE * WAV_NUM_CHANNELS * (WAV_BITS_PER_SAMPLE / 8);
  const uint16_t block_align = WAV_NUM_CHANNELS * (WAV_BITS_PER_SAMPLE / 8);
  const uint32_t chunk_size = 36 + data_length;

  file.seek(0);
  
  // RIFF header
  file.write((const uint8_t *)"RIFF", 4);
  file.write((uint8_t *)&chunk_size, 4);
  file.write((const uint8_t *)"WAVE", 4);
  
  // fmt subchunk
  file.write((const uint8_t *)"fmt ", 4);
  uint32_t subchunk1_size = 16;
  uint16_t audio_format = 1;  // PCM
  uint32_t sample_rate = WAV_SAMPLE_RATE;
  file.write((uint8_t *)&subchunk1_size, 4);
  file.write((uint8_t *)&audio_format, 2);
  file.write((uint8_t *)&WAV_NUM_CHANNELS, 2);
  file.write((uint8_t *)&sample_rate, 4);
  file.write((uint8_t *)&byte_rate, 4);
  file.write((uint8_t *)&block_align, 2);
  file.write((uint8_t *)&WAV_BITS_PER_SAMPLE, 2);
  
  // data subchunk
  file.write((const uint8_t *)"data", 4);
  file.write((uint8_t *)&data_length, 4);
}

bool MedallionVoiceComponent::parse_url_(const std::string &url, std::string &host, uint16_t &port, std::string &path) {
  // Parse URL like "http://192.168.1.119:8000/upload"
  if (url.substr(0, 7) != "http://") {
    ESP_LOGE(TAG, "Only http:// URLs are supported");
    return false;
  }

  std::string rest = url.substr(7);
  size_t path_start = rest.find('/');
  if (path_start == std::string::npos) {
    ESP_LOGE(TAG, "Invalid URL: no path");
    return false;
  }

  std::string host_port = rest.substr(0, path_start);
  path = rest.substr(path_start);

  size_t colon = host_port.find(':');
  if (colon != std::string::npos) {
    host = host_port.substr(0, colon);
    port = std::stoi(host_port.substr(colon + 1));
  } else {
    host = host_port;
    port = 80;
  }

  return true;
}

bool MedallionVoiceComponent::upload_recording() {
  if (this->recording_) {
    ESP_LOGW(TAG, "Cannot upload while recording");
    this->status_ = "Stop First";
    return false;
  }

  if (!this->sd_mounted_) {
    ESP_LOGE(TAG, "Cannot upload: SD card not mounted");
    this->status_ = "SD Not Ready";
    return false;
  }

  if (this->current_file_.empty()) {
    ESP_LOGW(TAG, "No recording to upload");
    this->status_ = "No File";
    return false;
  }

  // Check WiFi connection
  if (!network::is_connected()) {
    ESP_LOGE(TAG, "Cannot upload: WiFi not connected");
    this->status_ = "No WiFi";
    return false;
  }

  // Parse URL
  std::string host, path;
  uint16_t port;
  if (!this->parse_url_(this->upload_url_, host, port, path)) {
    this->status_ = "Bad URL";
    return false;
  }

  // Open file for reading
  const char *filename = this->current_file_.c_str();
  if (filename[0] == '/') filename++;
  
  FsFile file = this->sd_.open(filename, O_RDONLY);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for upload: %s", this->current_file_.c_str());
    this->status_ = "File Error";
    return false;
  }

  size_t file_size = file.size();
  if (file_size <= 44) {
    ESP_LOGW(TAG, "File too small to upload: %u bytes", (unsigned)file_size);
    file.close();
    this->status_ = "Empty File";
    return false;
  }

  this->status_ = "Uploading";
  ESP_LOGI(TAG, "Uploading %s (%u bytes) to %s:%d%s", 
           this->current_file_.c_str(), (unsigned)file_size, host.c_str(), port, path.c_str());

  // Connect to server
  WiFiClient client;
  client.setTimeout(10000);
  
  if (!client.connect(host.c_str(), port)) {
    ESP_LOGE(TAG, "Failed to connect to upload server");
    file.close();
    this->status_ = "Connect Fail";
    return false;
  }

  // Build multipart form data
  String boundary = "----ESPHomeMedallion";
  
  // Extract filename from path
  std::string http_filename = this->current_file_;
  size_t last_slash = http_filename.rfind('/');
  if (last_slash != std::string::npos) {
    http_filename = http_filename.substr(last_slash + 1);
  }

  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"" + String(http_filename.c_str()) + "\"\r\n";
  head += "Content-Type: audio/wav\r\n\r\n";
  
  String tail = "\r\n--" + boundary + "--\r\n";
  
  size_t total_length = head.length() + file_size + tail.length();

  // Send HTTP headers
  client.print("POST " + String(path.c_str()) + " HTTP/1.1\r\n");
  client.print("Host: " + String(host.c_str()) + "\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
  client.print("Content-Length: " + String(total_length) + "\r\n\r\n");
  
  // Send multipart header
  client.print(head);

  // Send file data
  uint8_t buf[1024];
  size_t bytes_sent = 0;
  while (file.available()) {
    size_t n = file.read(buf, sizeof(buf));
    if (n == 0) break;
    client.write(buf, n);
    bytes_sent += n;
    
    // Yield to prevent watchdog
    yield();
  }
  file.close();

  // Send multipart footer
  client.print(tail);

  // Read response
  String status_line = client.readStringUntil('\n');
  status_line.trim();
  ESP_LOGD(TAG, "Upload response: %s", status_line.c_str());

  client.stop();

  // Check response
  bool success = status_line.startsWith("HTTP/1.1 200") || 
                 status_line.startsWith("HTTP/1.1 201") ||
                 status_line.startsWith("HTTP/1.0 200") ||
                 status_line.startsWith("HTTP/1.0 201");

  if (success) {
    ESP_LOGI(TAG, "Upload successful");
    this->status_ = "Uploaded";
    return true;
  } else {
    ESP_LOGE(TAG, "Upload failed: %s", status_line.c_str());
    this->status_ = "Upload Fail";
    return false;
  }
}

}  // namespace medallion_voice
}  // namespace esphome

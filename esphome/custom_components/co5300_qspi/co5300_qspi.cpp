#include "co5300_qspi.h"
#include "esphome/core/log.h"
#include <Arduino_GFX_Library.h>

namespace esphome {
namespace co5300_qspi {

static const char *const TAG = "co5300_qspi";

// Store display instances (Arduino GFX doesn't use new easily with GPIOPin)
static Arduino_DataBus *g_bus = nullptr;
static Arduino_GFX *g_gfx = nullptr;

void CO5300QSPIComponent::setup() {
  ESP_LOGI(TAG, "Setting up CO5300 QSPI AMOLED display...");

  // Get raw GPIO numbers from ESPHome pins
  uint8_t cs = this->cs_pin_->get_pin();
  uint8_t sclk = this->sclk_pin_->get_pin();
  uint8_t d0 = this->data0_pin_->get_pin();
  uint8_t d1 = this->data1_pin_->get_pin();
  uint8_t d2 = this->data2_pin_->get_pin();
  uint8_t d3 = this->data3_pin_->get_pin();
  uint8_t rst = this->reset_pin_->get_pin();

  ESP_LOGD(TAG, "Pins: CS=%d, SCLK=%d, D0=%d, D1=%d, D2=%d, D3=%d, RST=%d",
           cs, sclk, d0, d1, d2, d3, rst);

  // Hardware reset sequence
  this->reset_display_();

  // Create QSPI bus
  g_bus = new Arduino_ESP32QSPI(cs, sclk, d0, d1, d2, d3);
  this->bus_ = g_bus;

  // Create CO5300 display driver
  g_gfx = new Arduino_CO5300(
      g_bus, rst, 0 /* rotation */, false /* IPS */,
      this->width_, this->height_,
      6 /* col_offset1 */, 0 /* row_offset1 */,
      0 /* col_offset2 */, 0 /* row_offset2 */);
  this->gfx_ = g_gfx;

  // Initialize display
  if (!g_gfx->begin()) {
    ESP_LOGE(TAG, "Display initialization failed!");
    this->mark_failed();
    return;
  }

  // Clear screen
  g_gfx->fillScreen(BLACK);

  // Set brightness
  ((Arduino_CO5300 *)g_gfx)->setBrightness(this->brightness_);

  // Draw startup test pattern
  int16_t third = this->height_ / 3;
  g_gfx->fillRect(0, 0, this->width_, third, RED);
  g_gfx->fillRect(0, third, this->width_, third, GREEN);
  g_gfx->fillRect(0, 2 * third, this->width_, third, BLUE);

  // Draw centered text
  g_gfx->setTextColor(WHITE);
  g_gfx->setTextSize(2);
  g_gfx->setTextWrap(false);
  int16_t cx = this->width_ / 2;
  int16_t cy = this->height_ / 2;
  g_gfx->fillRect(cx - 120, cy - 25, 240, 50, BLACK);
  g_gfx->setCursor(cx - 100, cy - 10);
  g_gfx->println("ESPHome Medallion");

  this->initialized_ = true;
  ESP_LOGI(TAG, "CO5300 QSPI display initialized (%dx%d)", this->width_, this->height_);
}

void CO5300QSPIComponent::loop() {
  // Display doesn't need continuous updates
}

void CO5300QSPIComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CO5300 QSPI AMOLED Display:");
  ESP_LOGCONFIG(TAG, "  Resolution: %dx%d", this->width_, this->height_);
  ESP_LOGCONFIG(TAG, "  Brightness: %d", this->brightness_);
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_PIN("  SCLK Pin: ", this->sclk_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
}

void CO5300QSPIComponent::reset_display_() {
  if (this->reset_pin_ == nullptr) return;
  
  // Hardware reset sequence
  this->reset_pin_->digital_write(true);
  delay(5);
  this->reset_pin_->digital_write(false);
  delay(20);
  this->reset_pin_->digital_write(true);
  delay(20);
}

void CO5300QSPIComponent::fill_screen(uint16_t color) {
  if (!this->initialized_ || g_gfx == nullptr) return;
  g_gfx->fillScreen(color);
}

void CO5300QSPIComponent::fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (!this->initialized_ || g_gfx == nullptr) return;
  g_gfx->fillRect(x, y, w, h, color);
}

void CO5300QSPIComponent::draw_pixel(int16_t x, int16_t y, uint16_t color) {
  if (!this->initialized_ || g_gfx == nullptr) return;
  g_gfx->drawPixel(x, y, color);
}

void CO5300QSPIComponent::set_brightness(uint8_t value) {
  this->brightness_ = value;
  if (this->initialized_ && g_gfx != nullptr) {
    ((Arduino_CO5300 *)g_gfx)->setBrightness(value);
  }
}

}  // namespace co5300_qspi
}  // namespace esphome

#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include "esp_system.h"

#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS
#define LCD_BACKLIGHT_PIN 21
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))

#define LATITUDE_DEFAULT "51.5074"
#define LONGITUDE_DEFAULT "-0.1278"
#define LOCATION_DEFAULT "London"
#define DEFAULT_CAPTIVE_SSID "Aura"
#define UPDATE_INTERVAL 600000UL  // 10 minutes
#define AQI_ICON_SIZE 48

//Global initializations
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint32_t draw_buf[DRAW_BUF_SIZE / 4];
static const char *weekdays[] = {"Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat"};
int x, y, z;

//AQI credentials (change before publishing)
String my_api_read_key = "C8F0D3F7-7A45-11ED-B6F4-42010A800007"; //This is your API read key - obtain from contact@purpleair.com
String serverPath = "";

// Preferences
static Preferences prefs;
static bool use_fahrenheit = false;
static bool use_24_hour = false; // Add this global variable
static char latitude[16] = LATITUDE_DEFAULT;
static char longitude[16] = LONGITUDE_DEFAULT;
static String location = String(LOCATION_DEFAULT);
static char dd_opts[512];
static DynamicJsonDocument geoDoc(8 * 1024);
static JsonArray geoResults;

// New global for sensor ID
static char sensor_id[8] = "";  // default value
static lv_obj_t *sensor_id_ta;       // text area for settings input

// UI components
static lv_obj_t *lbl_today_temp;
static lv_obj_t *lbl_today_feels_like;
static lv_obj_t *img_today_icon;
static lv_obj_t *lbl_forecast;
static lv_obj_t *box_daily;
static lv_obj_t *box_hourly;
static lv_obj_t *lbl_daily_day[7];
static lv_obj_t *lbl_daily_high[7];
static lv_obj_t *lbl_daily_low[7];
static lv_obj_t *img_daily[7];
static lv_obj_t *lbl_hourly[7];
static lv_obj_t *lbl_precipitation_probability[7];
static lv_obj_t *lbl_hourly_temp[7];
static lv_obj_t *img_hourly[7];
static lv_obj_t *lbl_loc;
static lv_obj_t *loc_ta;
static lv_obj_t *results_dd;
static lv_obj_t *btn_close_loc;
static lv_obj_t *btn_close_obj;
static lv_obj_t *kb;
static lv_obj_t *settings_win;
static lv_obj_t *location_win = nullptr;
static lv_obj_t *unit_switch;
static lv_obj_t *clock_24hr_switch;
static lv_obj_t *lbl_clock;

// New AQI UI objects
static lv_obj_t *lbl_aqi_value;
static lv_obj_t *lbl_aqi_category;   // NEW: AQI category text
static lv_obj_t *icon_green;
static lv_obj_t *icon_yellow;
static lv_obj_t *icon_orange;
static lv_obj_t *icon_red;
static lv_obj_t *icon_purple;
static lv_obj_t *icon_darkred;

// Weather icons
LV_IMG_DECLARE(icon_blizzard);
LV_IMG_DECLARE(icon_blowing_snow);
LV_IMG_DECLARE(icon_clear_night);
LV_IMG_DECLARE(icon_cloudy);
LV_IMG_DECLARE(icon_drizzle);
LV_IMG_DECLARE(icon_flurries);
LV_IMG_DECLARE(icon_haze_fog_dust_smoke);
LV_IMG_DECLARE(icon_heavy_rain);
LV_IMG_DECLARE(icon_heavy_snow);
LV_IMG_DECLARE(icon_isolated_scattered_tstorms_day);
LV_IMG_DECLARE(icon_isolated_scattered_tstorms_night);
LV_IMG_DECLARE(icon_mostly_clear_night);
LV_IMG_DECLARE(icon_mostly_cloudy_day);
LV_IMG_DECLARE(icon_mostly_cloudy_night);
LV_IMG_DECLARE(icon_mostly_sunny);
LV_IMG_DECLARE(icon_partly_cloudy);
LV_IMG_DECLARE(icon_partly_cloudy_night);
LV_IMG_DECLARE(icon_scattered_showers_day);
LV_IMG_DECLARE(icon_scattered_showers_night);
LV_IMG_DECLARE(icon_showers_rain);
LV_IMG_DECLARE(icon_sleet_hail);
LV_IMG_DECLARE(icon_snow_showers_snow);
LV_IMG_DECLARE(icon_strong_tstorms);
LV_IMG_DECLARE(icon_sunny);
LV_IMG_DECLARE(icon_tornado);
LV_IMG_DECLARE(icon_wintry_mix_rain_snow);

// Weather Images
LV_IMG_DECLARE(image_blizzard);
LV_IMG_DECLARE(image_blowing_snow);
LV_IMG_DECLARE(image_clear_night);
LV_IMG_DECLARE(image_cloudy);
LV_IMG_DECLARE(image_drizzle);
LV_IMG_DECLARE(image_flurries);
LV_IMG_DECLARE(image_haze_fog_dust_smoke);
LV_IMG_DECLARE(image_heavy_rain);
LV_IMG_DECLARE(image_heavy_snow);
LV_IMG_DECLARE(image_isolated_scattered_tstorms_day);
LV_IMG_DECLARE(image_isolated_scattered_tstorms_night);
LV_IMG_DECLARE(image_mostly_clear_night);
LV_IMG_DECLARE(image_mostly_cloudy_day);
LV_IMG_DECLARE(image_mostly_cloudy_night);
LV_IMG_DECLARE(image_mostly_sunny);
LV_IMG_DECLARE(image_partly_cloudy);
LV_IMG_DECLARE(image_partly_cloudy_night);
LV_IMG_DECLARE(image_scattered_showers_day);
LV_IMG_DECLARE(image_scattered_showers_night);
LV_IMG_DECLARE(image_showers_rain);
LV_IMG_DECLARE(image_sleet_hail);
LV_IMG_DECLARE(image_snow_showers_snow);
LV_IMG_DECLARE(image_strong_tstorms);
LV_IMG_DECLARE(image_sunny);
LV_IMG_DECLARE(image_tornado);
LV_IMG_DECLARE(image_wintry_mix_rain_snow);

//function declarations
void create_ui();
void fetch_and_update_weather();
void create_settings_window();
static void screen_event_cb(lv_event_t *e);
static void settings_event_handler(lv_event_t *e);
//const lv_img_dsc_t *choose_image(int wmo_code, int is_day);
const lv_img_dsc_t *choose_icon(int wmo_code, int is_day);
int fetch_aqi();
int aqiFromPM(float);
int calcAQI(float, int, int, float, float);


int day_of_week(int y, int m, int d) {
  static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

String hour_of_day(int hour) {
  if(hour < 0 || hour > 23) return String("Invalid hour");

  if (use_24_hour) {
    if (hour < 10)
      return String("0") + String(hour);
    else
      return String(hour);
  } else {
    if(hour == 0)   return String("12am");
    if(hour == 12)  return String("Noon");

    bool isMorning = (hour < 12);
    String suffix = isMorning ? "am" : "pm";

    int displayHour = hour % 12;

    return String(displayHour) + suffix;
  }
}

String urlencode(const String &str) {
  String encoded = "";
  char buf[5];
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    // Unreserved characters according to RFC 3986
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      // Percent-encode others
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

static void update_clock(lv_timer_t *timer) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char buf[16];
  if (use_24_hour) {
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  } else {
    int hour = timeinfo.tm_hour % 12;
    if(hour == 0) hour = 12;
    const char *ampm = (timeinfo.tm_hour < 12) ? "am" : "pm";
    snprintf(buf, sizeof(buf), "%d:%02d%s", hour, timeinfo.tm_min, ampm);
  }
  lv_label_set_text(lbl_clock, buf);
}

static void ta_event_cb(lv_event_t *e) {
  Serial.println("In Textbox event callback");
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);

  // Show keyboard
  //TODO: read the event target and change the keyboard to full for the loc_ta field, numbers for the sensor_id_ta field
  if(ta == sensor_id_ta) {
    Serial.println("KB callback Sensor ID field");
     lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
  }
  else {
    Serial.println("KB callback location field");
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
  }

  lv_keyboard_set_textarea(kb, ta);
  lv_obj_move_foreground(kb);
  lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void kb_event_cb(lv_event_t *e) {
  Serial.println("In KB Event callback");
  lv_obj_t *kb = static_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_obj_t *current_ta = (lv_obj_t *)lv_keyboard_get_textarea(kb);
  //Serial.print("KB event target was: ");
  //Serial.println(current_ta);

  //Read that trigger is coming from location textbox and run the geocode
  if (current_ta == loc_ta && lv_event_get_code(e) == LV_EVENT_READY) {
    Serial.println("Location textbox triggered");
    const char *loc = lv_textarea_get_text(loc_ta);
    if (strlen(loc) > 0) {
      do_geocode_query(loc);
    }
  }
  
  lv_obj_add_flag((lv_obj_t *)lv_event_get_target(e), LV_OBJ_FLAG_HIDDEN);

}

static void ta_defocus_cb(lv_event_t *e) {
  lv_obj_add_flag((lv_obj_t *)lv_event_get_user_data(e), LV_OBJ_FLAG_HIDDEN);
}

void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();

    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// AQI display update function
void update_aqi_display() {
  int aqi = fetch_aqi();   // your AQI fetch function
  Serial.print("Updating AQI display with value:");
  Serial.println(aqi);

  // Hide all icons first
  lv_obj_add_flag(icon_green,   LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_yellow,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_orange,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_red,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_purple,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_darkred, LV_OBJ_FLAG_HIDDEN);

  // Case 2: AQI error (returned 999)
  if (aqi == 999) {
      lv_label_set_text(lbl_aqi_value, "--");
      lv_label_set_text(lbl_aqi_category, "Error fetching AQI");
      lv_obj_set_style_text_color(lbl_aqi_category, lv_color_hex(0xFF0000), 0);  // red warning
      return;
  }

  // --- Normal AQI case ---
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", aqi);
  lv_label_set_text(lbl_aqi_value, buf);
  lv_color_t txt_color = lv_color_hex(0xFFFFFF);
  const char *category = "Unknown";

  if (aqi <= 50) {
    lv_obj_clear_flag(icon_green, LV_OBJ_FLAG_HIDDEN);
    txt_color = lv_color_hex(0x00FF00);
    category = "Good";
  } else if (aqi <= 100) {
      lv_obj_clear_flag(icon_yellow, LV_OBJ_FLAG_HIDDEN);
      txt_color = lv_color_hex(0xFFFF00);
      category = "Moderate";
  } else if (aqi <= 150) {
      lv_obj_clear_flag(icon_orange, LV_OBJ_FLAG_HIDDEN);
      txt_color = lv_color_hex(0xFFA500);
      category = "Unhealthy";
  } else if (aqi <= 200) {
      lv_obj_clear_flag(icon_red, LV_OBJ_FLAG_HIDDEN);
      txt_color = lv_color_hex(0xFF0000);
      category = "Unhealthy";
  } else if (aqi <= 300) {
      lv_obj_clear_flag(icon_purple, LV_OBJ_FLAG_HIDDEN);
      txt_color = lv_color_hex(0x800080);
      category = "Very Unhealthy";
  } else {
      lv_obj_clear_flag(icon_darkred, LV_OBJ_FLAG_HIDDEN);
      txt_color = lv_color_hex(0x8B0000);
      category = "Hazardous";
  }

  // Apply style
  lv_obj_set_style_text_color(lbl_aqi_value, txt_color, 0);
  lv_obj_set_style_text_color(lbl_aqi_category, txt_color, 0);
  lv_label_set_text(lbl_aqi_category, category);
}

// Helper to make a colored square
lv_obj_t* create_color_icon(lv_color_t color) {
  // Create a canvas
  lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
  // Allocate buffer for the canvas (RGBA, 4 bytes per pixel)
  static lv_color_t cbuf[AQI_ICON_SIZE * AQI_ICON_SIZE];
  lv_canvas_set_buffer(canvas, cbuf, AQI_ICON_SIZE, AQI_ICON_SIZE, LV_COLOR_FORMAT_NATIVE);
  // Fill with color
  lv_canvas_fill_bg(canvas, color, LV_OPA_COVER);

  // Hide it initially (we’ll only show one at a time)
  lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);

  return canvas;
}

void create_aqi_icons() {
  icon_green   = create_color_icon(lv_color_hex(0x00FF00)); // Green
  icon_yellow  = create_color_icon(lv_color_hex(0xFFFF00)); // Yellow
  icon_orange  = create_color_icon(lv_color_hex(0xFFA500)); // Orange
  icon_red     = create_color_icon(lv_color_hex(0xFF0000)); // Red
  icon_purple  = create_color_icon(lv_color_hex(0x800080)); // Purple
  icon_darkred = create_color_icon(lv_color_hex(0x8B0000)); // Dark Red

  // Position all canvases in the same place
  lv_obj_align(icon_green, LV_ALIGN_TOP_LEFT, 20, 110);
  lv_obj_align(icon_yellow, LV_ALIGN_TOP_LEFT, 20, 110);
  lv_obj_align(icon_orange, LV_ALIGN_TOP_LEFT, 20, 110);
  lv_obj_align(icon_red, LV_ALIGN_TOP_LEFT, 20, 110);
  lv_obj_align(icon_purple, LV_ALIGN_TOP_LEFT, 20, 110);
  lv_obj_align(icon_darkred, LV_ALIGN_TOP_LEFT, 20, 110);
}

// Instead of get_aqi_icon(), we just show/hide the right canvas
void update_aqi_icon(int aqi) {
  // Hide all first
  lv_obj_add_flag(icon_green,   LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_yellow,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_orange,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_red,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_purple,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(icon_darkred, LV_OBJ_FLAG_HIDDEN);

  // Show correct one
  if (aqi <= 50) {
      lv_obj_clear_flag(icon_green, LV_OBJ_FLAG_HIDDEN);
  } else if (aqi <= 100) {
      lv_obj_clear_flag(icon_yellow, LV_OBJ_FLAG_HIDDEN);
  } else if (aqi <= 150) {
      lv_obj_clear_flag(icon_orange, LV_OBJ_FLAG_HIDDEN);
  } else if (aqi <= 200) {
      lv_obj_clear_flag(icon_red, LV_OBJ_FLAG_HIDDEN);
  } else if (aqi <= 300) {
      lv_obj_clear_flag(icon_purple, LV_OBJ_FLAG_HIDDEN);
  } else {
      lv_obj_clear_flag(icon_darkred, LV_OBJ_FLAG_HIDDEN);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  TFT_eSPI tft = TFT_eSPI();
  tft.init();
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  //tft.invertDisplay( true );

  lv_init();

  // Init touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(0);

  lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Load saved prefs
  Serial.println("Loading preferences");
  prefs.begin("weather", false);
  String lat = prefs.getString("latitude", LATITUDE_DEFAULT);
  lat.toCharArray(latitude, sizeof(latitude));
  String lon = prefs.getString("longitude", LONGITUDE_DEFAULT);
  lon.toCharArray(longitude, sizeof(longitude));
  use_fahrenheit = prefs.getBool("useFahrenheit", false);
  location = prefs.getString("location", LOCATION_DEFAULT);
  uint32_t brightness = prefs.getUInt("brightness", 255);
  use_24_hour = prefs.getBool("use24Hour", false);
  analogWrite(LCD_BACKLIGHT_PIN, brightness);

  //Load the AQI prefs
  String sid = prefs.getString("sensorID", "10000");
  sid.toCharArray(sensor_id, sizeof(sensor_id));

  Serial.print("Preferences loaded: ");
  Serial.println(lat);
  Serial.println(lon);
  Serial.println(use_fahrenheit);
  Serial.println(brightness);
  Serial.println(use_24_hour);
  Serial.println(sid);
  Serial.println("----");
  // Build initial serverPath
  serverPath = "https://api.purpleair.com/v1/sensors/" + String(sensor_id) + "?fields=pm2.5_10minute%2Ctemperature%2Chumidity";

  // Check for Wi-Fi config and request it if not available
  WiFiManager wm;
  wm.setAPCallback(apModeCallback);
  wm.autoConnect(DEFAULT_CAPTIVE_SSID);

  lv_timer_create(update_clock, 1000, NULL);

  lv_obj_clean(lv_scr_act());
  create_ui();
  fetch_and_update_weather();
  update_aqi_display();
}

void flush_wifi_splashscreen(uint32_t ms = 200) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    lv_timer_handler();
    delay(5);
  }
}

void apModeCallback(WiFiManager *mgr) {
  wifi_splash_screen();
  flush_wifi_splashscreen();
}

void loop() {
  lv_timer_handler();
  static uint32_t last = millis();

  if (millis() - last >= UPDATE_INTERVAL) {
    fetch_and_update_weather();
    update_aqi_display();
    last = millis();
  }

  lv_tick_inc(5);
  delay(5);
}

void wifi_splash_screen() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x4c8cb9), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0xa6cdec), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *lbl = lv_label_create(scr);
  lv_label_set_text(lbl,
                    "Wi-Fi Configuration:\n\n"
                    "Please connect your\n"
                    "phone or laptop to the\n"
                    "temporary Wi-Fi access\n point "
                    DEFAULT_CAPTIVE_SSID
                    "\n"
                    "to configure.\n\n"
                    "If you don't see a \n"
                    "configuration screen \n"
                    "after connecting,\n"
                    "visit http://192.168.4.1\n"
                    "in your web browser.");
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(lbl);
  lv_scr_load(scr);
}


void create_aqi_icons(lv_obj_t *parent) {
    // helper to create square icons
    Serial.println("Creating AQI Icons");
    auto make_icon = [&](lv_color_t color, int x_ofs) {
        lv_obj_t *box = lv_obj_create(parent);
        lv_obj_set_size(box, 48, 48);
        lv_obj_set_style_bg_color(box, color, 0);
        lv_obj_set_style_border_width(box, 0, 0);
        lv_obj_set_style_radius(box, 4, 0);
        lv_obj_align(box, LV_ALIGN_TOP_MID, x_ofs, 4);
        lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
        return box;
    };
    
    icon_green   = make_icon(lv_color_hex(0x00FF00), -64);
    icon_yellow  = make_icon(lv_color_hex(0xFFFF00), -64);
    icon_orange  = make_icon(lv_color_hex(0xFFA500), -64);
    icon_red     = make_icon(lv_color_hex(0xFF0000), -64);
    icon_purple  = make_icon(lv_color_hex(0x800080), -64);
    icon_darkred = make_icon(lv_color_hex(0x8B0000), -64);
}

void create_ui() {
  Serial.println("Creating UI");
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x4c8cb9), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0xa6cdec), LV_PART_MAIN | LV_STATE_DEFAULT);
  //lv_obj_set_style_bg_color(scr, lv_color_hex(0x2A4C63), LV_PART_MAIN | LV_STATE_DEFAULT);
  //lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x050A0D), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Trigger settings screen on touch
  lv_obj_add_event_cb(scr, screen_event_cb, LV_EVENT_CLICKED, NULL);

  // --- AQI icons (instead of weather icon) ---
  create_aqi_icons(scr);

  static lv_style_t default_label_style;
  lv_style_init(&default_label_style);
  lv_style_set_text_color(&default_label_style, lv_color_hex(0xFFFFFF));
  lv_style_set_text_opa(&default_label_style, LV_OPA_COVER);

  // --- AQI Value Label (instead of temp) ---
  lbl_aqi_value = lv_label_create(scr);
  lv_label_set_text(lbl_aqi_value, "--");
  lv_obj_set_style_text_font(lbl_aqi_value, &lv_font_montserrat_42, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(lbl_aqi_value, LV_ALIGN_TOP_MID, 45, 25);
  lv_obj_add_style(lbl_aqi_value, &default_label_style, LV_PART_MAIN | LV_STATE_DEFAULT);

  // --- AQI Category Label (replaces Feels Like) ---
  lbl_aqi_category = lv_label_create(scr);
  lv_label_set_text(lbl_aqi_category, "");
  lv_obj_set_style_text_font(lbl_aqi_category, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_aqi_category, lv_color_hex(0xe4ffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(lbl_aqi_category, LV_ALIGN_TOP_MID, 45, 75);

  // --- Forecast Label ---
  lbl_forecast = lv_label_create(scr);
  lv_label_set_text(lbl_forecast, "SEVEN DAY FORECAST");
  lv_obj_set_style_text_font(lbl_forecast, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_forecast, lv_color_hex(0xe4ffff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(lbl_forecast, LV_ALIGN_TOP_LEFT, 20, 110);

  box_daily = lv_obj_create(scr);
  lv_obj_set_size(box_daily, 220, 180);
  lv_obj_align(box_daily, LV_ALIGN_TOP_LEFT, 10, 135);
  lv_obj_set_style_bg_color(box_daily, lv_color_hex(0x5e9bc8), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(box_daily, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(box_daily, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box_daily, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(box_daily, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(box_daily, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(box_daily, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(box_daily, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(box_daily, daily_cb, LV_EVENT_CLICKED, NULL);

  for (int i = 0; i < 7; i++) {
    lbl_daily_day[i] = lv_label_create(box_daily);
    lbl_daily_high[i] = lv_label_create(box_daily);
    lbl_daily_low[i] = lv_label_create(box_daily);
    img_daily[i] = lv_img_create(box_daily);

    lv_obj_add_style(lbl_daily_day[i], &default_label_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_daily_day[i], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lbl_daily_day[i], LV_ALIGN_TOP_LEFT, 2, i * 24);

    lv_obj_add_style(lbl_daily_high[i], &default_label_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_daily_high[i], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lbl_daily_high[i], LV_ALIGN_TOP_RIGHT, 0, i * 24);

    lv_label_set_text(lbl_daily_low[i], "");
    lv_obj_set_style_text_color(lbl_daily_low[i], lv_color_hex(0xb9ecff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_daily_low[i], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lbl_daily_low[i], LV_ALIGN_TOP_RIGHT, -50, i * 24);

    lv_img_set_src(img_daily[i], &icon_partly_cloudy);
    lv_obj_align(img_daily[i], LV_ALIGN_TOP_LEFT, 72, i * 24);
  }

  box_hourly = lv_obj_create(scr);
  lv_obj_set_size(box_hourly, 220, 180);
  lv_obj_align(box_hourly, LV_ALIGN_TOP_LEFT, 10, 135);
  lv_obj_set_style_bg_color(box_hourly, lv_color_hex(0x5e9bc8), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(box_hourly, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(box_hourly, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box_hourly, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(box_hourly, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(box_hourly, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(box_hourly, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(box_hourly, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(box_hourly, hourly_cb, LV_EVENT_CLICKED, NULL);

  for (int i = 0; i < 7; i++) {
    lbl_hourly[i] = lv_label_create(box_hourly);
    lbl_precipitation_probability[i] = lv_label_create(box_hourly);
    lbl_hourly_temp[i] = lv_label_create(box_hourly);
    img_hourly[i] = lv_img_create(box_hourly);

    lv_obj_add_style(lbl_hourly[i], &default_label_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_hourly[i], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lbl_hourly[i], LV_ALIGN_TOP_LEFT, 2, i * 24);

    lv_obj_add_style(lbl_hourly_temp[i], &default_label_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_hourly_temp[i], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lbl_hourly_temp[i], LV_ALIGN_TOP_RIGHT, 0, i * 24);

    lv_label_set_text(lbl_precipitation_probability[i], "");
    lv_obj_set_style_text_color(lbl_precipitation_probability[i], lv_color_hex(0xb9ecff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_precipitation_probability[i], &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(lbl_precipitation_probability[i], LV_ALIGN_TOP_RIGHT, -55, i * 24);

    lv_img_set_src(img_hourly[i], &icon_partly_cloudy);
    lv_obj_align(img_hourly[i], LV_ALIGN_TOP_LEFT, 72, i * 24);
  }

  lv_obj_add_flag(box_hourly, LV_OBJ_FLAG_HIDDEN);

  // Create clock label in the top-right corner
  lbl_clock = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xb9ecff), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lbl_clock, "");
  lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, -10, 5);
}

//function to populate the location dropdown after searching for a location
void populate_results_dropdown() {
  dd_opts[0] = '\0';
  for (JsonObject item : geoResults) {
    strcat(dd_opts, item["name"].as<const char *>());
    if (item["admin1"]) {
      strcat(dd_opts, ", ");
      strcat(dd_opts, item["admin1"].as<const char *>());
    }

    strcat(dd_opts, "\n");
  }

  if (geoResults.size() > 0) {
    lv_dropdown_set_options_static(results_dd, dd_opts);
    lv_obj_add_flag(results_dd, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(btn_close_loc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_close_loc, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_close_loc, lv_palette_darken(LV_PALETTE_GREEN, 1), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_flag(btn_close_loc, LV_OBJ_FLAG_CLICKABLE);
  }
}

//location save event callback
static void location_save_event_cb(lv_event_t *e) {
  JsonArray *pres = static_cast<JsonArray *>(lv_event_get_user_data(e));
  uint16_t idx = lv_dropdown_get_selected(results_dd);

  JsonObject obj = (*pres)[idx];
  double lat = obj["latitude"].as<double>();
  double lon = obj["longitude"].as<double>();

  snprintf(latitude, sizeof(latitude), "%.6f", lat);
  snprintf(longitude, sizeof(longitude), "%.6f", lon);
  prefs.putString("latitude", latitude);
  prefs.putString("longitude", longitude);

  String opts;
  const char *name = obj["name"];
  const char *admin = obj["admin1"];
  const char *country = obj["country_code"];
  opts += name;
  if (admin) {
    opts += ", ";
    opts += admin;
  }

  prefs.putString("location", opts);
  location = prefs.getString("location");
  Serial.print("Location saved: ");
  Serial.println(opts);

  // Re‐fetch weather immediately
  lv_label_set_text(lbl_loc, opts.c_str());
  fetch_and_update_weather();

  lv_obj_del(location_win);
  location_win = nullptr;
}

static void location_cancel_event_cb(lv_event_t *e) {
  lv_obj_del(location_win);
  location_win = nullptr;
}

void screen_event_cb(lv_event_t *e) {
  Serial.println("Screen event callback triggered");
  create_settings_window();
}

void daily_cb(lv_event_t *e) {
  lv_obj_add_flag(box_daily, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(lbl_forecast, "HOURLY FORECAST");
  lv_obj_clear_flag(box_hourly, LV_OBJ_FLAG_HIDDEN);
}

void hourly_cb(lv_event_t *e) {
  lv_obj_add_flag(box_hourly, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(lbl_forecast, "SEVEN DAY FORECAST");
  lv_obj_clear_flag(box_daily, LV_OBJ_FLAG_HIDDEN);
}


static void reset_wifi_event_handler(lv_event_t *e) {
  lv_obj_t *mbox = lv_msgbox_create(lv_scr_act());
  lv_obj_t *title = lv_msgbox_add_title(mbox, "Reset");
  lv_obj_set_style_margin_left(title, 10, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

  lv_msgbox_add_text(mbox,
                     "Are you sure you want to reset "
                     "Wi-Fi credentials?\n\n"
                     "You'll need to reconnect to the Wifi SSID " DEFAULT_CAPTIVE_SSID
                     " with your phone or browser to "
                     "reconfigure Wi-Fi credentials.");
  lv_msgbox_add_close_button(mbox);

  lv_obj_t *btn_no = lv_msgbox_add_footer_button(mbox, "Cancel");
  lv_obj_t *btn_yes = lv_msgbox_add_footer_button(mbox, "Reset");

  lv_obj_set_style_bg_color(btn_yes, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_yes, lv_palette_darken(LV_PALETTE_RED, 1), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_text_color(btn_yes, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_width(mbox, 230);
  lv_obj_center(mbox);

  lv_obj_set_style_border_width(mbox, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(mbox, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_border_opa(mbox, LV_OPA_COVER,   LV_PART_MAIN);
  lv_obj_set_style_radius(mbox, 4, LV_PART_MAIN);

  lv_obj_add_event_cb(btn_yes, reset_confirm_yes_cb, LV_EVENT_CLICKED, mbox);
  lv_obj_add_event_cb(btn_no, reset_confirm_no_cb, LV_EVENT_CLICKED, mbox);
}

static void reset_confirm_yes_cb(lv_event_t *e) {
  lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
  Serial.println("Clearing Wi-Fi creds and rebooting");
  WiFiManager wm;
  wm.resetSettings();
  delay(100);
  esp_restart();
}

static void reset_confirm_no_cb(lv_event_t *e) {
  lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_del(mbox);
}

static void change_location_event_cb(lv_event_t *e) {
  if (location_win) return;

  create_location_dialog();
}

void create_location_dialog() {
  Serial.println("Creating the location dialog");
  location_win = lv_win_create(lv_scr_act());
  lv_obj_t *title = lv_win_add_title(location_win, "Change Location");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_margin_left(title, 10, 0);
  lv_obj_set_size(location_win, 240, 320);
  lv_obj_center(location_win);

  lv_obj_t *cont = lv_win_get_content(location_win);

  lv_obj_t *lbl = lv_label_create(cont);
  lv_label_set_text(lbl, "City:");
  lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 5, 10);

  loc_ta = lv_textarea_create(cont);
  lv_textarea_set_one_line(loc_ta, true);
  lv_textarea_set_placeholder_text(loc_ta, "e.g. London");
  lv_obj_set_width(loc_ta, 170);
  lv_obj_align_to(loc_ta, lbl, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  lv_obj_add_event_cb(loc_ta, ta_event_cb, LV_EVENT_CLICKED, kb);
  lv_obj_add_event_cb(loc_ta, ta_defocus_cb, LV_EVENT_DEFOCUSED, kb);

  lv_obj_t *lbl2 = lv_label_create(cont);
  lv_label_set_text(lbl2, "Search Results");
  lv_obj_align(lbl2, LV_ALIGN_TOP_LEFT, 5, 50);

  results_dd = lv_dropdown_create(cont);
  lv_obj_set_width(results_dd, 200);
  lv_obj_align(results_dd, LV_ALIGN_TOP_LEFT, 5, 70);
  lv_dropdown_set_options(results_dd, "");
  lv_obj_clear_flag(results_dd, LV_OBJ_FLAG_CLICKABLE);

  btn_close_loc = lv_btn_create(cont);
  lv_obj_set_size(btn_close_loc, 80, 40);
  lv_obj_align(btn_close_loc, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lv_obj_add_event_cb(btn_close_loc, location_save_event_cb, LV_EVENT_CLICKED, &geoResults);
  lv_obj_set_style_bg_color(btn_close_loc, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn_close_loc, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_close_loc, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_clear_flag(btn_close_loc, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *lbl_close = lv_label_create(btn_close_loc);
  lv_label_set_text(lbl_close, "Save");
  lv_obj_center(lbl_close);

  lv_obj_t *btn_cancel_loc = lv_btn_create(cont);
  lv_obj_set_size(btn_cancel_loc, 80, 40);
  lv_obj_align_to(btn_cancel_loc, btn_close_loc, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_add_event_cb(btn_cancel_loc, location_cancel_event_cb, LV_EVENT_CLICKED, &geoResults);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel_loc);
  lv_label_set_text(lbl_cancel, "Cancel");
  lv_obj_center(lbl_cancel);
}

void create_settings_window() {
  Serial.println("In the settings window");
  if (settings_win) return;

  settings_win = lv_win_create(lv_scr_act());
  lv_obj_t *title = lv_win_add_title(settings_win, "Aura Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_margin_left(title, 10, 0);

  lv_obj_center(settings_win);
  lv_obj_set_width(settings_win, 240);

  lv_obj_t *cont = lv_win_get_content(settings_win);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
 

  // Enable vertical layout + scrolling
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(cont, LV_DIR_VER);
  lv_obj_set_style_pad_gap(cont, 12, 0);   // spacing between elements
  lv_obj_set_style_pad_all(cont, 8, 0);    // padding around content
  
  // --- Keyboard (hidden until needed) ---
  if (!kb) {
    kb = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_CANCEL, NULL);
  }

  // --- Sensor ID ---
  lv_obj_t *row_sensor = lv_obj_create(cont);
  lv_obj_set_size(row_sensor, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(row_sensor, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row_sensor, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row_sensor, 5, 0);
  lv_obj_set_style_border_width(row_sensor, 0, 0);
  lv_obj_set_style_bg_opa(row_sensor, LV_OPA_TRANSP, 0);

  lv_obj_t *lbl_sensor = lv_label_create(row_sensor);
  lv_label_set_text(lbl_sensor, "Sensor ID:");
  sensor_id_ta = lv_textarea_create(row_sensor);
  lv_textarea_set_one_line(sensor_id_ta, true);
  lv_textarea_set_placeholder_text(sensor_id_ta, "Sensor ID");
  lv_textarea_set_max_length(sensor_id_ta, 6);
  lv_obj_set_width(sensor_id_ta, 80);
  lv_textarea_set_text(sensor_id_ta, sensor_id);
  lv_obj_add_event_cb(sensor_id_ta, ta_event_cb, LV_EVENT_CLICKED, kb);
  lv_obj_add_event_cb(sensor_id_ta, ta_event_cb, LV_EVENT_DEFOCUSED, kb);
  lv_obj_add_event_cb(sensor_id_ta, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
  
  // --- Brightness ---
  lv_obj_t *row_b = lv_obj_create(cont);
  lv_obj_set_size(row_b, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(row_b, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row_b, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row_b, 10, 0);
  lv_obj_set_style_border_width(row_b, 0, 0);
  lv_obj_set_style_bg_opa(row_b, LV_OPA_TRANSP, 0);

  lv_obj_t *lbl_b = lv_label_create(row_b);
  lv_label_set_text(lbl_b, "Brightness:");
  lv_obj_t *slider = lv_slider_create(row_b);
  lv_slider_set_range(slider, 10, 255);
  uint32_t saved_b = prefs.getUInt("brightness", 128);
  lv_slider_set_value(slider, saved_b, LV_ANIM_OFF);
  lv_obj_set_width(slider, 100);
  lv_obj_add_event_cb(slider, [](lv_event_t *e){
    lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
    uint32_t v = lv_slider_get_value(s);
    analogWrite(LCD_BACKLIGHT_PIN, v);
    prefs.putUInt("brightness", v);
  }, LV_EVENT_VALUE_CHANGED, NULL);

  // --- Units row ---
  lv_obj_t *row_units = lv_obj_create(cont);
  lv_obj_set_size(row_units, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(row_units, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row_units, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row_units, 6, 0);
  lv_obj_set_style_border_width(row_units, 0, 0);
  lv_obj_set_style_bg_opa(row_units, LV_OPA_TRANSP, 0);

  lv_obj_t *lbl_u = lv_label_create(row_units);
  lv_label_set_text(lbl_u, "Use °F:");
  unit_switch = lv_switch_create(row_units);
  if (use_fahrenheit) lv_obj_add_state(unit_switch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(unit_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // --- 24hr row ---
  lv_obj_t *row_24 = lv_obj_create(cont);
  lv_obj_set_size(row_24, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(row_24, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row_24, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row_24, 6, 0);
  lv_obj_set_style_border_width(row_24, 0, 0);
  lv_obj_set_style_bg_opa(row_24, LV_OPA_TRANSP, 0);

  lv_obj_t *lbl_24hr = lv_label_create(row_24);
  lv_label_set_text(lbl_24hr, "24hr:");
  clock_24hr_switch = lv_switch_create(row_24);
  if (use_24_hour) lv_obj_add_state(clock_24hr_switch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(clock_24hr_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // --- Location info ---
  lv_obj_t *lbl_loc_l = lv_label_create(cont);
  lv_label_set_text(lbl_loc_l, "Location:");
  lbl_loc = lv_label_create(cont);
  lv_label_set_text(lbl_loc, location.c_str());

  lv_obj_t *row_loc = lv_obj_create(cont);
  lv_obj_set_size(row_loc, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(row_loc, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row_loc, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row_loc, 10, 0);
  lv_obj_set_style_border_width(row_loc, 0, 0);
  lv_obj_set_style_bg_opa(row_loc, LV_OPA_TRANSP, 0);
  lv_obj_set_scrollbar_mode(row_loc, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *btn_change_loc = lv_btn_create(row_loc);
  lv_obj_set_size(btn_change_loc, 100, 40);
  lv_obj_add_event_cb(btn_change_loc, change_location_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_chg = lv_label_create(btn_change_loc);
  lv_label_set_text(lbl_chg, "Location");
  lv_obj_center(lbl_chg);

  lv_obj_t *btn_reset = lv_btn_create(row_loc);
  lv_obj_set_size(btn_reset, 100, 40);
  lv_obj_set_style_bg_color(btn_reset, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_bg_color(btn_reset, lv_palette_darken(LV_PALETTE_RED, 1), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_text_color(btn_reset, lv_color_white(), 0);
  lv_obj_add_event_cb(btn_reset, reset_wifi_event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_reset = lv_label_create(btn_reset);
  lv_label_set_text(lbl_reset, "Reset Wi-Fi");
  lv_obj_center(lbl_reset);
   
  // --- Close button ---
  btn_close_obj = lv_btn_create(cont);
  lv_obj_set_size(btn_close_obj, 80, 40);
  lv_obj_add_event_cb(btn_close_obj, settings_event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_btn = lv_label_create(btn_close_obj);
  lv_label_set_text(lbl_btn, "Close");
  lv_obj_center(lbl_btn);

}


static void settings_event_handler(lv_event_t *e) {
  Serial.println("In Settings Event Handler");
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *tgt = (lv_obj_t *)lv_event_get_target(e);

  if (tgt == unit_switch && code == LV_EVENT_VALUE_CHANGED) {
    use_fahrenheit = lv_obj_has_state(unit_switch, LV_STATE_CHECKED);
  }

  if (tgt == clock_24hr_switch && code == LV_EVENT_VALUE_CHANGED) {
    use_24_hour = lv_obj_has_state(clock_24hr_switch, LV_STATE_CHECKED);
  }

  if(tgt == sensor_id_ta && code == LV_EVENT_VALUE_CHANGED) {
    Serial.println("Sensor ID field change detected");
    strcpy(sensor_id, lv_textarea_get_text(sensor_id_ta));
    Serial.print("Sensor ID field: ");
    Serial.println(sensor_id);
  }

  if (tgt == btn_close_obj && code == LV_EVENT_CLICKED) {
    Serial.println("Close button pressed");
    prefs.putBool("useFahrenheit", use_fahrenheit);
    prefs.putBool("use24Hour", use_24_hour);
    //update purpleair and publish sensorid change
    //strncpy(sensor_id, txt, sizeof(sensor_id));
    prefs.putString("sensorID", sensor_id);

    Serial.print("sensorID stored: ");
    Serial.println((String)prefs.getString("sensorID","failed"));
    //rebuild serverpath
    serverPath = "https://api.purpleair.com/v1/sensors/" + String(sensor_id) + "?fields=pm2.5_10minute%2Ctemperature%2Chumidity";

    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    lv_obj_del(settings_win);
    settings_win = nullptr;

    fetch_and_update_weather();
    update_aqi_display();
  }
}

//code to perform the geocode query from open meteo
void do_geocode_query(const char *q) {
  geoDoc.clear();
  String url = String("https://geocoding-api.open-meteo.com/v1/search?name=") + urlencode(q) + "&count=15";

  HTTPClient http;
  http.begin(url);
  if (http.GET() == HTTP_CODE_OK) {
    auto err = deserializeJson(geoDoc, http.getString());
    if (!err) {
      geoResults = geoDoc["results"].as<JsonArray>();
      populate_results_dropdown();
    }
  }
  http.end();
}

//code to fetch weather from open meteo, and updates the UI labels with the forecast information
void fetch_and_update_weather() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String("http://api.open-meteo.com/v1/forecast?latitude=")
               + latitude + "&longitude=" + longitude
               + "&current=temperature_2m,apparent_temperature,is_day,weather_code"
               + "&daily=temperature_2m_min,temperature_2m_max,weather_code"
               + "&hourly=temperature_2m,precipitation_probability,is_day,weather_code"
               + "&forecast_hours=7"
               + "&timezone=auto";

  HTTPClient http;
  http.begin(url);

  if (http.GET() == HTTP_CODE_OK) {
    Serial.println("Updated weather from open-meteo: " + url);

    String payload = http.getString();
    DynamicJsonDocument doc(32 * 1024);

    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      float t_now = doc["current"]["temperature_2m"].as<float>();
      float t_ap = doc["current"]["apparent_temperature"].as<float>();
      int code_now = doc["current"]["weather_code"].as<int>();
      int is_day = doc["current"]["is_day"].as<int>();

      if (use_fahrenheit) {
        t_now = t_now * 9.0 / 5.0 + 32.0;
        t_ap = t_ap * 9.0 / 5.0 + 32.0;
      }

      int utc_offset_seconds = doc["utc_offset_seconds"].as<int>();
      configTime(utc_offset_seconds, 0, "pool.ntp.org", "time.nist.gov");
      Serial.print("Updating time from NTP with UTC offset: ");
      Serial.println(utc_offset_seconds);

      char unit = use_fahrenheit ? 'F' : 'C';

      JsonArray times = doc["daily"]["time"].as<JsonArray>();
      JsonArray tmin = doc["daily"]["temperature_2m_min"].as<JsonArray>();
      JsonArray tmax = doc["daily"]["temperature_2m_max"].as<JsonArray>();
      JsonArray weather_codes = doc["daily"]["weather_code"].as<JsonArray>();

      for (int i = 0; i < 7; i++) {
        const char *date = times[i];
        int year = atoi(date + 0);
        int mon = atoi(date + 5);
        int dayd = atoi(date + 8);
        int dow = day_of_week(year, mon, dayd);
        const char *dayStr = (i == 0) ? "Today" : weekdays[dow];

        float mn = tmin[i].as<float>();
        float mx = tmax[i].as<float>();
        if (use_fahrenheit) {
          mn = mn * 9.0 / 5.0 + 32.0;
          mx = mx * 9.0 / 5.0 + 32.0;
        }

        lv_label_set_text_fmt(lbl_daily_day[i], "%s", dayStr);
        lv_label_set_text_fmt(lbl_daily_high[i], "%.0f°%c", mx, unit);
        lv_label_set_text_fmt(lbl_daily_low[i], "%.0f°%c", mn, unit);
        lv_img_set_src(img_daily[i], choose_icon(weather_codes[i].as<int>(), (i == 0) ? is_day : 1));
      }

      JsonArray hours = doc["hourly"]["time"].as<JsonArray>();
      JsonArray hourly_temps = doc["hourly"]["temperature_2m"].as<JsonArray>();
      JsonArray precipitation_probabilities = doc["hourly"]["precipitation_probability"].as<JsonArray>();
      JsonArray hourly_weather_codes = doc["hourly"]["weather_code"].as<JsonArray>();
      JsonArray hourly_is_day = doc["hourly"]["is_day"].as<JsonArray>();

      for (int i = 0; i < 7; i++) {
        const char *date = hours[i];  // "YYYY-MM-DD"
        int hour = atoi(date + 11);
        int minute = atoi(date + 14);
        String hour_name = hour_of_day(hour);

        float precipitation_probability = precipitation_probabilities[i].as<float>();
        float temp = hourly_temps[i].as<float>();
        if (use_fahrenheit) {
          temp = temp * 9.0 / 5.0 + 32.0;
        }

        if (i == 0) {
          lv_label_set_text(lbl_hourly[i], "Now");
        } else {
          lv_label_set_text(lbl_hourly[i], hour_name.c_str());
        }
        lv_label_set_text_fmt(lbl_precipitation_probability[i], "%.0f%%", precipitation_probability);
        lv_label_set_text_fmt(lbl_hourly_temp[i], "%.0f°%c", temp, unit);
        lv_img_set_src(img_hourly[i], choose_icon(hourly_weather_codes[i].as<int>(), hourly_is_day[i].as<int>()));
      }


    } else {
      Serial.println("JSON parse failed");
    }
  } else {
    Serial.println("HTTP GET failed");
  }
  http.end();
}

//Code to select and draw an icon for the daily or hourly forecast
const lv_img_dsc_t* choose_icon(int code, int is_day) {
  switch (code) {
    // Clear sky
    case  0:
      return is_day
        ? &icon_sunny
        : &icon_clear_night;

    // Mainly clear
    case  1:
      return is_day
        ? &icon_mostly_sunny
        : &icon_mostly_clear_night;

    // Partly cloudy
    case  2:
      return is_day
        ? &icon_partly_cloudy
        : &icon_partly_cloudy_night;

    // Overcast
    case  3:
      return &icon_cloudy;

    // Fog / mist
    case 45:
    case 48:
      return &icon_haze_fog_dust_smoke;

    // Drizzle (light → dense)
    case 51:
    case 53:
    case 55:
      return &icon_drizzle;

    // Freezing drizzle
    case 56:
    case 57:
      return &icon_sleet_hail;

    // Rain: slight showers
    case 61:
      return is_day
        ? &icon_scattered_showers_day
        : &icon_scattered_showers_night;

    // Rain: moderate
    case 63:
      return &icon_showers_rain;

    // Rain: heavy
    case 65:
      return &icon_heavy_rain;

    // Freezing rain
    case 66:
    case 67:
      return &icon_wintry_mix_rain_snow;

    // Snow fall (light, moderate, heavy) & snow showers (light)
    case 71:
    case 73:
    case 75:
    case 85:
      return &icon_snow_showers_snow;

    // Snow grains
    case 77:
      return &icon_flurries;

    // Rain showers (slight → moderate)
    case 80:
    case 81:
      return is_day
        ? &icon_scattered_showers_day
        : &icon_scattered_showers_night;

    // Rain showers: violent
    case 82:
      return &icon_heavy_rain;

    // Heavy snow showers
    case 86:
      return &icon_heavy_snow;

    // Thunderstorm (light)
    case 95:
      return is_day
        ? &icon_isolated_scattered_tstorms_day
        : &icon_isolated_scattered_tstorms_night;

    // Thunderstorm with hail
    case 96:
    case 99:
      return &icon_strong_tstorms;

    // Fallback for any other code
    default:
      return is_day
        ? &icon_mostly_cloudy_day
        : &icon_mostly_cloudy_night;
  }
}

//function to fetch the AQI, returns integer value of the AQI or 999 on failure
int fetch_aqi() {
  Serial.println("In FetchAQI");
  if (WiFi.status() != WL_CONNECTED) return 999;

  String url = serverPath;
  float PM2_5 = 999; //set return variable to 999 which indicates failure

  HTTPClient http;
  http.useHTTP10(true);
  http.begin(url.c_str()); // Open PurpleAir URL
  http.addHeader("X-API-Key",my_api_read_key); //Send API Read Key
  if (http.GET() == HTTP_CODE_OK) {
    Serial.println("Updated aqi from Purpleair: " + url);
    String payload = http.getString();
    Serial.print(payload);
    JsonDocument doc;
    //ReadLoggingStream loggingStream(http.getStream(), Serial); //This will print the extracted Json to the Serial Monitor
    deserializeJson(doc, payload);
    JsonObject sensor = doc["sensor"]; //Opens sensor Json Object
    JsonObject sensor_stats = sensor["stats"]; // Opens sub sensor stats Object
    PM2_5 = (float)sensor_stats["pm2.5_10minute"]; //Extracts PM2_5 reading
    Serial.print("PM2.5 reading: ");
    Serial.println(PM2_5);
    Serial.print("AQI number: ");
    Serial.println(aqiFromPM(PM2_5));
  } else {
    Serial.println("HTTP GET failed");
  }
  http.end();
  return aqiFromPM(PM2_5);
}

// Converts to US AQI from raw pm2.5 data
// Returns int of AQI, or 999 if the input is incorrect
int aqiFromPM(float pm){
  if (pm < 0) return 999;
  if (pm > 1000) return 999;
  if (pm > 350.5) return calcAQI(pm, 500, 401, 500.4, 350.5);
  else if (pm > 250.5) return calcAQI(pm, 400, 301, 350.4, 250.5);
  else if (pm > 150.5) return calcAQI(pm, 300, 201, 250.4, 150.5);
  else if (pm > 55.5) return calcAQI(pm, 200, 151, 150.4, 55.5);
  else if (pm > 35.5) return calcAQI(pm, 150, 101, 55.4, 35.5);
  else if (pm > 12.1) return calcAQI(pm, 100, 51, 35.4, 12.1);
  else if (pm >= 0) return calcAQI(pm, 50, 0, 12, 0);
  else return 999;
}

// Calculate AQI from standard ranges
// Returns integer value of AQI or 999 if invalid
int calcAQI(float Cp, int Ih, int Il, float BPh, float BPl){
    int a = (Ih - Il);
    float b = (BPh - BPl);
    float c = (Cp - BPl);
    return round((a / b) * c + Il);
}

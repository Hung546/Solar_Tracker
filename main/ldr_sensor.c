/**
 * @file ldr_sensor.c
 * @brief Hiện thực thư viện đọc 4 quang trở ADC1 Oneshot (ESP-IDF v5.5)
 */

#include "ldr_sensor.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "LDR_SENSOR";

#define LDR_TL_ADC_CHAN     ADC_CHANNEL_6 // GPIO 34
#define LDR_TR_ADC_CHAN     ADC_CHANNEL_7 // GPIO 35
#define LDR_BL_ADC_CHAN     ADC_CHANNEL_4 // GPIO 32
#define LDR_BR_ADC_CHAN     ADC_CHANNEL_5 // GPIO 33

#define ADC_ATTEN_MODE      ADC_ATTEN_DB_12
#define ADC_SAMPLES         32

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static adc_cali_handle_t s_cali_tl = NULL;
static adc_cali_handle_t s_cali_tr = NULL;
static adc_cali_handle_t s_cali_bl = NULL;
static adc_cali_handle_t s_cali_br = NULL;
static bool s_cali_tl_ok = false;
static bool s_cali_tr_ok = false;
static bool s_cali_bl_ok = false;
static bool s_cali_br_ok = false;

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    return calibrated;
}

static int read_adc_filtered(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int samples) {
    int sum = 0;
    int val = 0;
    for (int i = 0; i < samples; i++) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(adc_oneshot_read(handle, chan, &val));
        sum += val;
    }
    return sum / samples;
}

static int convert_to_mv(int raw_val, bool cali_ok, adc_cali_handle_t cali_handle) {
    int voltage = 0;
    if (cali_ok && cali_handle != NULL) {
        adc_cali_raw_to_voltage(cali_handle, raw_val, &voltage);
    } else {
        voltage = (raw_val * 3300) / 4095;
    }
    return voltage;
}

esp_err_t ldr_sensor_init(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config1, &s_adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Khoi tao ADC1 Oneshot that bai: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_MODE,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, LDR_TL_ADC_CHAN, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, LDR_TR_ADC_CHAN, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, LDR_BL_ADC_CHAN, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, LDR_BR_ADC_CHAN, &chan_config));

    s_cali_tl_ok = adc_calibration_init(ADC_UNIT_1, LDR_TL_ADC_CHAN, ADC_ATTEN_MODE, &s_cali_tl);
    s_cali_tr_ok = adc_calibration_init(ADC_UNIT_1, LDR_TR_ADC_CHAN, ADC_ATTEN_MODE, &s_cali_tr);
    s_cali_bl_ok = adc_calibration_init(ADC_UNIT_1, LDR_BL_ADC_CHAN, ADC_ATTEN_MODE, &s_cali_bl);
    s_cali_br_ok = adc_calibration_init(ADC_UNIT_1, LDR_BR_ADC_CHAN, ADC_ATTEN_MODE, &s_cali_br);

    ESP_LOGI(TAG, "Khoi tao 4 kenh LDR (ADC1) thanh cong!");
    return ESP_OK;
}

esp_err_t ldr_sensor_read(ldr_readings_t *out) {
    if (out == NULL || s_adc1_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    out->tl_raw = read_adc_filtered(s_adc1_handle, LDR_TL_ADC_CHAN, ADC_SAMPLES);
    out->tr_raw = read_adc_filtered(s_adc1_handle, LDR_TR_ADC_CHAN, ADC_SAMPLES);
    out->bl_raw = read_adc_filtered(s_adc1_handle, LDR_BL_ADC_CHAN, ADC_SAMPLES);
    out->br_raw = read_adc_filtered(s_adc1_handle, LDR_BR_ADC_CHAN, ADC_SAMPLES);

    out->tl_mv = convert_to_mv(out->tl_raw, s_cali_tl_ok, s_cali_tl);
    out->tr_mv = convert_to_mv(out->tr_raw, s_cali_tr_ok, s_cali_tr);
    out->bl_mv = convert_to_mv(out->bl_raw, s_cali_bl_ok, s_cali_bl);
    out->br_mv = convert_to_mv(out->br_raw, s_cali_br_ok, s_cali_br);

    out->avg_top    = (out->tl_raw + out->tr_raw) / 2;
    out->avg_bottom = (out->bl_raw + out->br_raw) / 2;
    out->avg_left   = (out->tl_raw + out->bl_raw) / 2;
    out->avg_right  = (out->tr_raw + out->br_raw) / 2;
    out->avg_total  = (out->avg_top + out->avg_bottom) / 2;

    out->diff_v = out->avg_top - out->avg_bottom;
    out->diff_h = out->avg_left - out->avg_right;

    out->is_night = (out->avg_total < LDR_NIGHT_THRESHOLD);

    // Hướng trục đứng (Elevation)
    if (out->diff_v > LDR_DEFAULT_DEADBAND) {
        out->dir_v = LDR_V_TOP;
    } else if (out->diff_v < -LDR_DEFAULT_DEADBAND) {
        out->dir_v = LDR_V_BOTTOM;
    } else {
        out->dir_v = LDR_V_BALANCED;
    }

    // Hướng trục ngang (Azimuth)
    if (out->diff_h > LDR_DEFAULT_DEADBAND) {
        out->dir_h = LDR_H_LEFT;
    } else if (out->diff_h < -LDR_DEFAULT_DEADBAND) {
        out->dir_h = LDR_H_RIGHT;
    } else {
        out->dir_h = LDR_H_BALANCED;
    }

    return ESP_OK;
}

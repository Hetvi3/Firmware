#ifdef CONFIG_ENABLE_ SENSOR_UI

#include "sensor_ui.h"
X
unsigned long bootTime = 0;
#define EYE_COLOR_ACTIVATION_DELAY 45000

lv_chart_series_t *ui_PM1chart_series_1 = nullptr;
static lv_coord_t ui_PM1chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM25chart_series_1 = nullptr;
static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM10chart_series_1 = nullptr;
static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};

void setupCharts()
{
    ui_PM1chart_series_1 = lv_chart_add_series(ui_Chart1, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart1, ui_PM1chart_series_1, ui_PM1chart_series_1_array);
    lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM25chart_series_1 = lv_chart_add_series(ui_Chart3, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1, ui_PM25chart_series_1_array);
    lv_chart_set_range(ui_Chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM10chart_series_1 = lv_chart_add_series(ui_Chart4, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1, ui_PM10chart_series_1_array);
    lv_chart_set_range(ui_Chart4, LV_CHART_AXIS_PRIMARY_Y, 0, 50);
}

 lv_label_set_text_fmt(ui_pm1value, "%.1f", pmsData.PM_AE_UG_1_0);
            lv_label_set_text_fmt(ui_pm25value, "%.1f", pmsData.PM_AE_UG_2_5);
            lv_label_set_text_fmt(ui_pm10value, "%.1f", pmsData.PM_AE_UG_10_0);
            
#endif
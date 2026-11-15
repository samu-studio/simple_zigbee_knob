#pragma once
// Zigbee helpers
#include "Zigbee.h"

// Role comes from platformio.ini (build flag + matching partition table)
#if defined(ZIGBEE_MODE_ED)
constexpr zigbee_role_t ZB_ROLE = ZIGBEE_END_DEVICE;
#elif defined(ZIGBEE_MODE_ZCZR)
constexpr zigbee_role_t ZB_ROLE = ZIGBEE_ROUTER;
#else
#error "Zigbee mode missing: -DZIGBEE_MODE_ED or -DZIGBEE_MODE_ZCZR in platformio.ini"
#endif

// Endpoint with firmware version in the Basic cluster (swBuildId -> "Firmware" in Z2M/HA)
template <class Base>
class WithSwBuild : public Base {
public:
  using Base::Base;
  void setSwBuild(const char *ver) {
    esp_zb_attribute_list_t *basic =
      esp_zb_cluster_list_get_cluster(this->_cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (!basic) return;
    char zcl[18];  // ZCL string: length byte + max. 16 chars
    size_t n = min<size_t>(strlen(ver), 16);
    zcl[0] = (char)n;
    memcpy(zcl + 1, ver, n);
    zcl[n + 1] = 0;
    if (esp_zb_basic_cluster_add_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, zcl) != ESP_OK)
      esp_zb_cluster_update_attr(basic, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, zcl);
  }
};

// Has Z2M configured reporting for the Analog Input value of this endpoint?
inline bool zbReportingConfigured(uint8_t ep) {
  esp_zb_zcl_attr_location_info_t loc = {};
  loc.endpoint_id = ep;
  loc.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT;
  loc.cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE;
  loc.manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC;
  loc.attr_id = ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID;
  if (!esp_zb_lock_acquire(portMAX_DELAY)) return false;
  bool configured = esp_zb_zcl_find_reporting_info(loc) != nullptr;
  esp_zb_lock_release();
  return configured;
}

// Report the Analog Input presentValue of an endpoint directly to the coordinator (0x0000, EP1).
// Needs no binding; the coordinator's default response also feeds the watchdog.
// Only sends if Z2M has configured reporting for this attribute (configureReporting during
// "Reconfigure"): without that entry esp_zb_zcl_report_attr_cmd_req() aborts with a stack
// assertion -> boot loop. This also applies to sending directly without a binding.
// The caller checks beforehand that the device is on the network.
inline void zbReportAnalogInput(uint8_t ep) {
  if (!zbReportingConfigured(ep)) return;
  esp_zb_zcl_report_attr_cmd_t cmd = {};
  cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0x0000;
  cmd.zcl_basic_cmd.dst_endpoint = 1;
  cmd.zcl_basic_cmd.src_endpoint = ep;
  cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT;
  cmd.attributeID = ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID;
  cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
  cmd.dis_default_resp = 0;
  if (!esp_zb_lock_acquire(portMAX_DELAY)) return;
  esp_zb_zcl_report_attr_cmd_req(&cmd);
  esp_zb_lock_release();
}

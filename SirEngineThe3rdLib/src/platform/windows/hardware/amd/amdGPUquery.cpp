#include "platform/windows/hardware/amd/amdGPUquery.h"
#include "SirEngine/log.h"
#include "amd/adl_sdk.h"
#include <Windows.h>
#include <cstdlib>

namespace SirEngine {
namespace Hardware {

#define AMDVENDORID (1002)
#define ADL_WARNING_NO_DATA -100

static HINSTANCE hDLL = nullptr;

// Definitions of the used function pointers. Add more if you use other ADL
// APIs
typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int (*ADL_MAIN_CONTROL_DESTROY)();
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int *);
typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
typedef int (*ADL_ADAPTER_ACTIVE_GET)(int, int *);
typedef int (*ADL_OVERDRIVE_CAPS)(int iAdapterIndex, int *iSupported,
                                  int *iEnabled, int *iVersion);
typedef int (*ADL_OVERDRIVE5_THERMALDEVICES_ENUM)(
    int iAdapterIndex, int iThermalControllerIndex,
    ADLThermalControllerInfo *lpThermalControllerInfo);
typedef int (*ADL_OVERDRIVE5_ODPARAMETERS_GET)(int iAdapterIndex,
                                               ADLODParameters *lpOdParameters);
typedef int (*ADL_OVERDRIVE5_TEMPERATURE_GET)(int iAdapterIndex,
                                              int iThermalControllerIndex,
                                              ADLTemperature *lpTemperature);
typedef int (*ADL_OVERDRIVE5_FANSPEED_GET)(int iAdapterIndex,
                                           int iThermalControllerIndex,
                                           ADLFanSpeedValue *lpFanSpeedValue);
typedef int (*ADL_OVERDRIVE5_FANSPEEDINFO_GET)(int iAdapterIndex,
                                               int iThermalControllerIndex,
                                               ADLFanSpeedInfo *lpFanSpeedInfo);
typedef int (*ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET)(
    int iAdapterIndex, int iDefault,
    ADLODPerformanceLevels *lpOdPerformanceLevels);
typedef int (*ADL_OVERDRIVE5_ODPARAMETERS_GET)(int iAdapterIndex,
                                               ADLODParameters *lpOdParameters);
typedef int (*ADL_OVERDRIVE5_CURRENTACTIVITY_GET)(int iAdapterIndex,
                                                  ADLPMActivity *lpActivity);
typedef int (*ADL_OVERDRIVE5_FANSPEED_SET)(int iAdapterIndex,
                                           int iThermalControllerIndex,
                                           ADLFanSpeedValue *lpFanSpeedValue);
typedef int (*ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET)(
    int iAdapterIndex, ADLODPerformanceLevels *lpOdPerformanceLevels);
typedef int (*ADL_OVERDRIVE5_POWERCONTROL_CAPS)(int iAdapterIndex,
                                                int *lpSupported);
typedef int (*ADL_OVERDRIVE5_POWERCONTROLINFO_GET)(
    int iAdapterIndex, ADLPowerControlInfo *lpPowerControlInfo);
typedef int (*ADL_OVERDRIVE5_POWERCONTROL_GET)(int iAdapterIndex,
                                               int *lpCurrentValue,
                                               int *lpDefaultValue);
typedef int (*ADL_OVERDRIVE5_POWERCONTROL_SET)(int iAdapterIndex, int iValue);
typedef int (*ADL_OVERDRIVE6_FANSPEED_GET)(int iAdapterIndex,
                                           ADLOD6FanSpeedInfo *lpFanSpeedInfo);
typedef int (*ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS)(
    int iAdapterIndex, ADLOD6ThermalControllerCaps *lpThermalControllerCaps);
typedef int (*ADL_OVERDRIVE6_TEMPERATURE_GET)(int iAdapterIndex,
                                              int *lpTemperature);
typedef int (*ADL_OVERDRIVE6_CAPABILITIES_GET)(
    int iAdapterIndex, ADLOD6Capabilities *lpODCapabilities);
typedef int (*ADL_OVERDRIVE6_STATEINFO_GET)(int iAdapterIndex, int iStateType,
                                            ADLOD6StateInfo *lpStateInfo);
typedef int (*ADL_OVERDRIVE6_CURRENTSTATUS_GET)(
    int iAdapterIndex, ADLOD6CurrentStatus *lpCurrentStatus);
typedef int (*ADL_OVERDRIVE6_POWERCONTROL_CAPS)(int iAdapterIndex,
                                                int *lpSupported);
typedef int (*ADL_OVERDRIVE6_POWERCONTROLINFO_GET)(
    int iAdapterIndex, ADLOD6PowerControlInfo *lpPowerControlInfo);
typedef int (*ADL_OVERDRIVE6_POWERCONTROL_GET)(int iAdapterIndex,
                                               int *lpCurrentValue,
                                               int *lpDefaultValue);
typedef int (*ADL_OVERDRIVE6_FANSPEED_SET)(
    int iAdapterIndex, ADLOD6FanSpeedValue *lpFanSpeedValue);
typedef int (*ADL_OVERDRIVE6_STATE_SET)(int iAdapterIndex, int iStateType,
                                        ADLOD6StateInfo *lpStateInfo);
typedef int (*ADL_OVERDRIVE6_POWERCONTROL_SET)(int iAdapterIndex, int iValue);

static ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;
static ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy;
static ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
static ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get;
static ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get;
static ADL_OVERDRIVE_CAPS ADL_Overdrive_Caps;

static ADL_OVERDRIVE5_THERMALDEVICES_ENUM ADL_Overdrive5_ThermalDevices_Enum;
static ADL_OVERDRIVE5_ODPARAMETERS_GET ADL_Overdrive5_ODParameters_Get;
static ADL_OVERDRIVE5_TEMPERATURE_GET ADL_Overdrive5_Temperature_Get;
static ADL_OVERDRIVE5_FANSPEED_GET ADL_Overdrive5_FanSpeed_Get;
static ADL_OVERDRIVE5_FANSPEEDINFO_GET ADL_Overdrive5_FanSpeedInfo_Get;
static ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET
    ADL_Overdrive5_ODPerformanceLevels_Get;
static ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;
static ADL_OVERDRIVE5_FANSPEED_SET ADL_Overdrive5_FanSpeed_Set;
static ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET
    ADL_Overdrive5_ODPerformanceLevels_Set;
static ADL_OVERDRIVE5_POWERCONTROL_CAPS ADL_Overdrive5_PowerControl_Caps;
static ADL_OVERDRIVE5_POWERCONTROLINFO_GET ADL_Overdrive5_PowerControlInfo_Get;
static ADL_OVERDRIVE5_POWERCONTROL_GET ADL_Overdrive5_PowerControl_Get;
static ADL_OVERDRIVE5_POWERCONTROL_SET ADL_Overdrive5_PowerControl_Set;
static ADL_OVERDRIVE6_STATE_SET ADL_Overdrive6_State_Set;

// Memory allocation function
static void *__stdcall ADL_Main_Memory_Alloc(int iSize) {
  void *lpBuffer = malloc(iSize);
  return lpBuffer;
}


void AMDGPUQuery::update() {

  auto now = std::chrono::high_resolution_clock::now();
  auto delta = now - lastUpdate;
  auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta);

  if (int_ms.count() < m_deltaTimeInMS) {
    return;
  }
  lastUpdate = now;

  if (m_invalid) {
    return;
  }
  ADLThermalControllerInfo termalControllerInfo = {0};
  termalControllerInfo.iSize = sizeof(ADLThermalControllerInfo);

  ADLFanSpeedInfo fanSpeedInfo = {0};
  int maxThermalControllerIndex = 0;

  int ADL_Err = ADL_ERR;
  for (int iThermalControllerIndex = 0; iThermalControllerIndex < 10;
       iThermalControllerIndex++) {
    ADL_Err = ADL_Overdrive5_ThermalDevices_Enum(
        m_adapterId, iThermalControllerIndex, &termalControllerInfo);

    if (ADL_Err == ADL_WARNING_NO_DATA) {
      maxThermalControllerIndex = iThermalControllerIndex - 1;
      break;
    }

    if (ADL_Err == ADL_WARNING_NO_DATA) {
      SE_CORE_ERROR("AMD ADL: Failed to enumerate thermal devices\n");
      m_invalid = true;
      return;
    }

    if (termalControllerInfo.iThermalDomain == ADL_DL_THERMAL_DOMAIN_GPU) {
      ADLTemperature adlTemperature = {0};
      adlTemperature.iSize = sizeof(ADLTemperature);
      if (ADL_OK != ADL_Overdrive5_Temperature_Get(m_adapterId,
                                                   iThermalControllerIndex,
                                                   &adlTemperature)) {
        SE_CORE_ERROR("AMD ADL Failed to get thermal devices temperature\n");
        m_invalid = true;
        return;
      }
      m_temp = float(adlTemperature.iTemperature) /
               1000.0f; // The temperature is returned in millidegrees Celsius.

      fanSpeedInfo.iSize = sizeof(ADLFanSpeedInfo);
      if (ADL_OK != ADL_Overdrive5_FanSpeedInfo_Get(
                        m_adapterId, iThermalControllerIndex, &fanSpeedInfo)) {
        SE_CORE_ERROR("Failed to get fan caps\n");
        m_invalid = true;
        return;
      }

      ADLFanSpeedValue fanSpeedValue = {0};
      fanSpeedValue.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
      if (ADL_OK != ADL_Overdrive5_FanSpeed_Get(
                        m_adapterId, iThermalControllerIndex, &fanSpeedValue)) {
        SE_CORE_ERROR("Failed to get fan speed\n");
        m_invalid = true;
        return;
      }

      m_fanSpeed = fanSpeedValue.iFanSpeed;
      m_maxFanSpeed = fanSpeedInfo.iMaxPercent;
      m_minFanSpeed = fanSpeedInfo.iMinPercent;
    }
  }

  ADLODParameters overdriveParameters = {0};
  overdriveParameters.iSize = sizeof(ADLODParameters);

  if (ADL_OK !=
      ADL_Overdrive5_ODParameters_Get(m_adapterId, &overdriveParameters)) {
    SE_CORE_ERROR("AMD ADL Failed to get overdrive parameters\n");
    m_invalid = true;
    return;
  }

  m_minCoreFreq = overdriveParameters.sEngineClock.iMin / 100;
  m_maxCoreFreq = overdriveParameters.sEngineClock.iMax / 100;

  m_minMemFreq = overdriveParameters.sMemoryClock.iMin / 100;
  m_maxMemFreq = overdriveParameters.sMemoryClock.iMax / 100;

  // Getting real current values for clocks, performance levels, voltage
  // effective in the system.
  ADLPMActivity activity = {0};
  activity.iSize = sizeof(ADLPMActivity);
  if (ADL_OK != ADL_Overdrive5_CurrentActivity_Get(m_adapterId, &activity)) {
    SE_CORE_ERROR("Failed to get current GPU activity.");
    m_invalid = true;
    return;
  }
  m_coreFreq = activity.iEngineClock / 100;
  m_memFreq = activity.iMemoryClock / 100;
  m_usage = activity.iActivityPercent;
}
bool AMDGPUQuery::initialize(int deltaTimeInMS) {

  m_deltaTimeInMS = deltaTimeInMS;
  hDLL = LoadLibrary(L"atiadlxx.dll");
  if (NULL == hDLL) {
    SE_CORE_ERROR("ADL library not found!");
    return 0;
  }

  LPAdapterInfo lpAdapterInfo = NULL;
  int i;
  int iNumberAdapters = 0;

  ADL_Main_Control_Create =
      (ADL_MAIN_CONTROL_CREATE)GetProcAddress(hDLL, "ADL_Main_Control_Create");
  ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(
      hDLL, "ADL_Main_Control_Destroy");
  ADL_Adapter_NumberOfAdapters_Get =
      (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(
          hDLL, "ADL_Adapter_NumberOfAdapters_Get");
  ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(
      hDLL, "ADL_Adapter_AdapterInfo_Get");
  ADL_Adapter_Active_Get =
      (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(hDLL, "ADL_Adapter_Active_Get");
  ADL_Overdrive_Caps =
      (ADL_OVERDRIVE_CAPS)GetProcAddress(hDLL, "ADL_Overdrive_Caps");

  if (NULL == ADL_Main_Control_Create || NULL == ADL_Main_Control_Destroy ||
      NULL == ADL_Adapter_NumberOfAdapters_Get ||
      NULL == ADL_Adapter_AdapterInfo_Get || NULL == ADL_Adapter_Active_Get ||
      NULL == ADL_Overdrive_Caps) {
    SE_CORE_ERROR("ADL's API is missing!");
    return false;
  }

  // Initialize ADL. The second parameter is 1, which means:
  // retrieve adapter information only for adapters that are physically present
  // and enabled in the system
  if (ADL_OK != ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1)) {
    SE_CORE_ERROR("ADL Initialization Error!");
    return false;
  }

  // Obtain the number of adapters for the system
  if (ADL_OK != ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters)) {
    SE_CORE_ERROR("AMD ADL: Cannot get the number of adapters!");
    return false;
  }

  if (0 < iNumberAdapters) {
    lpAdapterInfo =
        (LPAdapterInfo)malloc(sizeof(AdapterInfo) * iNumberAdapters);
    memset(lpAdapterInfo, '\0', sizeof(AdapterInfo) * iNumberAdapters);

    // Get the AdapterInfo structure for all adapters in the system
    ADL_Adapter_AdapterInfo_Get(lpAdapterInfo,
                                sizeof(AdapterInfo) * iNumberAdapters);
  }

  // Looking for first present and active adapter in the system
  for (i = 0; i < iNumberAdapters; i++) {
    int adapterActive = 0;
    AdapterInfo adapterInfo = lpAdapterInfo[i];
    ADL_Adapter_Active_Get(adapterInfo.iAdapterIndex, &adapterActive);
    if (adapterActive && adapterInfo.iVendorID == AMDVENDORID) {
      m_adapterId = adapterInfo.iAdapterIndex;
      break;
    }
  }

  if (-1 == m_adapterId) {
    SE_CORE_ERROR("Cannot find active AMD adapter");
    return false;
  }

  ADL_Overdrive5_ThermalDevices_Enum =
      (ADL_OVERDRIVE5_THERMALDEVICES_ENUM)GetProcAddress(
          hDLL, "ADL_Overdrive5_ThermalDevices_Enum");
  ADL_Overdrive5_ODParameters_Get =
      (ADL_OVERDRIVE5_ODPARAMETERS_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_ODParameters_Get");
  ADL_Overdrive5_Temperature_Get =
      (ADL_OVERDRIVE5_TEMPERATURE_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_Temperature_Get");
  ADL_Overdrive5_FanSpeed_Get = (ADL_OVERDRIVE5_FANSPEED_GET)GetProcAddress(
      hDLL, "ADL_Overdrive5_FanSpeed_Get");
  ADL_Overdrive5_FanSpeedInfo_Get =
      (ADL_OVERDRIVE5_FANSPEEDINFO_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_FanSpeedInfo_Get");
  ADL_Overdrive5_ODPerformanceLevels_Get =
      (ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_ODPerformanceLevels_Get");
  ADL_Overdrive5_CurrentActivity_Get =
      (ADL_OVERDRIVE5_CURRENTACTIVITY_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_CurrentActivity_Get");
  ADL_Overdrive5_FanSpeed_Set = (ADL_OVERDRIVE5_FANSPEED_SET)GetProcAddress(
      hDLL, "ADL_Overdrive5_FanSpeed_Set");
  ADL_Overdrive5_ODPerformanceLevels_Set =
      (ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET)GetProcAddress(
          hDLL, "ADL_Overdrive5_ODPerformanceLevels_Set");
  ADL_Overdrive5_PowerControl_Caps =
      (ADL_OVERDRIVE5_POWERCONTROL_CAPS)GetProcAddress(
          hDLL, "ADL_Overdrive5_PowerControl_Caps");
  ADL_Overdrive5_PowerControlInfo_Get =
      (ADL_OVERDRIVE5_POWERCONTROLINFO_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_PowerControlInfo_Get");
  ADL_Overdrive5_PowerControl_Get =
      (ADL_OVERDRIVE5_POWERCONTROL_GET)GetProcAddress(
          hDLL, "ADL_Overdrive5_PowerControl_Get");
  ADL_Overdrive5_PowerControl_Set =
      (ADL_OVERDRIVE5_POWERCONTROL_SET)GetProcAddress(
          hDLL, "ADL_Overdrive5_PowerControl_Set");
  ADL_Overdrive6_State_Set = (ADL_OVERDRIVE6_STATE_SET)GetProcAddress(
      hDLL, "ADL_Overdrive6_State_Set");

  if (NULL == ADL_Overdrive5_ThermalDevices_Enum ||
      NULL == ADL_Overdrive5_Temperature_Get ||
      NULL == ADL_Overdrive5_FanSpeedInfo_Get ||
      NULL == ADL_Overdrive5_ODPerformanceLevels_Get ||
      NULL == ADL_Overdrive5_ODParameters_Get ||
      NULL == ADL_Overdrive5_CurrentActivity_Get ||
      NULL == ADL_Overdrive5_FanSpeed_Set ||
      NULL == ADL_Overdrive5_ODPerformanceLevels_Set ||
      NULL == ADL_Overdrive5_PowerControl_Caps ||
      NULL == ADL_Overdrive5_PowerControlInfo_Get ||
      NULL == ADL_Overdrive5_PowerControl_Get ||
      NULL == ADL_Overdrive5_PowerControl_Set) {
    SE_CORE_ERROR("ADL's API is missing!\n");
    return false;
  }

  lastUpdate = std::chrono::high_resolution_clock::now();
  return true;
}
} // namespace Hardware
} // namespace SirEngine

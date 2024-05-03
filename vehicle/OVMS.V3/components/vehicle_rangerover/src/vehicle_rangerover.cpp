/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011        Sonny Chen @ EPRO/DX
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include "ovms_log.h"
static const char *TAG = "v-range";

#include <stdio.h>
#include "vehicle_rangerover.h"
// command
void xrv_set_pump_state(int verbosity, OvmsWriter *writer, OvmsCommand *cmd, int argc, const char *const *argv)
{
  OvmsVehicleRangeRover *rangerover = (OvmsVehicleRangeRover *)MyVehicleFactory.ActiveVehicle();
  if (strcmp(argv[0], "on") == 0)
    rangerover->StartPump();
  if (strcmp(argv[0], "off") == 0)
    rangerover->StopPump();
  // soul->SetHeadLightDelay(strcmp(argv[0],"on")==0);
}
// task for the pump

void vTaskRange(void *pvParameters)
{
  configASSERT(((uint32_t)pvParameters) == 1);
  for (;;)
  {
    OvmsVehicleRangeRover *rangerover = (OvmsVehicleRangeRover *)MyVehicleFactory.ActiveVehicle();
    // if(rangerover->VehicleType())
    static uint8_t loopcount = 0;
    static bool pump_first_time = true;


    
	uint8_t data[8] = {};
    if (pump_first_time)
    {
      data[0] = 0x02;
      data[1] = 0x10;
      data[2] = 0x03;
      data[3] = 0x00;
      data[4] = 0x00;
      pump_first_time = false;
    }
    else
    { // not the first start
      data[0] = 0x04;
      data[1] = 0x31;
      data[2] = 0x01;
      data[3] = 0x02;
      data[4] = 0x02;
    }

    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
	  rangerover->m_can1->WriteStandard(0x7E4, 8, data);
    loopcount++;
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (loopcount >= 8)
    { // 8*500ms = 4 sec
      loopcount = 0;
      pump_first_time = true;
    }
  }
}

OvmsVehicleRangeRover::OvmsVehicleRangeRover()
{
  ESP_LOGI(TAG, "Generic RANGE vehicle module");
  RegisterCanBus(1, CAN_MODE_ACTIVE, CAN_SPEED_500KBPS); // Tesla Model S/X CAN3: Powertrain
  cmd_xrv = MyCommandApp.RegisterCommand("xrv", "Range Rover");
  cmd_xrv->RegisterCommand("pump", "Start / Stop pump", xrv_set_pump_state, "<on/off>", 1, 1);
}

OvmsVehicleRangeRover::~OvmsVehicleRangeRover()
{
  // Just to be safe
  this->StopPump();
  ESP_LOGI(TAG, "Shutdown RANGE vehicle module");
}

class OvmsVehicleRangeRoverInit
{
public:
  OvmsVehicleRangeRoverInit();
} MyOvmsVehicleRangeRoverInit __attribute__((init_priority(9000)));

OvmsVehicleRangeRoverInit::OvmsVehicleRangeRoverInit()
{
  ESP_LOGI(TAG, "Registering Vehicle: Range (9000)");

  MyVehicleFactory.RegisterVehicle<OvmsVehicleRangeRover>("RRV", "Range Rover");
}

BaseType_t xReturned;
TaskHandle_t xHandle = NULL;
void OvmsVehicleRangeRover::StartPump()
{
  xReturned = xTaskCreate(
      vTaskRange,      
      "RangePump",     
      4096,            
      (void *)1,       
      tskIDLE_PRIORITY,
      &xHandle);       
     
}

void OvmsVehicleRangeRover::StopPump()
{
  if (xReturned == pdPASS && xHandle != nullptr)
  {
    /* The task was created.  Use the task's handle to delete the task. */
    vTaskDelete(xHandle);
    xHandle = nullptr;
  }
}
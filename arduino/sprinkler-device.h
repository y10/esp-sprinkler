#ifndef SPRINKLER_DEVICE_H
#define SPRINKLER_DEVICE_H

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "sprinkler.h"
#include "schedule.h"

#define EEPROM_SIZE 1024

struct SchedulerConfig
{
  bool enabled;
  unsigned int hour;
  unsigned int minute;
  unsigned int duration;
};

struct SprinklerConfig
{
  uint8_t version;
  char disp_name[50];
  char upds_addr[50];
  SchedulerConfig scheduler[8];
};

class SprinklerDevice
{
private:
  std::function<void()> onSetup;
  uint8_t led_pin;
  uint8_t rel_pin;

  String host_name;
  String disp_name;
  String safe_name;
  String upds_addr;

public:
  SprinklerDevice(std::function<void(void)> onSetupCallback, uint8_t led, uint8_t rel)
   : onSetup(onSetupCallback)
   , led_pin(led)
   , rel_pin(rel)
  {
    disp_name = "Sprinkler";
    safe_name = "sprinkler";
    host_name = "sprinkler-" + String(ESP.getChipId(), HEX);
    upds_addr = "http://ota.voights.net/sprinkler.bin";
  }

  const String hostname() const
  {
    return host_name;
  }

  const String dispname() const
  {
    return disp_name;
  }

  const String safename() const
  {
    return safe_name;
  }

  bool dispname(const char *name)
  {
    bool changed = false;
    if (strlen(name) > 0)
    {
      if (!disp_name.equals(name))
      {
        disp_name = name;
        changed = true;
      }
      if (!safe_name.equals(disp_name))
      {
        safe_name = name;
        safe_name.replace(" ", "_");
        safe_name.toLowerCase();
        changed = true;
      }
    }

    return changed;
  }

  const String updsaddr() const
  {
    return upds_addr;
  }

  const String updsaddr(const char *addr)
  {
    upds_addr = addr;

    if (upds_addr.indexOf("://") == -1)
    {
      upds_addr = "http://" + upds_addr;
    }

    return upds_addr;
  }

  void setup()
  {
    onSetup();
  }

  void load()
  {
    Serial.println("[EEPROM] reading...");
    EEPROM.begin(EEPROM_SIZE);
    SprinklerConfig config;
    EEPROM.get(0, config);
    if (config.version > 0)
    {
      dispname(config.disp_name);
      updsaddr(config.upds_addr);

      if (config.version > 1)
      {
        Schedule.setDuration(config.scheduler[0].duration);
        Schedule.setHour(config.scheduler[0].hour);
        Schedule.setMinute(config.scheduler[0].minute);
        if (config.scheduler[0].enabled) Schedule.enable();

        for (int day = (int)dowSunday; day <= (int)dowSaturday; day++)
        {
          ScheduleClass& skd = Schedule.get((timeDayOfWeek_t)day);
          skd.setDuration(config.scheduler[day].duration);
          skd.setHour(config.scheduler[day].hour);
          skd.setMinute(config.scheduler[day].minute);

          if (config.scheduler[day].enabled) skd.enable();
        }
      }
    }
    else
    {
      Serial.println("[EEPROM] not found.");
    }
  }

  void save()
  {
    Serial.println("[EEPROM] saving");
    SprinklerConfig config = {
      /*version*/       2, 
      /*display_name*/  {0}, 
      /*upds_addr*/     {0}, 
      /*scheduler*/     {
        /*everyday*/         {    Schedule.isEnabled(),     Schedule.getHour(), Schedule.getMinute(),     Schedule.getDuration()}, 
        /*sun*/              {Schedule.Sun.isEnabled(), Schedule.Sun.getHour(), Schedule.Sun.getMinute(), Schedule.Sun.getDuration()},
        /*mon*/              {Schedule.Mon.isEnabled(), Schedule.Mon.getHour(), Schedule.Mon.getMinute(), Schedule.Mon.getDuration()}, 
        /*tue*/              {Schedule.Tue.isEnabled(), Schedule.Tue.getHour(), Schedule.Tue.getMinute(), Schedule.Tue.getDuration()}, 
        /*wed*/              {Schedule.Wed.isEnabled(), Schedule.Wed.getHour(), Schedule.Wed.getMinute(), Schedule.Wed.getDuration()}, 
        /*thu*/              {Schedule.Thu.isEnabled(), Schedule.Thu.getHour(), Schedule.Thu.getMinute(), Schedule.Thu.getDuration()}, 
        /*fri*/              {Schedule.Fri.isEnabled(), Schedule.Fri.getHour(), Schedule.Fri.getMinute(), Schedule.Fri.getDuration()}, 
        /*sat*/              {Schedule.Sat.isEnabled(), Schedule.Sat.getHour(), Schedule.Sat.getMinute(), Schedule.Sat.getDuration()} 
      }
    };
    strcpy(config.disp_name, disp_name.c_str());
    strcpy(config.upds_addr, upds_addr.c_str());
    EEPROM.put(0, config);
    EEPROM.commit();
  }

  void clear()
  {
    Serial.println("[EEPROM] clear");
    for (int i = 0; i < EEPROM.length(); i++)
    {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
  }

  void turnOn()
  {
    digitalWrite(led_pin, LOW);
    digitalWrite(rel_pin, HIGH);
  }

  void turnOff()
  {
    digitalWrite(led_pin, HIGH);
    digitalWrite(rel_pin, LOW);
  }

  virtual void reset()
  {
    Serial.println("[MAIN] Factory reset requested.");
    Serial.println("[EEPROM] clear");
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();

    WiFi.disconnect(true);
    ESP.restart();
  }

  virtual void restart()
  {
    Serial.println("[MAIN] Restarting...");
    ESP.restart();
  }

  String toJSON()
  {
    return (String) "{" +
           "\r\n  \"disp_name\": \"" + disp_name + "\"" +
           "\r\n ,\"host_name\": \"" + host_name + "\"" +
           "\r\n ,\"upds_addr\": \"" + upds_addr + "\"" +
           "\r\n}";
  }
};

#endif

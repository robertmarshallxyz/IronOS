
#include "Buttons.hpp"
#include "OperatingModes.h"

uint8_t buttonAF[sizeof(buttonA)];
uint8_t buttonBF[sizeof(buttonB)];
uint8_t disconnectedTipF[sizeof(disconnectedTip)];
bool    showExitMenuTransition = false;

void renderHomeScreenAssets(void) {

  // Generate the flipped screen into ram for later use
  // flipped is generated by flipping each row
  for (int row = 0; row < 2; row++) {
    for (int x = 0; x < 42; x++) {
      buttonAF[(row * 42) + x]         = buttonA[(row * 42) + (41 - x)];
      buttonBF[(row * 42) + x]         = buttonB[(row * 42) + (41 - x)];
      disconnectedTipF[(row * 42) + x] = disconnectedTip[(row * 42) + (41 - x)];
    }
  }
}

OperatingMode handleHomeButtons(const ButtonState buttons, guiContext *cxt) {
  if (buttons != BUTTON_NONE && cxt->scratch_state.state1 == 0) {
    return OperatingMode::HomeScreen; // Ignore button press
  } else {
    cxt->scratch_state.state1 = 1;
  }
  switch (buttons) {
  case BUTTON_NONE:
    // Do nothing
    break;
  case BUTTON_BOTH:
    break;

  case BUTTON_B_LONG:
    cxt->transitionMode = TransitionAnimation::Up;
    return OperatingMode::DebugMenuReadout;
    break;
  case BUTTON_F_LONG:
#ifdef PROFILE_SUPPORT
    if (!isTipDisconnected()) {
      cxt->transitionMode = TransitionAnimation::Left;
      return OperatingMode::SolderingProfile;
    } else {
      return OperatingMode::HomeScreen;
    }
#else
    cxt->transitionMode = TransitionAnimation::Left;
    return OperatingMode::TemperatureAdjust;
#endif
    break;
  case BUTTON_F_SHORT:
    if (!isTipDisconnected()) {
      cxt->transitionMode = TransitionAnimation::Left;
      return OperatingMode::Soldering;
    }
    break;
  case BUTTON_B_SHORT:
    cxt->transitionMode = TransitionAnimation::Right;
    return OperatingMode::SettingsMenu;
    break;
  default:
    break;
  }
  return OperatingMode::HomeScreen;
}

void drawDetailedHomeScreen(uint32_t tipTemp) {
  if (isTipDisconnected()) {
    if (OLED::getRotation()) {
      // in right handed mode we want to draw over the first part
      OLED::drawArea(54, 0, 42, 16, disconnectedTipF);
    } else {
      OLED::drawArea(0, 0, 42, 16, disconnectedTip);
    }
    if (OLED::getRotation()) {
      OLED::setCursor(-1, 0);
    } else {
      OLED::setCursor(42, 0);
    }
    uint32_t Vlt = getInputVoltageX10(getSettingValue(SettingsOptions::VoltageDiv), 0);
    OLED::printNumber(Vlt / 10, 2, FontStyle::LARGE);
    OLED::print(LargeSymbolDot, FontStyle::LARGE);
    OLED::printNumber(Vlt % 10, 1, FontStyle::LARGE);
    if (OLED::getRotation()) {
      OLED::setCursor(48, 8);
    } else {
      OLED::setCursor(91, 8);
    }
    OLED::print(SmallSymbolVolts, FontStyle::SMALL);
  } else {
    if (!(getSettingValue(SettingsOptions::CoolingTempBlink) && (tipTemp > 55) && (xTaskGetTickCount() % 1000 < 300))) {
      // Blink temp if setting enable and temp < 55°
      // 1000 tick/sec
      // OFF 300ms ON 700ms
      gui_drawTipTemp(true, FontStyle::LARGE); // draw in the temp
    }
    if (OLED::getRotation()) {
      OLED::setCursor(6, 0);
    } else {
      OLED::setCursor(73, 0); // top right
    }
    // draw set temp
    OLED::printNumber(getSettingValue(SettingsOptions::SolderingTemp), 3, FontStyle::SMALL);

    OLED::printSymbolDeg(FontStyle::SMALL);

    if (OLED::getRotation()) {
      OLED::setCursor(0, 8);
    } else {
      OLED::setCursor(67, 8); // bottom right
    }
    printVoltage(); // draw voltage then symbol (v)
    OLED::print(SmallSymbolVolts, FontStyle::SMALL);
  }
}
void drawSimplifiedHomeScreen(uint32_t tipTemp) {
  bool tempOnDisplay          = false;
  bool tipDisconnectedDisplay = false;
  if (OLED::getRotation()) {
    OLED::drawArea(54, 0, 42, 16, buttonAF);
    OLED::drawArea(12, 0, 42, 16, buttonBF);
    OLED::setCursor(0, 0);
    gui_drawBatteryIcon();
  } else {
    OLED::drawArea(0, 0, 42, 16, buttonA);  // Needs to be flipped so button ends up
    OLED::drawArea(42, 0, 42, 16, buttonB); // on right side of screen
    OLED::setCursor(84, 0);
    gui_drawBatteryIcon();
  }
  tipDisconnectedDisplay = false;
  if (tipTemp > 55) {
    tempOnDisplay = true;
  } else if (tipTemp < 45) {
    tempOnDisplay = false;
  }
  if (isTipDisconnected()) {
    tempOnDisplay          = false;
    tipDisconnectedDisplay = true;
  }
  if (tempOnDisplay || tipDisconnectedDisplay) {
    // draw temp over the start soldering button
    // Location changes on screen rotation
    if (OLED::getRotation()) {
      // in right handed mode we want to draw over the first part
      OLED::fillArea(55, 0, 41, 16, 0); // clear the area for the temp
      OLED::setCursor(56, 0);
    } else {
      OLED::fillArea(0, 0, 41, 16, 0); // clear the area
      OLED::setCursor(0, 0);
    }
    // If we have a tip connected draw the temp, if not we leave it blank
    if (!tipDisconnectedDisplay) {
      // draw in the temp
      if (!(getSettingValue(SettingsOptions::CoolingTempBlink) && (xTaskGetTickCount() % 1000 < 300))) {
        gui_drawTipTemp(false, FontStyle::LARGE); // draw in the temp
      }
    } else {
      // Draw in missing tip symbol

      if (OLED::getRotation()) {
        // in right handed mode we want to draw over the first part
        OLED::drawArea(54, 0, 42, 16, disconnectedTipF);
      } else {
        OLED::drawArea(0, 0, 42, 16, disconnectedTip);
      }
    }
  }
}
OperatingMode drawHomeScreen(const ButtonState buttons, guiContext *cxt) {

  currentTempTargetDegC = 0; // ensure tip is off
  getInputVoltageX10(getSettingValue(SettingsOptions::VoltageDiv), 0);
  uint32_t tipTemp = TipThermoModel::getTipInC();

  // Setup LCD Cursor location
  if (OLED::getRotation()) {
    OLED::setCursor(50, 0);
  } else {
    OLED::setCursor(-1, 0);
  }
  if (getSettingValue(SettingsOptions::DetailedIDLE)) {
    drawDetailedHomeScreen(tipTemp);
  } else {
    drawSimplifiedHomeScreen(tipTemp);
  }
  return handleHomeButtons(buttons, cxt);
}

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/blue_button.h"

#include "grit/ui_resources.h"
#include "ui/base/accessibility/accessible_view_state.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/sys_color_change_listener.h"
#include "ui/views/controls/button/label_button_border.h"

namespace {

const int kBlueNormalImages[] = IMAGE_GRID(IDR_BLUE_BUTTON_NORMAL);
const int kBlueHoveredImages[] = IMAGE_GRID(IDR_BLUE_BUTTON_HOVER);
const int kBluePressedImages[] = IMAGE_GRID(IDR_BLUE_BUTTON_PRESSED);
const int kBlueFocusedNormalImages[] = IMAGE_GRID(
    IDR_BLUE_BUTTON_FOCUSED_NORMAL);
const int kBlueFocusedHoveredImages[] = IMAGE_GRID(
    IDR_BLUE_BUTTON_FOCUSED_HOVER);
const int kBlueFocusedPressedImages[] = IMAGE_GRID(
    IDR_BLUE_BUTTON_FOCUSED_PRESSED);

// Default text and shadow colors for the blue button.
const SkColor kBlueButtonTextColor = SK_ColorWHITE;
const SkColor kBlueButtonShadowColor = SkColorSetRGB(0x53, 0x8C, 0xEA);

}  // namespace

namespace views {

// static
const char BlueButton::kViewClassName[] = "views/BlueButton";

BlueButton::BlueButton(ButtonListener* listener, const string16& text)
    : LabelButton(listener, text) {
  // Inherit STYLE_BUTTON insets, minimum size, alignment, etc.
  SetStyle(STYLE_BUTTON);

  LabelButtonBorder* button_border = static_cast<LabelButtonBorder*>(border());
  button_border->SetPainter(false, STATE_NORMAL,
      Painter::CreateImageGridPainter(kBlueNormalImages));
  button_border->SetPainter(false, STATE_HOVERED,
      Painter::CreateImageGridPainter(kBlueHoveredImages));
  button_border->SetPainter(false, STATE_PRESSED,
      Painter::CreateImageGridPainter(kBluePressedImages));
  button_border->SetPainter(false, STATE_DISABLED,
      Painter::CreateImageGridPainter(kBlueNormalImages));
  button_border->SetPainter(true, STATE_NORMAL,
      Painter::CreateImageGridPainter(kBlueFocusedNormalImages));
  button_border->SetPainter(true, STATE_HOVERED,
      Painter::CreateImageGridPainter(kBlueFocusedHoveredImages));
  button_border->SetPainter(true, STATE_PRESSED,
      Painter::CreateImageGridPainter(kBlueFocusedPressedImages));
  button_border->SetPainter(true, STATE_DISABLED,
      Painter::CreateImageGridPainter(kBlueNormalImages));

  if (!gfx::IsInvertedColorScheme()) {
    for (size_t state = STATE_NORMAL; state < STATE_COUNT; ++state)
      SetTextColor(static_cast<ButtonState>(state), kBlueButtonTextColor);
    label()->SetShadowColors(kBlueButtonShadowColor, kBlueButtonShadowColor);
    label()->SetShadowOffset(0, 1);
  }
}

BlueButton::~BlueButton() {}

const char* BlueButton::GetClassName() const {
  return BlueButton::kViewClassName;
}

// TODO(msw): Re-enable animations for blue buttons. See crbug.com/239121.
const ui::Animation* BlueButton::GetThemeAnimation() const {
  return NULL;
}

}  // namespace views

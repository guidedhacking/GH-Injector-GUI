/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "pch.h"

#include "framelesswindow.h"

void StatusBox(bool ok, const QString & msg);

bool YesNoBox(const QString & title, const QString & msg, QWidget * parent = Q_NULLPTR, QMessageBox::Icon icon = QMessageBox::Icon::Question);
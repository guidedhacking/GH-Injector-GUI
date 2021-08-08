#pragma once

#include "pch.h"

#include "framelesswindow.h"

void ShowStatusbox(bool ok, const QString & msg);

bool YesNoBox(const QString & title, const QString & msg, QWidget * parent = Q_NULLPTR);
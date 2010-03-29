/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2010 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef guiclientinterface_h
#define guiclientinterface_h

#include "parameter.h"

#include <QString>
#include <QAction>

class XTUPLEWIDGETS_EXPORT GuiClientInterface
{
  public:
    virtual ~GuiClientInterface() {}
    virtual QWidget* openWindow(const QString pname, ParameterList pparams, QWidget *parent = 0, Qt::WindowModality modality = Qt::NonModal, Qt::WindowFlags flags = 0) = 0;
    virtual QAction* findAction(const QString pname) = 0;
};

#endif

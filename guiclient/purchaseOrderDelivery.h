/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef PURCHASEORDERDELIVERY_H
#define PURCHASEORDERDELIVERY_H

#include "guiclient.h"
#include "xdialog.h"
#include <QStandardItemModel>
#include <parameter.h>
#include "ui_purchaseOrderDelivery.h"

class purchaseOrderDelivery : public XDialog, public Ui::purchaseOrderDelivery
{
    Q_OBJECT

public:
    purchaseOrderDelivery(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
    ~purchaseOrderDelivery();

  
    virtual void  prepare();
    virtual void  clear();
    Q_INVOKABLE virtual int id() { return _poitemid; }

public slots:
    virtual SetResponse set( const ParameterList & pParams );
    virtual void populate();

protected slots:
    virtual void languageChange();

    virtual void sSave();


private:
    int _itemsrcid;
    int _mode;
    int _poheadid;
    int _poitemid;
    int _parentwo;
    int _parentso;
    int _vendid;
    int _preferredWarehouseid;
    int _taxzoneid;
    double _invVendUOMRatio;
    double _minimumOrder;
    double _orderMultiple;
    double _orderQtyCache;
    double _maxCost;
    bool _overriddenUnitPrice;
    bool _dropship;
    bool _captive;
    QString	_poStatus;
    QString	_costmethod;
    QStandardItemModel * _itemchar;

};

#endif // PURCHASEORDERDELIVERY_H

/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "purchaseOrderDelivery.h"

#include <QMessageBox>
#include <QValidator>
#include <QVariant>
#include <QSqlError>
#include <metasql.h>

#include "errorReporter.h"
#include "guiErrorCheck.h"
#include "mqlutil.h"

purchaseOrderDelivery::purchaseOrderDelivery(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  XSqlQuery purchasepurchaseOrderDelivery;
  setupUi(this);

  _vendid = -1;
  _preferredWarehouseid = -1;
  _invVendUOMRatio = 1;
  _minimumOrder = 0;
  _orderMultiple = 0;
  _maxCost = 0.0;
  _dropship = false;
  _costmethod = "";
  _captive = false;

  connect(_inventoryItem, SIGNAL(toggled(bool)), this, SLOT(sInventoryItemToggled(bool)));
  connect(_item, SIGNAL(privateIdChanged(int)), this, SLOT(sFindWarehouseItemsites(int)));
  connect(_item, SIGNAL(newId(int)), this, SLOT(sPopulateItemInfo(int)));
  connect(_warehouse, SIGNAL(newID(int)), this, SLOT(sPopulateItemsiteInfo()));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));

  _parentwo = -1;
  _parentso = -1;
  _itemsrcid = -1;
  _taxzoneid = -1;   //  _taxzoneid  added //
  _orderQtyCache = -1;

  _ordered->setValidator(omfgThis->qtyVal());
  _received->setValidator(omfgThis->qtyVal());

  //If not multi-warehouse hide whs control
  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
  
  adjustSize();
}

/*
 *  Destroys the object and frees any allocated resources
 */
purchaseOrderDelivery::~purchaseOrderDelivery()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void purchaseOrderDelivery::languageChange()
{
  retranslateUi(this);
}

enum SetResponse purchaseOrderDelivery::set(const ParameterList &pParams)
{
  XSqlQuery purchaseet;
  XDialog::set(pParams);
  QVariant param;
  bool     valid;
  bool     haveQty  = FALSE;
  bool     haveDate = FALSE;



  param = pParams.value("vend_id", &valid);
  if (valid)
    _vendid = param.toInt();

  param = pParams.value("warehous_id", &valid);
  if (valid)
    _preferredWarehouseid = param.toInt();

  param = pParams.value("dropship", &valid);
  if (valid)
    _dropship = param.toBool();

  param = pParams.value("parentWo", &valid);
  if (valid)
    _parentwo = param.toInt();

  param = pParams.value("parentSo", &valid);
  if (valid)
    _parentso = param.toInt();

  param = pParams.value("pohead_id", &valid);
  if (valid)
  {
    _poheadid = param.toInt();

    purchaseet.prepare( "SELECT pohead_taxzone_id, pohead_number, pohead_orderdate, pohead_status, " // pohead_taxzone_id added
               "       vend_id, vend_restrictpurch, pohead_curr_id "
               "FROM pohead, vendinfo "
               "WHERE ( (pohead_vend_id=vend_id)"
               " AND (pohead_id=:pohead_id) );" );
    purchaseet.bindValue(":pohead_id", param.toInt());
    purchaseet.exec();
    if (purchaseet.first())
    {
      _poNumber->setText(purchaseet.value("pohead_number").toString());
      _poStatus = purchaseet.value("pohead_status").toString();
    }
  }

  param = pParams.value("poitem_id", &valid);
  if (valid)
  {
    _poitemid = param.toInt();

    purchaseet.prepare( "SELECT pohead_number, pohead_id "
               "FROM pohead, poitem "
               "WHERE ( (pohead_id=poitem_pohead_id) "
               " AND (poitem_id=:poitem_id) );" );
    purchaseet.bindValue(":poitem_id", param.toInt());
    purchaseet.exec();
    if (purchaseet.first())
    {
      _poNumber->setText(purchaseet.value("pohead_number").toString());
	  _poheadid = purchaseet.value("pohead_id").toInt();
    }

    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _save->setEnabled(false);

      purchaseet.exec("SELECT NEXTVAL('poitem_poitem_id_seq') AS poitem_id;");
      if (purchaseet.first())
        _poitemid = purchaseet.value("poitem_id").toInt();
      else
      {
        systemError(this, tr("A System Error occurred at %1::%2.")
                          .arg(__FILE__)
                          .arg(__LINE__) );
        return UndefinedError;
      }

      purchaseet.prepare( "SELECT (COALESCE(MAX(poitem_linenumber), 0) + 1) AS _linenumber "
                 "FROM poitem "
                 "WHERE (poitem_pohead_id=:pohead_id);" );
      purchaseet.bindValue(":pohead_id", _poheadid);
      purchaseet.exec();
      if (purchaseet.first())
        _lineNumber->setText(purchaseet.value("_linenumber").toString());
      else
      {
        systemError(this, tr("A System Error occurred at %1::%2.")
                          .arg(__FILE__)
                          .arg(__LINE__) );

        return UndefinedError;
      }
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      _typeGroup->setEnabled(FALSE);
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _typeGroup->setEnabled(FALSE);
      _warehouse->setEnabled(FALSE);
      _dueDate->setEnabled(FALSE);
      _ordered->setEnabled(FALSE);

      _close->setText(tr("&Close"));
      _save->hide();
    }
  }

  param = pParams.value("itemsite_id", &valid);
  if (valid)
  {
    _item->setItemsiteid(param.toInt());
    _item->setEnabled(FALSE);
    _warehouse->setEnabled(FALSE);
  }
  
  param = pParams.value("qty", &valid);
  if (valid)
  {
    _ordered->setDouble((param.toDouble()/_invVendUOMRatio));

    haveQty = TRUE;
  }

  param = pParams.value("dueDate", &valid);
  if (valid)
  {
    _dueDate->setDate(param.toDate());
    haveDate = TRUE;
  }

  param = pParams.value("captive", &valid);
  if (valid)
    _captive = true;
  
  return NoError;
}

void purchaseOrderDelivery::populate()
{
  XSqlQuery purchasepopulate;
  MetaSQLQuery mql = mqlLoad("purchaseOrderDeliverys", "detail");

  ParameterList params;
  params.append("poitem_id", _poitemid);
  purchasepopulate = mql.toQuery(params);
  if (purchasepopulate.lastError().type() != QSqlError::NoError)
  {
    systemError(this, purchasepopulate.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  if (purchasepopulate.first())
  {
    _poNumber->setText(purchasepopulate.value("pohead_number").toString());
    _lineNumber->setText(purchasepopulate.value("poitem_linenumber").toString());
    _dueDate->setDate(purchasepopulate.value("poitem_duedate").toDate());
    _ordered->setDouble(purchasepopulate.value("poitem_qty_ordered").toDouble());
    _orderQtyCache = _ordered->toDouble();
    _received->setDouble(purchasepopulate.value("poitem_qty_received").toDouble());

    if(purchasepopulate.value("poitem_order_id") != -1)
    {
      _ordered->setEnabled(FALSE);
      _dueDate->setEnabled(FALSE);
    }

    if (purchasepopulate.value("poitem_itemsite_id").toInt() == -1)
    {
      _nonInventoryItem->setChecked(TRUE);
      _expcat->setId(purchasepopulate.value("poitem_expcat_id").toInt());
      _uom->setText(purchasepopulate.value("poitem_vend_uom").toString());
    }
    else
    {
      _inventoryItem->setChecked(TRUE);
      _item->setItemsiteid(purchasepopulate.value("poitem_itemsite_id").toInt());
    }

    _itemsrcid = purchasepopulate.value("poitem_itemsrc_id").toInt();
    _uom->setText(purchasepopulate.value("poitem_vend_uom").toString());
    _invVendUOMRatio = purchasepopulate.value("poitem_invvenduomratio").toDouble();
  }
}

void purchaseOrderDelivery::prepare()
{
  XSqlQuery prepareq;
  //  Grab the next poitem_id
  prepareq.exec("SELECT NEXTVAL('poitem_poitem_id_seq') AS _poitem_id");
  if (prepareq.first())
  {
    _poitemid = prepareq.value("_poitem_id").toInt();
  }
  else if (prepareq.lastError().type() != QSqlError::NoError)
  {
    systemError(this, prepareq.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
        
  prepareq.prepare( "SELECT (COALESCE(MAX(poitem_linenumber), 0) + 1) AS _linenumber "
                    "FROM poitem "
                    "WHERE (poitem_pohead_id=:pohead_id)" );
  prepareq.bindValue(":pohead_id", _poheadid);
  prepareq.exec();
  if (prepareq.first())
    _lineNumber->setText(prepareq.value("_linenumber").toString());
  else if (prepareq.lastError().type() != QSqlError::NoError)
  {
    systemError(this, prepareq.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void purchaseOrderDelivery::clear()
{
  _poitemid = -1;
  _item->setId(-1);
//  _warehouse->setId(-1);
  _lineNumber->clear();
  _ordered->clear();
  _dueDate->clear();
  _dropship = false;
  _orderQtyCache = -1;
  _itemsrcid = -1;
  _uom->setText(_item->uom());
  _invVendUOMRatio = 1;
  _save->setEnabled(false);
}

void purchaseOrderDelivery::sSave()
{
  QList<GuiErrorCheck> errors;
  errors << GuiErrorCheck(!_inventoryItem->isChecked() && _expcat->id() == -1, _expcat,
                          tr("<p>You must specify an Expense Category for this non-Inventory Item before you may save."))
         << GuiErrorCheck(_inventoryItem->isChecked() && !_item->isValid(), _item,
                          tr("<p>You must select an Item Number before you may save."))
         << GuiErrorCheck(_inventoryItem->isChecked() && _warehouse->id() == -1, _warehouse,
                          tr("<p>You must select a Supplying Site before you may save."))
         << GuiErrorCheck(!_dueDate->isValid(), _dueDate,
                          tr("<p>You must enter a due date before you may save this Purchase Order Item."))
     ;

  if (GuiErrorCheck::reportErrors(this, tr("Cannot Save Purchase Order Item"), errors))
    return;

  XSqlQuery purchaseSave;
  if (_ordered->toDouble() == 0.0)
  {
    if (QMessageBox::critical( this, tr("Zero Order Quantity"),
                               tr( "<p>The quantity that you are ordering is zero. "
                                   "<p>Do you wish to Continue or Change the Order Qty?" ),
                               QString("&Continue"), QString("Change Order &Qty."), QString::null, 1, 1 ) == 1)
    {
      _ordered->setFocus();
      return;
    }
  }

  if (_ordered->toDouble() < _minimumOrder)
  {
    if (QMessageBox::critical( this, tr("Invalid Order Quantity"),
                               tr( "<p>The quantity that you are ordering is below the Minimum Order Quantity for this "
                                   "Item Source.  You may continue but this Vendor may not honor pricing or delivery quotations. "
                                   "<p>Do you wish to Continue or Change the Order Qty?" ),
                               QString("&Continue"), QString("Change Order &Qty."), QString::null, 1, 1 ) == 1)
    {
      _ordered->setFocus();
      return;
    }
  }

  if ((int)_orderMultiple)
  {
    if (qRound(_ordered->toDouble()) % (int)_orderMultiple)
    {
      if (QMessageBox::critical( this, tr("Invalid Order Quantity"),
                                 tr( "<p>The quantity that you are ordering does not fall within the Order Multiple for this "
                                     "Item Source.  You may continue but this Vendor may not honor pricing or delivery quotations. "
                                     "<p>Do you wish to Continue or Change the Order Qty?" ),
                                 QString("&Continue"), QString("Change Order &Qty."), QString::null, 1, 1 ) == 1)
      {
        _ordered->setFocus();
        return;
      }
    }
  }

//  if (_dueDate->date() < _earliestDate->date())
//  {
//    if (QMessageBox::critical( this, tr("Invalid Due Date "),
//                               tr( "<p>The Due Date that you are requesting does not fall within the Lead Time Days for this "
//                                   "Item Source.  You may continue but this Vendor may not honor pricing or delivery quotations "
//                                   "or may not be able to deliver by the requested Due Date. "
//                                   "<p>Do you wish to Continue or Change the Due Date?" ),
//                               QString("&Continue"), QString("Change Order &Due Date"), QString::null, 1, 1 ) == 1)
//    {
//      _dueDate->setFocus();
//      return;
//    }
//  }

  if (_mode == cNew)
  {
    purchaseSave.prepare( "INSERT INTO poitem "
               "( poitem_id, poitem_pohead_id, poitem_status, poitem_linenumber,"
               "  poitem_taxtype_id, poitem_tax_recoverable,"
               "  poitem_itemsite_id, poitem_expcat_id,"
               "  poitem_itemsrc_id, poitem_vend_item_number, poitem_vend_item_descrip,"
               "  poitem_vend_uom, poitem_invvenduomratio,"
               "  poitem_qty_ordered,"
               "  poitem_unitprice, poitem_freight, poitem_duedate, "
               "  poitem_bom_rev_id, poitem_boo_rev_id, "
               "  poitem_comments, poitem_prj_id, poitem_stdcost, poitem_manuf_name, "
               "  poitem_manuf_item_number, poitem_manuf_item_descrip, poitem_rlsd_duedate ) "
               "VALUES "
               "( :poitem_id, :poitem_pohead_id, :status, :poitem_linenumber,"
               "  :poitem_taxtype_id, :poitem_tax_recoverable,"
               "  :poitem_itemsite_id, :poitem_expcat_id,"
               "  :poitem_itemsrc_id, :poitem_vend_item_number, :poitem_vend_item_descrip,"
               "  :poitem_vend_uom, :poitem_invvenduomratio,"
               "  :poitem_qty_ordered,"
               "  :poitem_unitprice, :poitem_freight, :poitem_duedate, "
               "  :poitem_bom_rev_id, :poitem_boo_rev_id, "
               "  :poitem_comments, :poitem_prj_id, stdcost(:item_id), :poitem_manuf_name, "
               "  :poitem_manuf_item_number, :poitem_manuf_item_descrip, :poitem_duedate) ;" );

    purchaseSave.bindValue(":status", _poStatus);
    purchaseSave.bindValue(":item_id", _item->id());

    if (_inventoryItem->isChecked())
    {
      XSqlQuery itemsiteid;
      itemsiteid.prepare( "SELECT itemsite_id "
                          "FROM itemsite "
                          "WHERE ( (itemsite_item_id=:item_id)"
                          " AND (itemsite_warehous_id=:warehous_id) );" );
      itemsiteid.bindValue(":item_id", _item->id());
      itemsiteid.bindValue(":warehous_id", _warehouse->id());
      itemsiteid.exec();
      if (itemsiteid.first())
        purchaseSave.bindValue(":poitem_itemsite_id", itemsiteid.value("itemsite_id").toInt());
      else
      {
        QMessageBox::critical( this, tr("Invalid Item/Site"),
                               tr("<p>The Item and Site you have selected does not appear to be a valid combination. "
                                  "Make sure you have a Site selected and that there is a valid itemsite for "
                                  "this Item and Site combination.") );
        return;
      }
    }
    else
    {
      purchaseSave.bindValue(":poitem_expcat_id", _expcat->id());
    }
  }
  else if (_mode == cEdit)
    purchaseSave.prepare( "UPDATE poitem "
               "SET poitem_itemsrc_id=:poitem_itemsrc_id,"   
               "    poitem_taxtype_id=:poitem_taxtype_id,"
               "    poitem_tax_recoverable=:poitem_tax_recoverable,"
               "    poitem_vend_item_number=:poitem_vend_item_number,"
               "    poitem_vend_item_descrip=:poitem_vend_item_descrip,"
               "    poitem_vend_uom=:poitem_vend_uom, poitem_invvenduomratio=:poitem_invvenduomratio,"
               "    poitem_qty_ordered=:poitem_qty_ordered, poitem_unitprice=:poitem_unitprice,"
               "    poitem_freight=:poitem_freight,"
               "    poitem_duedate=:poitem_duedate, poitem_comments=:poitem_comments,"
               "    poitem_prj_id=:poitem_prj_id, "
               "    poitem_bom_rev_id=:poitem_bom_rev_id, "
               "    poitem_boo_rev_id=:poitem_boo_rev_id, "
               "    poitem_manuf_name=:poitem_manuf_name, "
               "    poitem_manuf_item_number=:poitem_manuf_item_number, "
               "    poitem_manuf_item_descrip=:poitem_manuf_item_descrip "
               "WHERE (poitem_id=:poitem_id);" );

  purchaseSave.bindValue(":poitem_id", _poitemid);
  purchaseSave.bindValue(":poitem_pohead_id", _poheadid);
  purchaseSave.bindValue(":poitem_linenumber", _lineNumber->text().toInt());
  if (_itemsrcid != -1)
    purchaseSave.bindValue(":poitem_itemsrc_id", _itemsrcid);
  purchaseSave.bindValue(":poitem_qty_ordered", _ordered->toDouble());
  purchaseSave.bindValue(":poitem_duedate", _dueDate->date());
  purchaseSave.exec();
  if (purchaseSave.lastError().type() != QSqlError::NoError)
  {
    systemError(this, purchaseSave.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (cNew == _mode && !_captive)
  {
    clear();
    prepare();
    _item->setFocus();
  }
  else
    done(_poitemid);
}

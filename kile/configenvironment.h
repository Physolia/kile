/***************************************************************************
                           configenvironment.h
----------------------------------------------------------------------------
    date                 : Feb 09 2004
    version              : 0.10.0
    copyright            : (C) 2004 by Holger Danielsson
    email                : holger.danielsson@t-online.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONFIGENVIRONMENT_H
#define CONFIGENVIRONMENT_H

#include <kconfig.h>
#include <kdialogbase.h>

#include <qlistbox.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qmap.h>


/**
  *@author Holger Danielsson
  */

class ConfigEnvironment : public QWidget
{
    Q_OBJECT
public: 
   ConfigEnvironment(QWidget *parent=0, const char *name=0);
   ~ConfigEnvironment() {}

   void readConfig(KConfig *config);
   void writeConfig(KConfig *config);

private:
   QRadioButton *rb_listenv, *rb_mathenv, *rb_tabularenv;
   QPushButton *pb_add, *pb_remove;
   QListBox *listbox;

   QMap<QString,bool> m_dictenvlatex;
   QMap<QString,bool> m_dictenvlist;
   QMap<QString,bool> m_dictenvmath;
   QMap<QString,bool> m_dictenvtab;

   QStringList envlist,envmath,envtab;

   void fillListbox(const QMap<QString,bool> *map);
   void setEnvironments(const QStringList &list, QMap<QString,bool> &map);
   QStringList getEnvironments(const QMap<QString,bool> &map);
   QMap<QString,bool> *getDictionary();

private slots:
  void clickedEnvtype();
  void highlightedListbox(int index);
  void clickedAdd();
  void clickedRemove();
};

#endif

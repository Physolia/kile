/***************************************************************************
                          usertooldialog.h  -  description
                             -------------------
    begin                : mer avr 9 2003
    copyright            : (C) 2003 by Pascal Brachet
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef USERTOOLDIALOG_H
#define USERTOOLDIALOG_H

#include <qdialog.h>
#include <qstring.h>

class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;

typedef QString userCd[5];

/**
  *@author Pascal Brachet
  */

class usertooldialog : public QDialog  {
   Q_OBJECT
public:
	usertooldialog(QWidget *parent=0, const char *name=0, const QString &caption = QString::null);
	~usertooldialog();

      userCd Name,Tool;

private:
    int previous_index;
    QLineEdit *itemedit, *tooledit;
    QComboBox *combo1;
    QLabel* label1;
    QLabel* label2;
	  QPushButton *buttonOk;
	  QPushButton *buttonCancel;

public slots:
    void init();

private slots:
    void change(int index);
    void slotOk();
};

#endif

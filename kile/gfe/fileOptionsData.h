/**********************************************************************

	--- Qt Architect generated file ---

	File: fileOptionsData.h

    This file is part of Xgfe: X Windows GUI front end to Gnuplot
    Copyright (C) 1998 David Ishee

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.       

 *********************************************************************/

#ifndef fileOptionsData_included
#define fileOptionsData_included

#include <qdialog.h>
#include <qlineedit.h>
#include <qcombobox.h>

class fileOptionsData : public QDialog
{
    Q_OBJECT

public:

    fileOptionsData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~fileOptionsData();

public slots:


protected slots:

    virtual void setFormat();

protected:
    QLineEdit* dataSetStartEdit;
    QLineEdit* dataSetEndEdit;
    QLineEdit* dataSetIncEdit;
    QLineEdit* pointIncEdit;
    QLineEdit* lineIncEdit;
    QLineEdit* startPointEdit;
    QLineEdit* startLineEdit;
    QLineEdit* endPointEdit;
    QLineEdit* endLineEdit;
    QLineEdit* xColumnEdit;
    QLineEdit* yColumnEdit;
    QLineEdit* zColumnEdit;
    QLineEdit* formatEdit;
    QLineEdit* rawFormatEdit;
    QComboBox* interpList;

};

#endif // fileOptionsData_included

/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/


void KileWidgetLatexConfig::init() 
{ 
	int w = m_tlResolution->sizeHint().width();      
	
	int wi = m_tlType->sizeHint().width();  
	if ( wi > w ) w = wi;
	
	wi = m_tlPath->sizeHint().width(); 
	if ( wi > w ) w = wi;
	
	wi = m_pbCommands->sizeHint().width(); 
	if ( wi > w ) w = wi;
	
	w += 8;
	m_tlResolution->setFixedWidth(w);    
	m_tlType->setFixedWidth(w);    
	m_tlPath->setFixedWidth(w);     
	m_pbCommands->setFixedWidth(w);     
}


int KileWidgetLatexConfig::getDoubleQuotes()
{
    return kcfg_DoubleQuotes->currentItem();
}


void KileWidgetLatexConfig::setDoubleQuotes(int index)
{
    kcfg_DoubleQuotes->setCurrentItem(index);
}

void KileWidgetLatexConfig::slotConfigure()
{
    KileDialog::LatexCommandsDialog *dlg = new KileDialog::LatexCommandsDialog(m_config,m_commands,this);
    dlg->exec();
    delete dlg;
}


void KileWidgetLatexConfig::setLatexCommands( KConfig *config, KileDocument::LatexCommands *commands )
{
    m_config = config;
    m_commands = commands;
}


bool KileWidgetLatexConfig::getInsertDoubleQuotes()
{
   return kcfg_InsertDoubleQuotes->isChecked();
}

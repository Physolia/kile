//
// C++ Implementation: kiledocmanager
//
// Description:
//
//
// Author: Jeroen Wijnhout <Jeroen.Wijnhout@kdemail.net>, (C) 2004
//

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qtextcodec.h>
#include <qfile.h>

#include <kate/document.h>
#include <kate/view.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kprogress.h>
#include <kfile.h>
#include <kfiledialog.h>
#include <krun.h>
#include <kstandarddirs.h>

#include "templates.h"
#include "newfilewizard.h"
#include "managetemplatesdialog.h"
#include "kileinfo.h"
#include "kileproject.h"
#include "kiledocumentinfo.h"
#include "kiledocmanager.h"
#include "kileviewmanager.h"
#include "kilefileselect.h"
#include "kileprojectview.h"
#include "kilestructurewidget.h"
#include "kileprojectdlgs.h"
#include "kiletool.h"
#include "kiletool_enums.h"
#include "kilestdtools.h"
#include "kilelistselector.h"
#include "kiletoolmanager.h"
#include "kileautosavejob.h"
#include "kilekonsolewidget.h"
#include "kileconfig.h"
#include "kilelogwidget.h"
#include "cleandialog.h"

namespace KileDocument
{

Manager::Manager(KileInfo *info, QObject *parent, const char *name) :
	QObject(parent, name),
	m_ki(info)
{
	m_infoList.setAutoDelete(false);
	Kate::Document::setFileChangedDialogsActivated (true);
}


Manager::~Manager()
{
	kdDebug() << "==KileDocument::Manager::~Manager()=========" << endl;
}

void Manager::trashDoc(Info *docinfo, Kate::Document *doc /*= 0L*/ )
{
	kdDebug() << "==void Manager::trashDoc(" << docinfo->url().path() << ")=====" << endl;
	
	if ( m_ki->isOpen(docinfo->url()) ) return;

	if ( doc == 0L ) doc = docinfo->getDoc();
	//look for doc before we detach the docinfo
	//if we do it the other way around, docFor will always return nil
 	if ( doc == 0L ) doc = docFor(docinfo->url());
	
	kdDebug() << "DETACHING " << docinfo << endl;
	docinfo->detach();
	
	kdDebug() << "\tTRASHING " <<  doc  << endl;
	if ( doc == 0L ) return;

	kdDebug() << "just checking: docinfo->getDoc() =  " << docinfo->getDoc() << endl;
	kdDebug() << "just checking: docFor(docinfo->url()) = " << docFor(docinfo->url()) << endl;

	for ( uint i = 0; i < m_infoList.count(); i++ )
	{
		if ( (m_infoList.at(i) != docinfo) && (m_infoList.at(i)->getDoc() == doc) )
		{
			KMessageBox::information(0, i18n("The internal structure of Kile is corrupted (probably due to a bug in Kile). Please select Save All from the File menu and close Kile.\nThe Kile team apologizes for any inconvenience and would appreciate a bug report."));
			kdWarning() << "docinfo " << m_infoList.at(i) << " url " << m_infoList.at(i)->url().fileName() << " has a wild pointer!!!"<< endl;
		}
	}

	kdDebug() << "DELETING doc" << endl;
	delete doc;
}

Kate::Document* Manager::docFor(const KURL & url)
{
	for (uint i=0; i < m_infoList.count(); i++)
	{
		if (m_ki->similarOrEqualURL(m_infoList.at(i)->url(),url))
			return m_infoList.at(i)->getDoc();
	}

	return 0L;
}

Info* Manager::getInfo() const
{
	Kate::Document *doc = m_ki->activeDocument();
	if ( doc != 0L )
		return infoFor(doc);
	else
		return 0L;
}

Info *Manager::infoFor(const QString & path) const
{
	kdDebug() << "==KileInfo::infoFor(" << path << ")==========================" << endl;
	QPtrListIterator<Info> it(m_infoList);
	while ( true )
	{
		if ( it.current()->url().path() == path)
			return it.current();

		if (it.atLast()) break;

		++it;
	}

	kdDebug() << "\tCOULD NOT find info for " << path << endl;
	return 0L;
}

Info* Manager::infoFor(Kate::Document* doc, bool usepath /*= true*/ ) const
{
	if (usepath)
		return infoFor(doc->url().path());
	else
	{
		QPtrListIterator<Info> it(m_infoList);
		while ( true )
		{
			if ( it.current()->getDoc() == doc)
				return it.current();
	
			if (it.atLast()) return 0L;
	
			++it;
		}
	}
}

void Manager::mapItem(Info *docinfo, KileProjectItem *item)
{
	item->setInfo(docinfo);
}

KileProject* Manager::projectFor(const KURL &projecturl)
{
	KileProject *project = 0;

	//find project with url = projecturl
	QPtrListIterator<KileProject> it(m_projects);
	while ( it.current() )
	{
		if ((*it)->url() == projecturl)
		{
			return *it;
		}
		++it;
	}

	return project;
}

KileProject* Manager::projectFor(const QString &name)
{
	KileProject *project = 0;

	//find project with url = projecturl
	QPtrListIterator<KileProject> it(m_projects);
	while ( it.current() )
	{
		if ((*it)->name() == name)
		{
			return *it;
		}
		++it;
	}

	return project;
}

KileProjectItem* Manager::itemFor(const KURL &url, KileProject *project /*=0L*/) const
{
	if (project == 0L)
	{
		QPtrListIterator<KileProject> it(m_projects);
		while ( it.current() )
		{
			kdDebug() << "looking in project " << (*it)->name() << endl;
			if ((*it)->contains(url))
			{
				kdDebug() << "\t\tfound!" << endl;
				return (*it)->item(url);
			}
			++it;
		}
		kdDebug() << "\t nothing found" << endl;
	}
	else
	{
		if ( project->contains(url) )
			return project->item(url);
	}

	return 0L;
}

KileProjectItem* Manager::itemFor(Info *docinfo, KileProject *project /*=0*/) const
{
	return itemFor(docinfo->url(), project);
}

KileProjectItemList* Manager::itemsFor(Info *docinfo) const
{
	kdDebug() << "==KileInfo::itemsFor(" << docinfo->url().fileName() << ")============" << endl;
	KileProjectItemList *list = new KileProjectItemList();
	list->setAutoDelete(false);

	QPtrListIterator<KileProject> it(m_projects);
	while ( it.current() )
	{
		kdDebug() << "\tproject: " << (*it)->name() << endl;
		if ((*it)->contains(docinfo->url()))
		{
			kdDebug() << "\t\tcontains" << endl;
			list->append((*it)->item(docinfo->url()));
		}
		++it;
	}

	return list;
}

KileProject* Manager::activeProject()
{
	KileProject *curpr=0;
	Kate::Document *doc = m_ki->activeDocument();

	if (doc)
	{
		for (uint i=0; i < m_projects.count(); i++)
		{
			if (m_projects.at(i)->contains(doc->url()) )
			{
				curpr = m_projects.at(i);
				break;
			}
		}
	}

	return curpr;
}

KileProjectItem* Manager::activeProjectItem()
{
	KileProject *curpr = activeProject();
	Kate::Document *doc = m_ki->activeDocument();
	KileProjectItem *item = 0;

	if (curpr && doc)
	{
		KileProjectItemList *list = curpr->items();

		for (uint i=0; i < list->count(); i++)
		{
			if (list->at(i)->url() == doc->url())
			{
				item = list->at(i);
				break;
			}
		}
	}

	return item;
}

Info* Manager::createDocumentInfo(const KURL & url)
{
	Info *docinfo = 0L;

	//see if this file belongs to an opened project
	KileProjectItem *item = itemFor(url);
	if ( item != 0L ) docinfo = item->getInfo();

	if ( docinfo == 0L )
	{
		if ( Info::isTeXFile(url) || url.isEmpty() )
		{
			kdDebug() << "CREATING TeXInfo for " << url.url() << endl;
			docinfo = new TeXInfo(0L);
		}
		else if ( Info::isBibFile(url) )
		{
			kdDebug() << "CREATING BibInfo for " << url.url() << endl;
			docinfo = new BibInfo(0L);
		}
		else
		{
			kdDebug() << "CREATING Info for " << url.url() << endl;
			docinfo = new Info(0L);
		}
		docinfo->setURL(url);
		emit(documentInfoCreated(docinfo));
		m_infoList.append(docinfo);
	}

	kdDebug() << "DOCINFO: returning " << docinfo << " " << docinfo->url().fileName() << endl;
	return docinfo;
}

Info* Manager::recreateDocInfo(Info *oldinfo, const KURL & url)
{
	KileProjectItemList *list = itemsFor(oldinfo);
	Info *newinfo = createDocumentInfo(url);
	newinfo->setDoc(oldinfo->getDoc());

	KileProjectItem *pritem = 0L;
	for ( pritem = list->first(); pritem; pritem = list->next() )
		pritem->setInfo(newinfo);
	
	m_infoList.removeRef(oldinfo);
	delete oldinfo;

	m_infoList.append(newinfo);
	return newinfo;
}

bool Manager::removeDocumentInfo(Info *docinfo, bool closingproject /* = false */)
{
	kdDebug() << "==Manager::removeDocumentInfo(Info *docinfo)=====" << endl;
	KileProjectItemList *itms = itemsFor(docinfo);

	if ( itms->count() == 0 || (closingproject && itms->count() == 1))
	{
		kdDebug() << "\tremoving " << docinfo <<  " count = " << m_infoList.count() << endl;
		m_infoList.remove(docinfo);

		emit ( closingDocument ( docinfo ) );

		delete docinfo;
		delete itms;

		return true;
	}

	kdDebug() << "\tnot removing " << docinfo << endl;
	delete itms;
	return false;
}

Kate::Document* Manager::createDocument(Info *docinfo, const QString & encoding, const QString & highlight)
{
	kdDebug() << "==Kate::Document* Manager::createDocument()===========" << endl;

	Kate::Document *doc = (Kate::Document*) KTextEditor::createDocument ("libkatepart", this, "Kate::Document");
	if ( docFor(docinfo->url()) != 0L )
		kdFatal() << docinfo->url().path() << " already has a document!" << endl;
	else
		kdDebug() << "\tappending document " <<  doc << endl;

	//set the default encoding
	QString enc = encoding.isNull() ? QString::fromLatin1(QTextCodec::codecForLocale()->name()) : encoding;
	m_ki->fileSelector()->comboEncoding()->lineEdit()->setText(enc);
	doc->setEncoding(encoding);

	kdDebug() << "opening url: " << docinfo->url().path() << endl;

	doc->openURL(docinfo->url());
	//TODO: connect to completed() signal, now updatestructure is called before loading is completed

	if ( !docinfo->url().isEmpty() )
	{
		doc->setDocName(docinfo->url().path());
		emit(addToRecentFiles(docinfo->url()));
	}
	else
		doc->setDocName(i18n("Untitled"));

	setHighlightMode(doc, highlight);

	//handle changes of the document
	connect(doc, SIGNAL(nameChanged(Kate::Document *)), docinfo, SLOT(emitNameChanged(Kate::Document *)));
	//why not connect doc->nameChanged directly ot this->slotNameChanged ? : the function emitNameChanged
	//updates the docinfo, on which all decisions are bases in slotNameChanged
	connect(docinfo,SIGNAL(nameChanged(Kate::Document*)), this, SLOT(slotNameChanged(Kate::Document*)));
	connect(docinfo, SIGNAL(nameChanged(Kate::Document *)), m_ki->parentWidget(), SLOT(newCaption()));
	connect(doc, SIGNAL(modStateChanged(Kate::Document*)), this, SLOT(newDocumentStatus(Kate::Document*)));
	connect(doc, SIGNAL(modifiedOnDisc(Kate::Document*, bool, unsigned char)), this, SIGNAL(documentStatusChanged(Kate::Document*, bool, unsigned char)));

	docinfo->setDoc(doc);

	kdDebug() << "createDocument: SANITY check: " << (docinfo->getDoc() == docFor(docinfo->url())) << endl;
	return doc;
}

void Manager::setHighlightMode(Kate::Document * doc, const QString &highlight)
{
	kdDebug() << "==Kile::setHighlightMode(" << doc->docName() << "," << highlight << " )==================" << endl;

	int c = doc->hlModeCount();
	int nHlLaTeX = 0;
	//determine default highlighting mode (LaTeX)
	for (int i = 0; i < c; i++)
	{
		if (doc->hlModeName(i) == "LaTeX") { nHlLaTeX = i; break; }
	}

	//don't let KatePart determine the highlighting
	if ( highlight != QString::null )
	{
		bool found = false;
		int mode = 0;
		for (int i = 0; i < c; i++)
		{
			if (doc->hlModeName(i) == highlight) { found = true; mode = i; }
		}
		if (found) doc->setHlMode(mode);
		else doc->setHlMode(nHlLaTeX);
	}
	else if ( doc->url().isEmpty() || doc->docName() == i18n("Untitled") )
	{
		doc->setHlMode(nHlLaTeX);
	}
}

Kate::View* Manager::loadItem(KileProjectItem *item, const QString & text)
{
	Kate::View *view = 0L;

	kdDebug() << "==loadItem(" << item->url().path() << ")======" << endl;

	if ( item->type() != KileProjectItem::Image )
	{
		view = load(item->url(), item->encoding(), item->isOpen(), item->highlight(), text);
		kdDebug() << "\tloadItem: docfor = " << docFor(item->url().path()) << endl;

		Info *docinfo = infoFor(item->url().path());
		item->setInfo(docinfo);

		kdDebug() << "\tloadItem: docinfo = " << docinfo << " doc = " << docinfo->getDoc() << " docfor = " << docFor(docinfo->url().path()) << endl;
		if ( docinfo->getDoc() != docFor(docinfo->url().path()) ) kdFatal() << "docinfo->getDoc() != docFor()" << endl;
	}
	else
	{
		kdDebug() << "\tloadItem: no document generated" << endl;
		Info *docinfo = createDocumentInfo(item->url());
		item->setInfo(docinfo);

		if ( docFor(item->url()) == 0L)
		{
			docinfo->detach();
			kdDebug() << "\t\t\tdetached" << endl;
		}
	}

	return view;
}

Kate::View* Manager::load(const KURL &url , const QString & encoding /* = QString::null */, bool create /* = true */, const QString & highlight /* = QString::null */, const QString & text /* = QString::null */)
{
	kdDebug() << "==load()=================" << endl;
	//if doc already opened, update the structure view and return the view
	if ( url.path() != i18n("Untitled") && m_ki->isOpen(url))
		return m_ki->viewManager()->switchToView(url);

	Info *docinfo = createDocumentInfo(url);
	Kate::Document *doc = createDocument(docinfo, encoding, highlight);

	m_ki->structureWidget()->clean(docinfo);
	docinfo->updateStruct();

	if ( text != QString::null ) doc->setText(text);

	//FIXME: use signal/slot
	if (doc && create)
		return m_ki->viewManager()->createView(doc);

	kdDebug() << "just after createView()" << endl;
	kdDebug() << "\tdocinfo = " << docinfo << " doc = " << docinfo->getDoc() << " docfor = " << docFor(docinfo->url().path()) << endl;

	return 0L;
}

//FIXME: template stuff should be in own class
Kate::View* Manager::loadTemplate(TemplateItem *sel)
{
	QString text = QString::null;

	if (sel && sel->name() != DEFAULT_EMPTY_CAPTION)
	{
		//create a new document to open the template in
		Kate::Document *tempdoc = (Kate::Document*) KTextEditor::createDocument ("libkatepart", this, "Kate::Document");

		if (!tempdoc->openURL(KURL(sel->path())))
		{
			KMessageBox::error(m_ki->parentWidget(), i18n("Could not find template: %1").arg(sel->name()),i18n("File Not Found"));
		}
		else
		{
			//substitute templates variables
			text = tempdoc->text();
			delete tempdoc;
			replaceTemplateVariables(text);
		}
	}

	return createDocumentWithText(text);
}

Kate::View* Manager::createDocumentWithText(const QString & text)
{
	Kate::View *view = load(KURL(), QString::null, true, QString::null, text);
	if (view)
	{
		//FIXME this shouldn't be necessary!!!
		view->getDoc()->setModified(true);
		newDocumentStatus(view->getDoc());
	}

	return view;
}

void Manager::replaceTemplateVariables(QString &line)
{
	line=line.replace("$$AUTHOR$$", KileConfig::author());
	line=line.replace("$$DOCUMENTCLASSOPTIONS$$", KileConfig::documentClassOptions());
	if (KileConfig::templateEncoding() != "") { line=line.replace("$$INPUTENCODING$$", "\\usepackage["+ KileConfig::templateEncoding() +"]{inputenc}");}
	else { line = line.replace("$$INPUTENCODING$$","");}
}

void Manager::createTemplate()
{
	if (m_ki->viewManager()->currentView())
	{
		if (m_ki->viewManager()->currentView()->getDoc()->isModified() )
		{
			KMessageBox::information(m_ki->parentWidget(),i18n("Please save the file first."));
			return;
		}
	}
	else
	{
		KMessageBox::information(m_ki->parentWidget(),i18n("Open/create a document first."));
		return;
	}

	QFileInfo fi(m_ki->viewManager()->currentView()->getDoc()->url().path());
	ManageTemplatesDialog mtd(&fi,i18n("Create Template From Document"));
	mtd.exec();
}

void Manager::removeTemplate()
{
	ManageTemplatesDialog mtd(i18n("Remove Template"));
	mtd.exec();
}

void Manager::fileNew()
{
	NewFileWizard *nfw = new NewFileWizard(m_ki->parentWidget());
	if (nfw->exec())
	{
		loadTemplate(nfw->getSelection());
		if ( nfw->useWizard() ) emit ( startWizard() );
		emit(updateStructure(false, 0L));
	}
	delete nfw;
}

void Manager::fileNew(const KURL & url)
{
	//create an empty file
	QFile file(url.path());
	file.open(IO_ReadWrite);
	file.close();

	fileOpen(url, QString::null);
}

void Manager::fileOpen()
{
	//determine the starting dir for the file dialog
	QString currentDir= m_ki->fileSelector()->dirOperator()->url().path();
	QString filter;
	QFileInfo fi;
	if (m_ki->viewManager()->currentView())
	{
		fi.setFile(m_ki->viewManager()->currentView()->getDoc()->url().path());
		if (fi.exists()) currentDir= fi.dirPath();
	}

	//get the URLs
	filter.append(SOURCE_EXTENSIONS);
	filter.append(" ");
	filter.append(PACKAGE_EXTENSIONS);
	filter.replace(".","*.");
	filter.append("|");
	filter.append(i18n("TeX Files"));
	filter.append("\n*|");
	filter.append(i18n("All Files"));
	kdDebug() << "using FILTER " << filter << endl;
	KURL::List urls = KFileDialog::getOpenURLs( currentDir, filter, m_ki->parentWidget(), i18n("Open Files") );

	//open them
	for (uint i=0; i < urls.count(); i++)
		fileOpen(urls[i]);
}

void Manager::fileSelected(const KFileItem *file)
{
	fileSelected(file->url());
}

void Manager::fileSelected(const KileProjectItem * item)
{
	fileOpen(item->url(), item->encoding());
}

void Manager::fileSelected(const KURL & url)
{
	fileOpen(url, m_ki->fileSelector()->comboEncoding()->lineEdit()->text());
}

void Manager::saveURL(const KURL & url)
{
	Kate::Document *doc = docFor(url);
	if (doc) doc->save();
}

void Manager::slotNameChanged(Kate::Document * doc)
{
	kdDebug() << "==Kile::slotNameChanged==========================" << endl;

	KURL validURL = Info::makeValidTeXURL(doc->url());
	if(validURL != doc->url()) 
	{
		QFile oldFile(doc->url().path());
		if(doc->saveAs(validURL))
			oldFile.remove();

		m_ki->viewManager()->projectView()->add(doc->url());
	}

	emit(documentStatusChanged(doc, doc->isModified(), 0));

	Info *docinfo = infoFor(doc);

	//add to project view if doc was Untitled before
	if (docinfo->oldURL().isEmpty())
	{
		kdDebug() << "\tadding URL to projectview " << doc->url().path() << endl;
		m_ki->viewManager()->projectView()->add(doc->url());
		
		//hack: by default a TeX docinfo is created, if a newly created file is save as
		//a .bib file, recreate the docinfo (now as a BibTeX docinfo)
		if (doc->url().path().endsWith(".bib")) recreateDocInfo(docinfo, doc->url());
	}
}

void Manager::newDocumentStatus(Kate::Document *doc)
{
	if (doc == 0L) return;

	//sync terminal
	m_ki->texKonsole()->sync();

	emit(documentStatusChanged(doc,  doc->isModified(), 0));

	//updatestructure if active document changed from modified to unmodified (typically after a save)
	if (!doc->isModified())
		emit(updateStructure(true, infoFor(doc)));
}

void Manager::fileSaveAll(bool amAutoSaving)
{
	Kate::View *view;
	QFileInfo fi;

	kdDebug() << "==Kile::fileSaveAll=================" << endl;
	kdDebug() << "\tautosaving = " << amAutoSaving << endl;

	for (uint i = 0; i < m_ki->viewManager()->views().count(); i++)
	{
		view = m_ki->viewManager()->view(i);

		if (view && (view->getDoc()->isModified() || amAutoSaving) )
		{
			fi.setFile(view->getDoc()->url().path());
			//don't save unwritable and untitled documents when autosaving
			if (
			      (!amAutoSaving) ||
				  (amAutoSaving && (!view->getDoc()->url().isEmpty() ) && fi.isWritable() )
			   )
			{
				kdDebug() << "\tsaving: " << view->getDoc()->url().path() << endl;

				if (amAutoSaving)
				{
					//make a backup
					KURL url = view->getDoc()->url();
					KileAutoSaveJob *job = new KileAutoSaveJob(url);

					//save the current file if job is finished succesfully
					if (view->getDoc()->isModified()) connect(job, SIGNAL(success()), view, SLOT(save()));
				}
				else
					view->save();
			}
		}
	}
}

void Manager::fileOpen(const KURL & url, const QString & encoding)
{
	kdDebug() << "==Kile::fileOpen==========================" << endl;
	kdDebug() << "\t" << url.url() << endl;
	bool isopen = m_ki->isOpen(url);

	load(url, encoding);

	//URL wasn't open before loading, add it to the project view
	//FIXME: use signal/slot
	if (!isopen && (itemFor(url) == 0) ) m_ki->viewManager()->projectView()->add(url);

	emit(updateStructure(false, 0L));
	emit(updateModeStatus());
}

bool Manager::fileCloseAll()
{
	Kate::View * view = m_ki->viewManager()->currentView();

	//assumes one view per doc here
	while( ! m_ki->viewManager()->views().isEmpty() )
	{
		view = m_ki->viewManager()->view(0);
		if (! fileClose(view->getDoc())) return false;
	}

	return true;
}

bool Manager::fileClose(const KURL & url)
{
	Kate::Document *doc = docFor(url);
	if ( doc == 0L )
		return true;
	else
		return fileClose(doc);
}

bool Manager::fileClose(Kate::Document *doc /* = 0L*/, bool closingproject /*= false*/)
{
	kdDebug() << "==Kile::fileClose==========================" << endl;

	if (doc == 0L)
		doc = m_ki->activeDocument();

	if (doc == 0L)
		return true;
	else
	//FIXME: remove from docinfo map, remove from dirwatch
	{
		kdDebug() << "\t" << doc->url().path() << endl;
		QString fn = doc->url().fileName();

		KURL url = doc->url();

		Info *docinfo= infoFor(doc);
		KileProjectItemList *items = itemsFor(docinfo);

		while ( items->current() )
		{
			//FIXME: refactor here
 			if (items->current() && doc) storeProjectItem(items->current(),doc);
			items->next();
		}

		delete items;

		if ( doc->closeURL() )
		{
			if ( KileConfig::cleanUpAfterClose() ) cleanUpTempFiles(docinfo, true);
			
			//FIXME: use signal/slot
			m_ki->viewManager()->removeView(static_cast<Kate::View*>(doc->views().first()));
			//remove the decorations

			trashDoc(docinfo, doc);
            m_ki->structureWidget()->clean(docinfo);
			removeDocumentInfo(docinfo, closingproject);

			//FIXME:remove entry in projectview
			m_ki->viewManager()->removeFromProjectView(url);
		}
		else
			return false;
	}

	return true;
}

void Manager::buildProjectTree(const KURL & url)
{
	KileProject * project = projectFor(url);

	if (project)
		buildProjectTree(project);
}

void Manager::buildProjectTree(KileProject *project)
{
	if (project == 0)
		project = activeProject();

	if (project == 0 )
		project = selectProject(i18n("Refresh Project Tree"));

	if (project)
	{
		//TODO: update structure for all docs
		project->buildProjectTree();
	}
	else if (m_projects.count() == 0)
		KMessageBox::error(m_ki->parentWidget(), i18n("The current document is not associated to a project. Please activate a document that is associated to the project you want to build the tree for, then choose Refresh Project Tree again."),i18n( "Could Not Refresh Project Tree"));
}

void Manager::projectNew()
{
	kdDebug() << "==Kile::projectNew==========================" << endl;
	KileNewProjectDlg *dlg = new KileNewProjectDlg(m_ki->parentWidget());
	kdDebug()<< "\tdialog created" << endl;

	if (dlg->exec())
	{
		kdDebug()<< "\tdialog executed" << endl;
		kdDebug() << "\t" << dlg->name() << " " << dlg->location() << endl;

		KileProject *project = dlg->project();

		//add the project file to the project
		//TODO: shell expand the filename
		KileProjectItem *item = new KileProjectItem(project, project->url());
		item->setOpenState(false);
		projectOpenItem(item);

		if (dlg->createNewFile())
		{
			QString filename = dlg->file();
			KURL fileURL; fileURL.setFileName(filename);
			if(KileDocument::Info::containsInvalidCharacters(fileURL)) {
				KURL newURL = KileDocument::Info::repairInvalidCharacters(fileURL);
				filename = newURL.fileName();
			}

			//create the new document and fill it with the template
			//TODO: shell expand the filename
			Kate::View *view = loadTemplate(dlg->getSelection());

			//derive the URL from the base url of the project
			KURL url = project->baseURL();
			url.addPath(filename);

			Info *docinfo = infoFor(view->getDoc());
			docinfo->setURL(url);

			//save the new file
			view->getDoc()->saveAs(url);

			//add this file to the project
			item = new KileProjectItem(project, url);
			//project->add(item);
			mapItem(docinfo, item);

			//docinfo->updateStruct(m_kwStructure->level());
			emit(updateStructure(true, docinfo));
		}

		project->buildProjectTree();
		//project->save();
		addProject(project);
	}
}

void Manager::addProject(const KileProject *project)
{
	kdDebug() << "==void Manager::addProject(const KileProject *project)==========" << endl;
	m_projects.append(project);
	kdDebug() << "\tnow " << m_projects.count() << " projects" << endl;
	m_ki->viewManager()->projectView()->add(project);
	connect(project, SIGNAL(projectTreeChanged(const KileProject *)), this, SIGNAL(projectTreeChanged(const KileProject *)));
}

KileProject* Manager::selectProject(const QString& caption)
{
	QStringList list;
	QPtrListIterator<KileProject> it(m_projects);
	while (it.current())
	{
		list.append((*it)->name());
		++it;
	}

	KileProject *project = 0;
	QString name = QString::null;
	if (list.count() > 1)
	{
		KileListSelector *dlg  = new KileListSelector(list, caption, i18n("Select Project"), m_ki->parentWidget());
		if (dlg->exec())
		{
			name = list[dlg->currentItem()];
		}
	}
	else if (list.count() == 0)
	{
		return 0;
	}
	else
		name = m_projects.first()->name();

	project = projectFor(name);

	return project;
}

void Manager::addToProject(const KURL & url)
{
	kdDebug() << "==Kile::addToProject==========================" << endl;
	kdDebug() << "\t" <<  url.fileName() << endl;

	KileProject *project = selectProject(i18n("Add to Project"));

	if (project) addToProject(project, url);
}

void Manager::addToProject(KileProject* project, const KURL & url)
{
	if (project->contains(url)) return;

	KileProjectItem *item = new KileProjectItem(project, url);
	item->setOpenState(m_ki->isOpen(url));
	projectOpenItem(item);
	m_ki->viewManager()->projectView()->add(item);
	buildProjectTree(project);
}

void Manager::removeFromProject(const KileProjectItem *item)
{
	if (item->project())
	{
		kdDebug() << "\tprojecturl = " << item->project()->url().path() << ", url = " << item->url().path() << endl;

		if (item->project()->url() == item->url())
		{
			KMessageBox::error(m_ki->parentWidget(), i18n("This file is the project file, it holds all the information about your project. Therefore it is not allowed to remove this file from its project."), i18n("Cannot Remove File From Project"));
			return;
		}

		m_ki->viewManager()->projectView()->removeItem(item, m_ki->isOpen(item->url()));

		KileProject *project = item->project();
		item->project()->remove(item);

		project->buildProjectTree();
	}
}

void Manager::projectOpenItem(KileProjectItem *item)
{
	kdDebug() << "==Kile::projectOpenItem==========================" << endl;
	kdDebug() << "\titem:" << item->url().path() << endl;

	if (m_ki->isOpen(item->url())) //remove item from projectview (this file was opened before as a normal file)
		m_ki->viewManager()->projectView()->remove(item->url());

	Kate::View *view = loadItem(item);

	if ( (!item->isOpen()) && (view == 0L) && (item->getInfo()) ) //doc shouldn't be displayed, trash the doc
		trashDoc(item->getInfo());
	else if (view != 0L)
		view->setCursorPosition(item->lineNumber(), item->columnNumber());

	//oops, doc apparently was open while the project settings wants it closed, don't trash it the doc, update openstate instead
	if ((!item->isOpen()) && (view != 0L))
		item->setOpenState(true);
}

KileProject* Manager::projectOpen(const KURL & url, int step, int max)
{
	static KProgressDialog *kpd = 0;

	kdDebug() << "==Kile::projectOpen==========================" << endl;
	kdDebug() << "\tfilename: " << url.fileName() << endl;
	if (m_ki->projectIsOpen(url))
	{
		if (kpd != 0) kpd->cancel();

		KMessageBox::information(m_ki->parentWidget(), i18n("The project you tried to open is already opened. If you wanted to reload the project, close the project before you re-open it."),i18n("Project Already Open"));
		return 0L;
	}

	QFileInfo fi(url.path());
	if ( ! fi.isReadable() )
	{
		if (kpd != 0) kpd->cancel();

		if (KMessageBox::warningYesNo(m_ki->parentWidget(), i18n("The project file for this project does not exists or is not readable. Remove this project from the recent projects list?"),i18n("Could Not Load Project File"))  == KMessageBox::Yes)
			emit(removeFromRecentProjects(url));

		return 0L;
	}

	if (kpd == 0)
	{
		kpd = new KProgressDialog (m_ki->parentWidget(), 0, i18n("Open Project..."), QString::null, true);
		kpd->showCancelButton(false);
		kpd->setLabel(i18n("Scanning project files..."));
		kpd->setAutoClose(true);
		kpd->setMinimumDuration(2000);
	}
	kpd->show();

	KileProject *kp = new KileProject(url);

	emit(addToRecentProjects(url));

	KileProjectItemList *list = kp->items();

	int project_steps = list->count() + 1;
	kpd->progressBar()->setTotalSteps(project_steps * max);
	project_steps *= step;
	kpd->progressBar()->setValue(project_steps);

	for ( uint i=0; i < list->count(); i++)
	{
		projectOpenItem(list->at(i));
		kpd->progressBar()->setValue(i + project_steps);
		kapp->processEvents();
	}

	kp->buildProjectTree();
	addProject(kp);

	emit(updateStructure(false, 0L));
	emit(updateModeStatus());

	if (step == (max - 1))
		kpd->cancel();
		
	return kp;
}

KileProject* Manager::projectOpen()
{
	kdDebug() << "==Kile::projectOpen==========================" << endl;
	KURL url = KFileDialog::getOpenURL( "", i18n("*.kilepr|Kile Project Files\n*|All Files"), m_ki->parentWidget(), i18n("Open Project") );

	if (!url.isEmpty())
		return projectOpen(url);
	else
		return 0L;
}


void Manager::projectSave(KileProject *project /* = 0 */)
{
	kdDebug() << "==Kile::projectSave==========================" << endl;
	if (project == 0)
	{
		//find the project that corresponds to the active doc
		project= activeProject();
	}

	if (project == 0 )
		project = selectProject(i18n("Save Project"));

	if (project)
	{
		KileProjectItemList *list = project->items();
		Kate::Document *doc = 0L;

		KileProjectItem *item;
		Info *docinfo;
		//update the open-state of the items
		for (uint i=0; i < list->count(); i++)
		{
			item = list->at(i);
			kdDebug() << "\tsetOpenState(" << item->url().path() << ") to " << m_ki->isOpen(item->url()) << endl;
			item->setOpenState(m_ki->isOpen(item->url()));
			docinfo = item->getInfo();

			kdDebug() << "test" << endl;
			kdDebug() << "\t\tDOCINFO = " << docinfo << ", DOC = " << docinfo->getDoc() << endl;
			if (docinfo != 0L) doc = docinfo->getDoc();
			if (doc != 0L) storeProjectItem(item, doc);

			doc = 0L;
		}

		project->save();
	}
	else
		KMessageBox::error(m_ki->parentWidget(), i18n("The current document is not associated to a project. Please activate a document that is associated to the project you want to save, then choose Save Project again."),i18n( "Could Determine Active Project"));
}

void Manager::projectAddFiles(const KURL & url)
{
	KileProject *project = projectFor(url);

	if (project)
		projectAddFiles(project);
}

void Manager::projectAddFiles(KileProject *project)
{
	kdDebug() << "==Kile::projectAddFiles()==========================" << endl;
 	if (project == 0 )
		project = activeProject();

	if (project == 0 )
		project = selectProject(i18n("Add Files to Project"));

	if (project)
	{
		//determine the starting dir for the file dialog
		QString currentDir=m_ki->fileSelector()->dirOperator()->url().path();
		QFileInfo fi;
		if (m_ki->viewManager()->currentView())
		{
			fi.setFile(m_ki->viewManager()->currentView()->getDoc()->url().path());
			if (fi.exists()) currentDir= fi.dirPath();
		}

		KURL::List urls = KFileDialog::getOpenURLs( currentDir, i18n("*|All Files"), m_ki->parentWidget(), i18n("Add Files") );

		//open them
		for (uint i=0; i < urls.count(); i++)
		{
			addToProject(project, urls[i]);
		}
	}
	else if (m_projects.count() == 0)
		KMessageBox::error(m_ki->parentWidget(), i18n("There are no projects opened. Please open the project you want to add files to, then choose Add Files again."),i18n( "Could Not Determine Active Project"));
}

void Manager::toggleArchive(KileProjectItem *item)
{
	item->setArchive(!item->archive());
}

bool Manager::projectArchive(const KURL & url)
{
	KileProject *project = projectFor(url);

	if (project)
		return projectArchive(project);
	else
		return false;
}

bool Manager::projectArchive(KileProject *project /* = 0*/)
{
	if (project == 0)
		project = activeProject();

	if (project == 0 )
		project = selectProject(i18n("Archive Project"));

	if (project)
	{
		//TODO: this should be in the KileProject class
		//QString command = project->archiveCommand();
		QString files, path;
		QPtrListIterator<KileProjectItem> it(*project->items());
		while (it.current())
		{
			if ((*it)->archive())
			{
				path = (*it)->path();
				KRun::shellQuote(path);
				files += path+" ";
			}
			++it;
		}

		KileTool::Base *tool = new KileTool::Base("Archive", m_ki->toolManager());
		tool->setSource(project->url().path());
		tool->addDict("%F", files);
		m_ki->toolManager()->run(tool);
	}
	else if (m_projects.count() == 0)
		KMessageBox::error(m_ki->parentWidget(), i18n("The current document is not associated to a project. Please activate a document that is associated to the project you want to archive, then choose Archive again."),i18n( "Could Not Determine Active Project"));

	return true;
}

void Manager::projectOptions(const KURL & url)
{
	KileProject *project = projectFor(url);

	if (project)
		projectOptions(project);
}

void Manager::projectOptions(KileProject *project /* = 0*/)
{
	kdDebug() << "==Kile::projectOptions==========================" << endl;
	if (project ==0 )
		project = activeProject();

	if (project == 0 )
		project = selectProject(i18n("Project Options For"));

	if (project)
	{
		kdDebug() << "\t" << project->name() << endl;
		KileProjectOptionsDlg *dlg = new KileProjectOptionsDlg(project, m_ki->parentWidget());
		dlg->exec();
	}
	else if (m_projects.count() == 0)
		KMessageBox::error(m_ki->parentWidget(), i18n("The current document is not associated to a project. Please activate a document that is associated to the project you want to modify, then choose Project Options again."),i18n( "Could Not Determine Active Project"));
}

bool Manager::projectCloseAll()
{
	kdDebug() << "==Kile::projectCloseAll==========================" << endl;
	bool close = true;

	QPtrListIterator<KileProject> it(m_projects);
	int i = m_projects.count() + 1;
	while ( it.current() && close && (i > 0))
	{
		close = close && projectClose(it.current()->url());
		i--;
	}

	return close;
}

bool Manager::projectClose(const KURL & url)
{
	kdDebug() << "==Kile::projectClose==========================" << endl;
	KileProject *project = 0;

	if (url.isEmpty())
	{
		 project = activeProject();

		 if (project == 0 )
			project = selectProject(i18n("Close Project"));
	}
	else
	{
		project = projectFor(url);
	}

 	if (project)
	{
		kdDebug() << "\tclosing:" << project->name() << endl;

		//close the project file first, projectSave, changes this file
		Kate::Document *doc = docFor(project->url());
		if (doc)
		{
			doc->save();
			fileClose(doc);
		}

		projectSave(project);

		KileProjectItemList *list = project->items();

		bool close = true;
		Info *docinfo;
		for (uint i =0; i < list->count(); i++)
		{
			docinfo = list->at(i)->getInfo();
			if (docinfo) doc = docinfo->getDoc();
			if (doc)
			{
				kdDebug() << "\t\tclosing item " << doc->url().path() << endl;
				bool r = fileClose(doc, true);
				close = close && r;
				if (!close) break;
			}
			else
				removeDocumentInfo(docinfo, true);

			docinfo = 0L;
			doc = 0L;
		}

		if (close)
		{
			m_projects.remove(project);
			//FIXME: use signal/slot
			m_ki->viewManager()->projectView()->remove(project);
			delete project;
			return true;
		}
		else
			return false;
	}
	else if (m_projects.count() == 0)
		KMessageBox::error(m_ki->parentWidget(), i18n("The current document is not associated to a project. Please activate a document that is associated to the project you want to close, then choose Close Project again."),i18n( "Could Not Close Project"));

	return true;
}

void Manager::storeProjectItem(KileProjectItem *item, Kate::Document *doc)
{
	kdDebug() << "===Kile::storeProjectItem==============" << endl;
	kdDebug() << "\titem = " << item << ", doc = " << doc << endl;
	item->setEncoding(doc->encoding());
	item->setHighlight(doc->hlModeName(doc->hlMode()));
	Kate::View *view = static_cast<Kate::View*>(doc->views().first());
	uint l = 0, c = 0;
	if (view) view->cursorPosition(&l,&c);
	item->setLineNumber(l);
	item->setColumnNumber(c);

	kdDebug() << "\t" << item->encoding() << " " << item->highlight() << " should be " << doc->hlModeName(doc->hlMode()) << endl;
}

void Manager::cleanUpTempFiles(Info *docinfo, bool silent)
{
	QStringList extlist;
	QStringList templist = QStringList::split(" ", KileConfig::cleanUpFileExtensions());
	QString str;
	QFileInfo file(docinfo->url().path()), fi;
	for (uint i=0; i <  templist.count(); i++)
	{
		str = file.dirPath(true) + "/" + file.baseName(true) + templist[i];
		fi.setFile(str);
		if ( fi.exists() )
			extlist.append(templist[i]);
	}

	str = file.fileName();
	if (!silent &&  (str==i18n("Untitled") || str == "" ) )	return;

	if (!silent && extlist.count() > 0 )
	{
		kdDebug() << "\tnot silent" << endl;
		KileDialog::Clean *dialog = new KileDialog::Clean(m_ki->parentWidget(), str, extlist);
		if ( dialog->exec() )
			extlist = dialog->getCleanlist();
		else
		{
			delete dialog;
			return;
		}

		delete dialog;
	}

	if ( extlist.count() == 0 )
	{
		m_ki->logWidget()->printMsg(KileTool::Warning, i18n("Nothing to clean for %1").arg(str), i18n("Clean"));
		return;
	}

	m_ki->logWidget()->printMsg(KileTool::Info, i18n("cleaning %1 : %2").arg(str).arg(extlist.join(" ")), i18n("Clean"));

	docinfo->cleanTempFiles(extlist);
}

};

#include "kiledocmanager.moc"

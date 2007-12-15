/***************************************************************************
    begin                : mon 3-11 20:40:00 CEST 2003
    copyright            : (C) 2003 by Jeroen Wijnhout
    email                : Jeroen.Wijnhout@kdemail.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kilelauncher.h"

#include "kileinfo.h"
#include "kiletool.h"
#include "kiletoolmanager.h"
#include "kiletool_enums.h"
#include "docpart.h"
#include "kileconfig.h"

#include <QStackedWidget>
#include <qregexp.h>
#include <qfileinfo.h>

#include "kiledebug.h"
#include <krun.h>
#include <k3process.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <klibloader.h>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/partmanager.h>

 namespace KileTool
{
	Launcher::Launcher() :
		m_tool(0L)
	{
	}
	
	Launcher::~ Launcher()
	{
		KILE_DEBUG() << "DELETING launcher" << endl;
	}

	ProcessLauncher::ProcessLauncher(const char * shellname /* =0 */) :
		m_wd(QString::null),
		m_cmd(QString::null),
		m_options(QString::null),
		m_texinputs(KileConfig::teXPaths()),
		m_bibinputs(KileConfig::bibInputPaths()),
 		m_bstinputs(KileConfig::bstInputPaths()),
		m_changeTo(true)
	{
		KILE_DEBUG() << "==KileTool::ProcessLauncher::ProcessLauncher()==============" << endl;

		m_proc = new K3ShellProcess(shellname);
		if (m_proc)
			KILE_DEBUG() << "\tKShellProcess created" << endl;
		else
			KILE_DEBUG() << "\tNO K3ShellProcess created" << endl;

		connect(m_proc, SIGNAL( receivedStdout(K3Process*, char*, int) ), this, SLOT(slotProcessOutput(K3Process*, char*, int ) ) );
 		connect(m_proc, SIGNAL( receivedStderr(K3Process*, char*, int) ),this, SLOT(slotProcessOutput(K3Process*, char*, int ) ) );
		connect(m_proc, SIGNAL( processExited(K3Process*)), this, SLOT(slotProcessExited(K3Process*)));
	}

	ProcessLauncher::~ProcessLauncher()
	{
		KILE_DEBUG() << "DELETING ProcessLauncher" << endl;
		delete m_proc;
	}

	void ProcessLauncher::setWorkingDirectory(const QString &wd)
	{
		m_wd = wd;
	}

	bool ProcessLauncher::launch()
	{
		KILE_DEBUG() << "KileTool::ProcessLauncher::launch()=================" << endl;
		KILE_DEBUG() << "\tbelongs to tool " << tool()->name() << endl;

		QString msg, out = "*****\n*****     " + tool()->name() + i18n(" output: \n");

		if ( m_cmd.isNull() ) 
			m_cmd = tool()->readEntry("command");

		if ( m_options.isNull() )
			m_options = tool()->readEntry("options");
		
		if ( m_changeTo && (!m_wd.isNull()) )
		{
			m_proc->setWorkingDirectory(m_wd);
			out += QString("*****     cd '")+ m_wd +QString("'\n");
		}

		QString str;
		tool()->translate(m_cmd);
		tool()->translate(m_options);
		*m_proc  << m_cmd << m_options;
		
		if (m_proc)
		{
			out += QString("*****     ")+ m_cmd+ ' ' + m_options + '\n';

			QString src = tool()->source(false);
			QString trgt = tool()->target();
			if (src == trgt)
				msg = src;
			else
				msg = src + " => " + trgt;

			msg += " (" + m_proc->args()[0] + ')';
			 
			emit(message(Info,msg));

			// QuickView tools need a special TEXINPUTS environment variable
			if ( tool()->isQuickie() ) 
				m_texinputs = KileConfig::previewTeXPaths();

			KILE_DEBUG() << "$PATH=" << tool()->manager()->info()->expandEnvironmentVars("$PATH") << endl;
			KILE_DEBUG() << "$TEXINPUTS=" << tool()->manager()->info()->expandEnvironmentVars(m_texinputs + ":$TEXINPUTS") << endl;
			KILE_DEBUG() << "$BIBINPUTS=" << tool()->manager()->info()->expandEnvironmentVars(m_bibinputs + ":$BIBINPUTS") << endl;
			KILE_DEBUG() << "$BSTINPUTS=" << tool()->manager()->info()->expandEnvironmentVars(m_bstinputs + ":$BSTINPUTS") << endl;
			KILE_DEBUG() << "Tool name is "<< tool()->name() << endl;

			m_proc->setEnvironment("PATH",tool()->manager()->info()->expandEnvironmentVars("$PATH"));

			if (! m_texinputs.isEmpty())
				m_proc->setEnvironment("TEXINPUTS", tool()->manager()->info()->expandEnvironmentVars(m_texinputs + ":$TEXINPUTS"));
			if (! m_bibinputs.isEmpty())
				m_proc->setEnvironment("BIBINPUTS", tool()->manager()->info()->expandEnvironmentVars(m_bibinputs + ":$BIBINPUTS"));
			if (! m_bstinputs.isEmpty())
				m_proc->setEnvironment("BSTINPUTS", tool()->manager()->info()->expandEnvironmentVars(m_bstinputs + ":$BSTINPUTS"));

			out += "*****\n";
			emit(output(out));

			return m_proc->start(tool()->manager()->shouldBlock() ? K3Process::Block : K3Process::NotifyOnExit, K3Process::AllOutput);
		}
		else
			return false;
	}

	bool ProcessLauncher::kill()
	{
		KILE_DEBUG() << "==KileTool::ProcessLauncher::kill()==============" << endl;
		if ( m_proc && m_proc->isRunning() )
		{
			KILE_DEBUG() << "\tkilling" << endl;
			return m_proc->kill();
		}
		else
		{
			KILE_DEBUG() << "\tno process or process not running" << endl;
			return false;
		}
	}

	bool ProcessLauncher::selfCheck()
	{
		emit(message(Error, i18n("Launching failed, diagnostics:")));

		QString exe = KRun::binaryName(tool()->readEntry("command"), false);
		QString path = KGlobal::dirs()->findExe(exe, QString(), KStandardDirs::IgnoreExecBit);

		if ( path.isNull() )
		{
			emit(message(Error, i18n("There is no executable named \"%1\" in your path.").arg(exe)));
			return false;
		}
		else
		{
			QFileInfo fi(path);
			if ( ! fi.isExecutable() )
			{
				emit(message(Error, i18n("You do not have permission to run %1.").arg(path)));
				return false;
			}
		}

		emit(message(Info, i18n("Diagnostics could not find any obvious problems.")));
		return true;
	}

	void ProcessLauncher::slotProcessOutput(K3Process*, char* buf, int len)
	{
		emit output(QString::fromLocal8Bit(buf, len));
	}

	void ProcessLauncher::slotProcessExited(K3Process*)
	{
		KILE_DEBUG() << "==KileTool::ProcessLauncher::slotProcessExited=============" << endl;
		KILE_DEBUG() << "\t" << tool()->name() << endl;

		if (m_proc)
		{
			if (m_proc->normalExit())
			{
				KILE_DEBUG() << "\tnormal exit" << endl;
				int type = Info;
				if (m_proc->exitStatus() != 0) 
				{
					type = Error;
					emit(message(type,i18n("finished with exit status %1").arg(m_proc->exitStatus())));
				}

				if (type == Info)
					emit(done(Success));
				else
					emit(done(Failed));
			}
			else
			{
				KILE_DEBUG() << "\tabnormal exit" << endl;
				emit(message(Error,i18n("finished abruptly")));
				emit(done(AbnormalExit));
			}
		}
		else
		{
			kWarning() << "\tNO PROCESS, emitting done" << endl;
			emit(done(Success));
		}
	}

	KonsoleLauncher::KonsoleLauncher(const char * shellname) : ProcessLauncher(shellname)
	{
	}

	bool KonsoleLauncher::launch()
	{
		QString cmd = tool()->readEntry("command");
		QString noclose = (tool()->readEntry("close") == "no") ? "--noclose" : "";
		setCommand("konsole");
		setOptions(noclose + " -T \"" + cmd + " (Kile)\" -e " + cmd + ' ' + tool()->readEntry("options"));

		if ( KGlobal::dirs()->findExe(KRun::binaryName(cmd, false)).isNull() ) return false;

		return ProcessLauncher::launch();
	}

	PartLauncher::PartLauncher(const char *name /* = 0*/ ) :
		m_part(0L),
		m_state("Viewer"),
		m_name(name),
		m_libName(0L),
		m_className(0L),
		m_options(QString::null)
	{
	}

	PartLauncher::~PartLauncher()
	{
		KILE_DEBUG () << "DELETING PartLauncher" << endl;
	}

	bool PartLauncher::launch()
	{
		m_libName = tool()->readEntry("libName").ascii();
		m_className = tool()->readEntry("className").ascii();
		m_options=tool()->readEntry("libOptions");
		m_state=tool()->readEntry("state");

		QString msg, out = "*****\n*****     " + tool()->name() + i18n(" output: \n");
		
		QString shrt = "%target";
		tool()->translate(shrt);
		QString dir  = "%dir_target"; tool()->translate(dir);

		QString name = shrt;
		if ( dir[0] == '/' )
			name = dir + '/' + shrt;


		KLibFactory *factory = KLibLoader::self()->factory(m_libName);
		if (factory == 0)
		{
			emit(message(Error, i18n("Could not find the %1 library.").arg(m_libName)));
			return false;
		}

		QStackedWidget *stack = tool()->manager()->widgetStack();
		KParts::PartManager *pm = tool()->manager()->partManager();

#ifdef __GNUC__
#warning Port KPluginFactory::create correctly!
#endif
//FIXME: port for KDE4
// 		m_part = (KParts::ReadOnlyPart *)factory->create(stack, m_libName, m_className, m_options);
m_part = NULL;
		if (m_part == 0)
		{
			emit(message(Error, i18n("Could not create component %1 from the library %2.").arg(m_className).arg(m_libName)));
			emit(done(Failed));
			return false;
		}
		else
		{
			QString cmd = QString(m_libName) + "->" + QString(m_className) + ' ' + m_options + ' ' + name;
			out += "*****     " + cmd + '\n';

			msg = shrt+ " (" + tool()->readEntry("libName") + ')';
			emit(message(Info,msg));
		}

		out += "*****\n";
		emit(output(out));

		tool()->manager()->wantGUIState(m_state);

		stack->insertWidget(1, m_part->widget());
		stack->setCurrentIndex(1);

		m_part->openUrl(KUrl(name));
		pm->addPart(m_part, true);
		pm->setActivePart(m_part);

		emit(done(Success));

		return true;
	}

	bool PartLauncher::kill()
	{
		return true;
	}

	bool DocPartLauncher::launch()
	{
		m_state=tool()->readEntry("state");

		QString shrt = "%target";
		tool()->translate(shrt);
		QString name="%dir_target/%target";
		tool()->translate(name);

		QString out = "*****\n*****     " + tool()->name() + i18n(" output: \n") + "*****     KHTML " + name + "\n*****\n";
		QString msg =  shrt+ " (KHTML)";
		emit(message(Info, msg));
		emit(output(out));

		QStackedWidget *stack = tool()->manager()->widgetStack();
		KParts::PartManager *pm = tool()->manager()->partManager();

		DocumentationViewer *htmlpart = new DocumentationViewer(stack);
		htmlpart->setObjectName("help");
		m_part = static_cast<KParts::ReadOnlyPart*>(htmlpart);

		connect(htmlpart, SIGNAL(updateStatus(bool, bool)), tool(), SIGNAL(updateStatus(bool, bool)));

		tool()->manager()->wantGUIState(m_state);

		htmlpart->openUrl(KUrl(name));
		htmlpart->addToHistory(name);
		stack->insertWidget(1, htmlpart->widget());
		stack->setCurrentIndex(1);

		pm->addPart(htmlpart, true);
		pm->setActivePart( htmlpart);

		emit(done(Success));
		
		return true;
	}
}

#include "kilelauncher.moc"

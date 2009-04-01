/***************************************************************************
 *   Copyright (C) 2008 by David Sansome                                   *
 *   me@davidsansome.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "utilities.h"
#include "engine.h"

#ifdef USE_FREEBSD
#include <kvm.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/file.h>
#endif

#ifdef USE_SOLARIS
#include <procfs.h>
#endif

#include <QFile>
#include <QDebug>

#include <gtk/gtk.h>


QString GtkQtUtilities::getCommandLine()
{
	QString commandLine;
	
#ifdef USE_FREEBSD
	// procfs(5) is deprecated on FreeBSD.
	// We use the kvm(3) library to get argv[0] of the current pid.
	
	kvm_t* kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
	if (kd == NULL)
		qWarning() << "kvm_open failed";
	else
	{
		int cnt = 0;
		struct kinfo_proc* pbase = kvm_getprocs(kd, KERN_PROC_PID, getpid(), &cnt);
		if (( pbase == NULL ) || ( cnt != 1 ))
			qWarning() << "kvm_getprocs failed";
		else
		{
			char** arg = kvm_getargv(kd, pbase, 1024);
			if (arg == NULL)
				qWarning() << "kvm_getargv failed";
			else
				commandLine = arg[0];
		}
		kvm_close(kd);
	}
#endif // USE_FREEBSD
	
#ifdef USE_SOLARIS
	int pid=getpid();
	char filen[256];
	psinfo_t pfi;
	uintptr_t addr = 0;
	uintptr_t addr2 = 0;
	int  i,count,readl;
	
	sprintf(filen, "/proc/%d/psinfo",pid);
	int fd=open(filen, O_RDONLY);
	if (fd == -1)
		qWarning() << "Open of psinfo failed";
	else
	{
		readl=read(fd, (void *)&pfi, sizeof(psinfo_t));
		if (readl < 0)
			qWarning() << "Read on as failed";
		else
		{
			addr=pfi.pr_argv;
			count=pfi.pr_argc;
		}
		close(fd);
	}
	/* if read of psinfo was success */
	if (addr != 0)
	{
		sprintf(filen, "/proc/%d/as",pid);
		fd=open(filen, O_RDONLY);
		if (fd == -1)
			qWarning() << "Open of as failed";
		else if (count > 0)
		{
			lseek(fd, addr, SEEK_SET);
			if (pfi.pr_dmodel == PR_MODEL_ILP32)
			{
				addr += 4;
				read(fd, (void *)&addr2, 4);
			}
			else
			{
				addr += 8;
				read(fd, (void *)&addr2, 8);
			}
			commandLine += (char*) addr2;
			
			close(fd);
		}
	}
#else // USE_SOLARIS
#ifndef USE_FREEBSD
	QFile cmdFile("/proc/self/cmdline");
	cmdFile.open(QIODevice::ReadOnly);
	commandLine = cmdFile.readAll();
#endif // USE_FREEBSD
#endif // USE_SOLARIS

	return commandLine;
}



QString GtkQtUtilities::colorString(const QColor& color)
{
	QString ret = "{";
	ret += QString::number(color.red() * 257) + ", ";
	ret += QString::number(color.green() * 257) + ", ";
	ret += QString::number(color.blue() * 257) + "}";
	
	return ret;
}

QColor GtkQtUtilities::convertColor(const GdkColor& color)
{
	int r = color.red / 257;
	int g = color.green / 257;
	int b = color.blue / 257;
	return QColor(r, g, b);
}

GdkColor GtkQtUtilities::convertColor(const QColor& color, GtkStyle* style)
{
	GdkColor ret;
	ret.red = color.red() * 257;
	ret.green = color.green() * 257;
	ret.blue = color.blue() * 257;
	
	gdk_colormap_alloc_color(style->colormap, &ret, false, true);
	
	return ret;
}


void GtkQtUtilities::parseRcString(const QString& string)
{
	gtk_rc_parse_string(string.toAscii().data());
	GTK_QT_DEBUG("Setting RC string:" << string.trimmed())
}

void GtkQtUtilities::parseRcString(const QString& defs, const QString& pattern, StyleType type)
{
	parseRcString(generateRcString(defs, pattern, type));
}

// Thanks Martin Dvorak of metatheme
QString GtkQtUtilities::generateRcString(const QString& defs, const QString& pattern, StyleType type)
{
	static int count = 0;
	count++;
	
	QString typeStr;
	switch (type)
	{
		case Class:       typeStr = "class";        break;
		case WidgetClass: typeStr = "widget_class"; break;
		case Widget:      typeStr = "widget";       break;
	}
	
	QString countStr(QString::number(count));
	QString ret("style \"gtk-qt-dynamic-" + countStr + "\" { " + defs + " } " + typeStr + " \"" + pattern + "\" style \"gtk-qt-dynamic-" + countStr + "\"\n");

	return ret;
}

/***************************************************************************
                          krpmview.cpp  -  description
                             -------------------
    begin                : Die Jul  9 13:09:32 CEST 2002
    copyright            : (C) 2002 by Adrian Schröter
    email                : adrian@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <kprocess.h>
#include <krun.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <time.h>
#include <QTextDocument>

#include "krpmview.h"
#include "krpmview_factory.h"

extern "C" {
  // from legacy.c from rpm sources
  void expandFilelist(Header h);
};

// typedef KParts::GenericFactory<KRPMViewPart> KRPMViewFactory;
K_EXPORT_COMPONENT_FACTORY( libkrpmview, KRPMViewFactory );

KRPMViewPart::KRPMViewPart( QWidget *parentWidget,
            QObject *parent, const QStringList &)
 : KParts::ReadOnlyPart( parent )
{
  setComponentData( KRPMViewPart::componentData() );
  setObjectName( name );

  KGlobal::locale()->insertCatalog("krpmview");

  // Create the tree widget, and set it as the part's widget
  box = new QWidget(parentWidget);
  box->setObjectName( name );

  // button widget
  QWidget *bwidget    = new QWidget( box );
  bwidget->setObjectName( "bwidget" );
  QVBoxLayout *Layout = new QVBoxLayout( bwidget );
  Layout->setSpacing( 6 );
  Layout->setObjectName ("Layout");
  Layout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

  QHBoxLayout *LayoutInner = new QHBoxLayout();
  LayoutInner->setSpacing( 6 );
  LayoutInner->setObjectName("LayoutInner");
  LayoutInner->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );

  PushButtonPackage = new QPushButton( bwidget );
  PushButtonPackage->setObjectName("PushButtonPackage" );
  PushButtonPackage->setText( i18n( "Install Package with YaST" ) );
  LayoutInner->addWidget( PushButtonPackage );
  LayoutInner->addItem( new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );

  PushButtonDir = new QPushButton( bwidget );
  PushButtonDir->setObjectName("PushButtonDir" );
  PushButtonDir->setEnabled(false);
  PushButtonDir->setText( i18n( "Use Directory as Source with YaST" ) );
  LayoutInner->addWidget( PushButtonDir );
  LayoutInner->addItem(new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ));
  Layout->addLayout( LayoutInner );
  Layout->addItem( new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
  bwidget->setMaximumHeight( PushButtonPackage->fontInfo().pixelSize() * 4 );
  PushButtonDir->setHidden(true);


  // create the tab box
  tab = new QTabBar( box );
  browserDescription = new KTextBrowser( tab, "browser" );
  browserTechnicalData = new KTextBrowser( tab, "technicaldata" );
  browserDependencies = new KTextBrowser( tab, "dependencies" );
  browserChangelog   = new KTextBrowser( tab, "changelog" );
  browserFilelist    = new KTextBrowser( tab, "filelist" );
  browserTechnicalData->hide();
  browserDependencies->hide();
  browserChangelog->hide();
  browserFilelist->hide();

  tab->addTab( i18n("Description") );
  tab->addTab( i18n("Technical Data") );
  tab->addTab( i18n("Dependencies") );
  tab->addTab( i18n("File List") );
  tab->addTab( i18n("Change Log") );
  connect( tab, SIGNAL(selected(int)), this, SLOT(selectedTab(int)) );

  QVBoxLayout *boxlayout = new QVBoxLayout;
  boxlayout->addWidget(bwidget);
  boxlayout->addWidget(tab);
  boxlayout->addWidget(browserDescription);
  boxlayout->addWidget(browserTechnicalData);
  boxlayout->addWidget(browserDependencies);
  boxlayout->addWidget(browserChangelog);
  boxlayout->addWidget(browserFilelist);
  box->setLayout(boxlayout);

  setWidget( box );

  connect( browserDescription, SIGNAL(urlClick(const QString&)), this, SLOT(urlClick(const QString&)) );
  connect( PushButtonPackage, SIGNAL(clicked()), this, SLOT(install_package()) );
  connect( PushButtonDir, SIGNAL(clicked()), this, SLOT(use_directory()) );

  // Action creation code will go here
  setXMLFile( "krpmview.rc" );
}

void KRPMViewPart::selectedTab(int t)
{
  browserDescription->hide();
  browserDependencies->hide();
  browserTechnicalData->hide();
  browserChangelog->hide();
  browserFilelist->hide();
  switch( t ){
    case 0:
     browserDescription->show();
     break;
    case 1:
     browserTechnicalData->show();
     break;
    case 2:
     browserDependencies->show();
     break;
    case 3:
     browserFilelist->show();
     break;
    case 4:
     browserChangelog->show();
     break;
  }
}

QString KRPMViewPart::createDependencyList(const Header &h, const QString &caption, int TAGNAME, int TAGVERSION, int TAGFLAGS, bool strongState)
{
  QString result, temp;
  void *tmpVoid = 0;
  int nEntries;
  bool captionAdded = false;

  if (headerGetEntry(h, TAGNAME, NULL, &tmpVoid, &nEntries)) {
     const char **files = (const char **)tmpVoid;
     headerGetEntry(h, TAGVERSION, NULL, &tmpVoid, NULL);
     const char **version = (const char **)tmpVoid;
     headerGetEntry(h, TAGFLAGS, NULL, &tmpVoid, NULL);
     const uint *flags = (const uint *)tmpVoid;
     for (int i = 0; i < nEntries; i++){
        if (((flags[i] & RPMSENSE_STRONG) == RPMSENSE_STRONG) == strongState) {
          if (!captionAdded) {
            result += "<h3>"+caption+"</h3>";
            captionAdded = true;
          }
          temp.sprintf("%s", files[i]);
          result += temp;
          temp.sprintf("%s", version[i]);
          if (!temp.isEmpty()) {
            result += " ";
            if (flags[i] & RPMSENSE_LESS)
              result += "<";
            if (flags[i] & RPMSENSE_GREATER)
              result += ">";
            if (flags[i] & RPMSENSE_EQUAL)
              result += "=";
            result += " " + temp;
          }
          result += "<br>";
       }
     }
  }
  return result;
}

bool KRPMViewPart::openFile()
{
  QString changelog, filelist, temp, technicaldata, dependencies;
  int nfiles;
  int numchangelog;
  int i;
  FD_t fd;
  Header h;
  rpmts ts;

  fd = Fopen(localFilePath().toAscii(), "r.ufdio");
  if (fd == 0 || Ferror(fd))
     return false;

  ts = rpmtsCreate();
  rpmtsSetVSFlags(ts, rpmVSFlags(_RPMVSF_NODIGESTS|_RPMVSF_NOSIGNATURES|RPMVSF_NOHDRCHK));

  if (rpmReadPackageFile(ts, fd, "krpmview", &h) != RPMRC_OK) {
     rpmtsFree(ts);
     Fclose(fd);
     return false;
  }
  rpmtsFree(ts);

  headerNVR(h, &name, &version, &release);
  void *tmpVoid = 0;
  if (!headerGetEntry(h, RPMTAG_DESCRIPTION, NULL, &tmpVoid, NULL))
    description=0;
  else
    description=(const char*)tmpVoid;

  if (!headerGetEntry(h, RPMTAG_SUMMARY, NULL, &tmpVoid, NULL))
    summary=0;
  else
    summary=(const char*)tmpVoid;

  if (!headerGetEntry(h, RPMTAG_VENDOR, NULL, &tmpVoid, NULL))
    vendor=0;
  else
    vendor=(const char*)tmpVoid;

  if (!headerGetEntry(h, RPMTAG_URL, NULL, &tmpVoid, NULL))
    url=0;
  else
    url=(const char*)tmpVoid;

  if (headerGetEntry(h, RPMTAG_CHANGELOGTEXT, NULL, &tmpVoid, &numchangelog)) {
      const char **changelogtext=(const char**)tmpVoid;
      headerGetEntry(h, RPMTAG_CHANGELOGNAME, NULL, &tmpVoid, NULL);
      const char **changelogname=(const char**)tmpVoid;
      headerGetEntry(h, RPMTAG_CHANGELOGTIME, NULL, &tmpVoid, NULL);
      const uint_32 * changelogtime=(const uint_32 *)tmpVoid;
      for (i = 0; i < numchangelog; i++)
        {
          time_t t = changelogtime[i];
	  temp = changelogname[i];
          temp.sprintf("* %.24s %s\n\n%s\n\n", ctime(&t), changelogname[i], changelogtext[i]);
          changelog += temp;
        }
  }
  expandFilelist(h);
  if (headerGetEntry(h, RPMTAG_OLDFILENAMES, NULL, &tmpVoid, &nfiles)) {
     const char **files = (const char **)tmpVoid;
     for (i = 0; i < nfiles; i++){
        temp.sprintf("%s\n", files[i]);
        filelist += temp;
     }
  }

  if (headerGetEntry(h, RPMTAG_LICENSE, NULL, &tmpVoid, NULL))
      technicaldata += i18n("License: %1\n").arg((const char *)(tmpVoid));

  if (headerGetEntry(h, RPMTAG_GROUP, NULL, &tmpVoid, NULL))
      technicaldata += i18n("RPM group: %1\n").arg((const char *)(tmpVoid));

  if (headerGetEntry(h, RPMTAG_DISTRIBUTION, NULL, &tmpVoid, NULL))
      technicaldata += i18n("Distribution: %1\n").arg((const char *)(tmpVoid));

  if (headerGetEntry(h, RPMTAG_PACKAGER, NULL, &tmpVoid, NULL))
      technicaldata += i18n("Packager: %1\n").arg((const char *)(tmpVoid));

  if (headerGetEntry(h, RPMTAG_VENDOR, NULL, &tmpVoid, NULL))
      technicaldata += i18n("Vendor: %1\n").arg((const char *)(tmpVoid));

  if (headerGetEntry(h, RPMTAG_BUILDTIME, NULL, &tmpVoid, NULL)) {
      const uint_32 * buildtime=(const uint_32 *)tmpVoid;
      time_t t=buildtime[0];
      temp.sprintf("%.24s", ctime(&t));
      technicaldata += i18n("Build time: %1\n").arg(temp);
  }

  if (headerGetEntry(h, RPMTAG_BUILDHOST, NULL, &tmpVoid, NULL))
      technicaldata += i18n("Build host: %1\n").arg((const char *)(tmpVoid));

  if (headerGetEntry(h, RPMTAG_SIZE, NULL, &tmpVoid, NULL)) {
      const uint *size = (const uint *)tmpVoid;
      temp.sprintf("%d", *size);
      technicaldata += i18n("Size: %1\n").arg(temp);
  }
  if (headerGetEntry(h, RPMTAG_SOURCERPM, NULL, &tmpVoid, NULL))
      technicaldata += i18n("Source RPM: %1\n").arg((const char *)(tmpVoid));

  dependencies += createDependencyList(h, i18n("Provides"), RPMTAG_PROVIDENAME, RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS, false);

  dependencies += createDependencyList(h, i18n("Requires"), RPMTAG_REQUIRENAME, RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS, false);

  dependencies += createDependencyList(h, i18n("Conflicts"), RPMTAG_CONFLICTNAME, RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS, false);

  dependencies += createDependencyList(h, i18n("Obsoletes"), RPMTAG_OBSOLETENAME, RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS, false);

  dependencies += createDependencyList(h, i18n("Recommends"), RPMTAG_SUGGESTSNAME, RPMTAG_SUGGESTSVERSION, RPMTAG_SUGGESTSFLAGS, true);

  dependencies += createDependencyList(h, i18n("Suggests"), RPMTAG_SUGGESTSNAME, RPMTAG_SUGGESTSVERSION, RPMTAG_SUGGESTSFLAGS, false);

  dependencies += createDependencyList(h, i18n("Enhances"), RPMTAG_ENHANCESNAME, RPMTAG_ENHANCESVERSION, RPMTAG_ENHANCESFLAGS, false);

  dependencies += createDependencyList(h, i18n("Supplements"), RPMTAG_ENHANCESNAME, RPMTAG_ENHANCESVERSION, RPMTAG_ENHANCESFLAGS, true);

  Fclose(fd);

  QString text;
  text =  "<h2>" + Qt::convertFromPlainText(QString(name) + " - " + QString(summary)) + "</h2><h3>"
          " Version: " + QString(version) + "-" + QString(release) + "</h3>";
  text += "<p>" + i18n("Project Page: ") + "<a href=" + QString::fromLocal8Bit(url) + ">" + QString::fromLocal8Bit(url) + "</a>";
//  text += "<p>" + i18n("Vendor: ") + QStyleSheet::convertFromPlainText(QString::fromLocal8Bit(vendor));
  text += "<p><p>";
  text += Qt::convertFromPlainText(QString::fromLocal8Bit(description));
  browserDescription->setText( text );
  browserDescription->setNotifyClick(true);
  browserDependencies->setText( dependencies );
  browserTechnicalData->setText( technicaldata );
  browserChangelog->setText( changelog );
  browserFilelist->setText( filelist );

  if ( KParts::ReadOnlyPart::url().isLocalFile() )
    PushButtonDir->setEnabled(true);
  else
    PushButtonDir->setEnabled(false);

  // Some feedback for the user
  emit setStatusBarText( i18n("Parsing complete.") );
  return true;
}

void KRPMViewPart::urlClick(const QString &url)
{
  KRun::runUrl( url, "text/html", 0 );
}

void KRPMViewPart::install_package()
{
  KProcess p;
  p << "kdesu" << "-n" << "--nonewdcop" << "--" << "/usr/share/kde4/apps/krpmview/setup_temp_source" << localFilePath().toAscii();
  p.start();
}

void KRPMViewPart::use_directory()
{
  KProcess p;
  p << "kdesu" << "-n" << "--" << "/usr/bin/kde_add_yast_source.sh";
  p << KParts::ReadOnlyPart::url().path().left(KParts::ReadOnlyPart::url().path().lastIndexOf("/"));

  p.start();
}

KAboutData* KRPMViewPart::createAboutData()
{
    KAboutData* about = new KAboutData( "krpmview", 0, ki18n("krpmview"),
            "0.1",
            ki18n( "Viewer for RPM files"),
            KAboutData::License_GPL,
            ki18n("(C) 2003 Novell Inc."),
            ki18n( "KRPMView views the content of RPM archives "
                       "and can use YaST to install them" ) );
    about->addAuthor( ki18n("Adrian Schröter"),
                      ki18n( "Current Maintainer" ),
                      "adrian@suse.de",
                      "http://www.suse.de" );

    return about;
}

#include "krpmview.moc"
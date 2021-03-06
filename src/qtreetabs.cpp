#include "qtreetabs.h"
#include "ui_qtreetabs.h"

#include "opentab.h"
#include "js/localscript.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineScriptCollection>
#include <QWebChannel>
#include <QStackedWidget>
#include <QSplitter>
#include <QMap>
#include <QLayout>
#include <QUrl>
#include <iostream>

QTreeTabs::QTreeTabs(QWidget *parent) :
	QWidget(parent),
	m_profile(new QWebEngineProfile("tabs", this)),
	m_currentTab(NULL),
	m_map(new QMap<QString, OpenTab*>()),
	ui(new Ui::QTreeTabs)
{
	ui->setupUi(this);

	// Create and configure the QWebChannel for communication between the
	// JavaScript in the tabs view and ourselves
	this->m_channel = new QWebChannel(this);
	this->m_channel->registerObject("god", this);

	// Create the WebEngine instance that will do the Lord's work
	QWebEnginePage* page = new QWebEnginePage(this->m_profile, this->ui->m_tabs);
	this->ui->m_tabs->setPage(page);
	Q_INIT_RESOURCE(web);
	this->ui->m_tabs->setUrl(QUrl("qrc:///tabs.html"));
	page->setWebChannel(this->m_channel);

	qRegisterMetaType<OpenTab*>();
}

QTreeTabs::~QTreeTabs()
{
	// Nothing here?
}

OpenTab* QTreeTabs::addItem(QWidget *widget, bool childOfActive, bool displayNow)
{
	// Create new tab
	OpenTab* tab = new OpenTab(widget);

	// Currently, this doesn't work - items registered after the first client connects are
	// not able to be registered into the namespace. When that feature is lifted, this next
	// line will become relevant once again
//    this->m_channel->registerObject(tab->uuid(), tab);
	tab->connect(tab, &OpenTab::onTextChanged, this, &QTreeTabs::onTextChanged);
	tab->connect(tab, &OpenTab::onIconUrlChanged, this, &QTreeTabs::onIconUrlChanged);

	this->m_map->insert(tab->uuid(), tab);
	this->ui->m_widgets->addWidget(widget);
	// Notify listeners of new tab creation
	emit tabCreated(tab->uuid());
	if (displayNow)
		this->setCurrentTab(tab);

	return tab;
}

void QTreeTabs::removeItem(const QString& uuid) {
	this->removeItem(this->tabFromUuid(uuid));
}

void QTreeTabs::removeItem(OpenTab *tab) {
	if (tab != NULL) {
		this->m_map->remove(tab->uuid());
		// TODO: Closing the last remaining item?
		if (this->m_map->isEmpty())
			this->setCurrentTab(NULL);
		else
			this->setCurrentTab(this->m_map->first());
		this->ui->m_widgets->removeWidget(tab->widget());
		emit closedTab(tab->uuid(), this->m_map->size());
		delete tab;
	}
}

void QTreeTabs::setCurrentTab(const QString &uuid)
{
	this->setCurrentTab(this->tabFromUuid(uuid));
}

void QTreeTabs::setCurrentTab(OpenTab *tab)
{
	if (tab != NULL) {
		if (this->ui->m_widgets->indexOf(tab->widget()) != -1) {
			this->ui->m_widgets->setCurrentWidget(tab->widget());
			OpenTab* previous = this->m_currentTab;
			QString oldUuid = "";
			if ( previous != NULL ) oldUuid = previous->uuid();
			this->m_currentTab = tab;
			emit tabChanged(oldUuid, tab->uuid());
		}
	} else {
		this->ui->m_widgets->setCurrentWidget(NULL);
		emit tabChanged(this->m_currentTab->uuid(), "");
		this->m_currentTab = NULL;
	}
}

OpenTab* QTreeTabs::currentTab() const {
	return this->m_currentTab;
}

OpenTab* QTreeTabs::tabFromUuid(const QString &uuid) const
{
	return this->m_map->value(uuid, NULL);
}

QList<OpenTab*>* QTreeTabs::openTabs() const
{
	QList<OpenTab*>* tabs = new QList<OpenTab*>(this->m_map->values());
	return tabs;
}

OpenTab* QTreeTabs::findTabByWidget(QWidget *widget) const
{
	for(QMap<QString, OpenTab*>::iterator it = this->m_map->begin(); it != this->m_map->end(); it++) {
		if (it.value()->widget() == widget) {
			return it.value();
		}
	}

	return NULL;
}

void QTreeTabs::onTextChanged(const QString &text)
{
	emit textChanged(((OpenTab*)this->sender())->uuid(), text);
}

void QTreeTabs::onIconUrlChanged(const QString &url)
{
	emit iconUrlChanged(((OpenTab*)this->sender())->uuid(), url);
}

void QTreeTabs::jsRequestTab()
{
	emit tabRequested();
}

void QTreeTabs::jsRequestClose(const QString& uuid) {
	this->removeItem(uuid);
}

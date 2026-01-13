/*!
 * \file mdiarea.cpp
 * \brief MdiArea file.
 * \author Martial TOLA, CNRS-Lyon, LIRIS UMR 5205
 * \date 2010
 */
#include "Mdiarea.h"

#include "QPainter"
#include "QFileInfo"
#include "QUrl"
#include <QMimeData>

#include "BCGP.h"
#include "GLSLViewer/GLSLViewer.h"

MdiArea::MdiArea(QWidget *parent) : QMdiArea(parent)
{
	bType = bNone;
}

void MdiArea::paintEvent(QPaintEvent *paintEvent)
{
	QMdiArea::paintEvent(paintEvent);

	// and now only paint your image here
	QPainter painter(viewport());
 
	painter.fillRect(paintEvent->rect(), QColor(23, 74, 124));

	int tWidth = paintEvent->rect().width();
	int tHeight = paintEvent->rect().height();

	int imgPxWidth = 800;
	int imgPxHight = 390;

	if (tWidth < imgPxWidth)
	{
		tWidth = imgPxWidth;
	}

	if (tHeight < imgPxHight)
	{
		tHeight = imgPxHight;
	}

	int posx = (tWidth - imgPxWidth) / 2.0;
	int posy = (tHeight - imgPxHight) / 2.0;
	painter.drawImage(QRect(posx, posy, imgPxWidth, imgPxHight), QImage(":/img/Resources/gpbg.png"));
 
	painter.end();
}

void MdiArea::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls())	//hasFormat("text/uri-list")
	{
		event->acceptProposedAction();

// ---------------------------------------------------
#ifdef __linux__
	bType=bLeft;
	if (event->mouseButtons() & Qt::RightButton)
		bType=bRight;
#endif
// ---------------------------------------------------
	}
}

void MdiArea::dropEvent(QDropEvent *event)
{
	int res = 0;
	GLSLViewer* viewer = NULL;
	QList<QUrl> urls = event->mimeData()->urls();

// ---------------------------------------------------
#ifdef __APPLE__
	bType=bLeft;
	if (event->keyboardModifiers() & Qt::MetaModifier)
		bType=bRight;
#endif
#ifdef _MSC_VER
	bType=bLeft;
	if (event->buttons() & Qt::RightButton)
		bType=bRight;
#endif
// ---------------------------------------------------

	for (int i=0; i<urls.size(); i++)
	{
		{
			viewer = static_cast<GLSLViewer* >(m_mw->ActiveMdiChild());
			if (m_mw->ActiveMdiChild() == 0)
				res = m_mw->LoadFile(urls[i].toLocalFile(), viewer);
			else
			{
				if (bType == bLeft)
					res = m_mw->LoadFile(urls[i].toLocalFile(), viewer);
				else if (bType == bRight)
				{
	#ifdef __linux__
					if (event->keyboardModifiers() & Qt::MetaModifier)
	#else
					if (event->modifiers() & Qt::AltModifier)
	#endif
						res = m_mw->AddFile(viewer, urls[i].toLocalFile());
					else
						res = m_mw->AddFile(viewer, urls[i].toLocalFile());
				}
			}

			if (res)
				break;

			//m_mw->writeSettings();
			//m_mw->readSettings();
		}
	}
	if (viewer)
		viewer->update();

	event->acceptProposedAction();
}
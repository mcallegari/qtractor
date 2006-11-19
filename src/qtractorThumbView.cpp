// qtractorThumbView.cpp
//
/****************************************************************************
   Copyright (C) 2005-2006, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorAbout.h"
#include "qtractorThumbView.h"
#include "qtractorTrackView.h"
#include "qtractorSession.h"
#include "qtractorTracks.h"
#include "qtractorClip.h"

#include "qtractorMainForm.h"

#include <QApplication>
#include <QPainter>

#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>

#include <QRubberBand>


//-------------------------------------------------------------------------
// qtractorThumbView -- Session track line thumb view.

// Constructor.
qtractorThumbView::qtractorThumbView( QWidget *pParent )
	: QFrame(pParent)
{
	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);
	QFrame::setMinimumSize(QSize(120, 32));
	QFrame::setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

	m_dragState   = DragNone;
	m_pRubberBand = new QRubberBand(QRubberBand::Rectangle, this);
	QPalette pal(m_pRubberBand->palette());
	pal.setColor(m_pRubberBand->foregroundRole(), Qt::darkGray);
	m_pRubberBand->setPalette(pal);
	m_pRubberBand->setBackgroundRole(QPalette::NoRole);
	m_pRubberBand->show();

	QFrame::setToolTip(tr("Thumb view"));
}


// Update thumb-position.
void qtractorThumbView::updateThumb ( int dx )
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	qtractorTracks *pTracks = pMainForm->tracks();
	if (pTracks == NULL)
		return;

	int w = QFrame::width()  - 2;
	int h = QFrame::height() - 2;

	int f2 = pSession->pixelFromFrame(pSession->sessionLength());
	if (f2 > 0)
		f2 += pSession->pixelFromBeat(2 * pSession->beatsPerBar());
	else
		f2 += pTracks->trackView()->viewport()->width();

	int w2 = (w * pTracks->trackView()->viewport()->width()) / f2;
	if (w2 < 8)
		w2 = 8;
	else
	if (w2 > w)
		w2 = w;

	int x2 = dx + (w * pTracks->trackView()->contentsX()) / f2;
	if (x2 < 1)
		x2 = 1;
	else
	if (x2 > w - w2)
		x2 = w - w2;

	m_pRubberBand->setGeometry(x2, 1, w2, h);
}


// Update view-position.
void qtractorThumbView::updateView ( int dx )
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	qtractorTracks *pTracks = pMainForm->tracks();
	if (pTracks == NULL)
		return;

	int w = QFrame::width() - 2;

	int f2 = pSession->pixelFromFrame(pSession->sessionLength())
		+ pSession->pixelFromBeat(2 * pSession->beatsPerBar());

	int cx = pTracks->trackView()->contentsX() + (dx * f2) / w;
	int cy = pTracks->trackView()->contentsY();

	if (cx < 0)
		cx = 0;

	pTracks->trackView()->setContentsPos(cx, cy);
}


// Session track-line paint method.
void qtractorThumbView::paintEvent ( QPaintEvent *pPaintEvent )
{
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorSession *pSession = pMainForm->session();
	if (pSession == NULL)
		return;

	qtractorTracks *pTracks = pMainForm->tracks();
	if (pTracks == NULL)
		return;

	QPainter painter(this);

	int w = QFrame::width()  - 2;
	int h = QFrame::height() - 2;

	const QPalette& pal = QFrame::palette();
	painter.fillRect(1, 1, w, h, pal.dark().color());

	int x2, w2;
	int f2 = pSession->sessionLength();
	if (f2 > 0)
		f2 += pSession->frameFromBeat(2 * pSession->beatsPerBar());
	else
		f2 += pSession->frameFromPixel(pTracks->trackView()->viewport()->width());
	f2 /= w;
	f2++;

	qtractorTrack *pTrack = pSession->tracks().first();
	if (pTrack) {
		int h2 = ((h - 1) / pSession->tracks().count());
		if (h2 < 1)
			h2 = 1;
		int y2 = 1;
		while (pTrack) {
			painter.setPen(pTrack->foreground());
			painter.setBrush(pTrack->background());
			qtractorClip *pClip = pTrack->clips().first();
			while (pClip) {
				x2 = pClip->clipStart()  / f2;
				w2 = pClip->clipLength() / f2;
				painter.drawRect(x2, y2, w2, h2);
				pClip = pClip->next();
			}
			y2 += h2;
			pTrack = pTrack->next();
		}
	}

	// Draw the loop-bound lines, if any...
	if (pSession->isLooping()) {
		painter.setPen(Qt::darkCyan);
		x2 = int(pSession->loopStart()) / f2;
		if (x2 < w)
			painter.drawLine(x2, 1, x2, h);
		x2 = int(pSession->loopEnd()) / f2;
		if (x2 < w)
			painter.drawLine(x2, 1, x2, h);
	}

	// Finally, daw current edit-bound lines...
	painter.setPen(Qt::blue);
	x2 = int(pSession->editHead()) / f2;
	if (x2 < w)
		painter.drawLine(x2, 1, x2, h);
	x2 = int(pSession->editTail()) / f2;
	if (x2 < w)
		painter.drawLine(x2, 1, x2, h);

	// done for now, QFrame will need right away...
	painter.end();

	// Do it the QFrame way...
	QFrame::paintEvent(pPaintEvent);
}


// Session track-line paint method.
void qtractorThumbView::resizeEvent ( QResizeEvent *pResizeEvent )
{
	QFrame::resizeEvent(pResizeEvent);

	updateThumb();
}


// Handle selection with mouse.
void qtractorThumbView::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	m_dragState = DragNone;

	const QPoint& pos = pMouseEvent->pos();
	if (m_pRubberBand->geometry().contains(pos)) {
		m_dragState = DragStart;
		m_posDrag   = pos;
	}

	QFrame::mousePressEvent(pMouseEvent);
}


void qtractorThumbView::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	const QPoint& pos = pMouseEvent->pos();
	if (m_dragState == DragStart
		&& (pos - m_posDrag).manhattanLength()
			> QApplication::startDragDistance())
		m_dragState = DragMove;

	if (m_dragState == DragMove)
		updateThumb(pos.x() - m_posDrag.x());

	QFrame::mouseMoveEvent(pMouseEvent);
}


void qtractorThumbView::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	const QPoint& pos = pMouseEvent->pos();
	if (m_dragState == DragMove)
		updateView(pos.x() - m_posDrag.x());
	else
	if (m_dragState == DragNone)
		updateView(pos.x() - m_pRubberBand->pos().x());

	m_dragState = DragNone;

	QFrame::mouseReleaseEvent(pMouseEvent);
}


// end of qtractorThumbView.cpp

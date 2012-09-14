/*
    Copyright (C) 2012  Dan Vratil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "qmloutputview.h"
#include "qmloutputcomponent.h"
#include "qmloutput.h"

#include <QDeclarativeEngine>
#include <qdeclarativeexpression.h>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <KStandardDirs>
#include <KDebug>
#include <QGraphicsScene>

#include <kscreen/output.h>

Q_DECLARE_METATYPE(QMLOutput*);

QMLOutputView::QMLOutputView():
	QDeclarativeItem(),
	m_activeOutput(0)
{
}

QMLOutputView::~QMLOutputView()
{
}

QList<QMLOutput*> QMLOutputView::outputs() const
{
	return m_outputs;
}

QMLOutput* QMLOutputView::activeOutput() const
{
	return m_activeOutput;
}


void QMLOutputView::addOutput(QDeclarativeEngine *engine, /*KScreen::*/Output* output)
{
	QMLOutputComponent outputComponent(engine);

	QMLOutput *instance = dynamic_cast<QMLOutput*>(outputComponent.createForOutput(output));
	if (!instance) {
		kWarning() << "Failed to add output" << output->name();
		return;
	}

	instance->setParentItem(this);

	/* Root refers to the root object. We need it in order to set drag range */
	instance->setProperty("viewport", this->property("root"));
	connect(instance, SIGNAL(moved()), this, SLOT(outputMoved()));
	connect(instance, SIGNAL(clicked()), this, SLOT(outputClicked()));
	connect(instance, SIGNAL(changed()), this, SIGNAL(changed()));
	connect(output, SIGNAL(isPrimaryChanged()), SLOT(primaryOutputChanged()));

	m_outputs << instance;
	instance->setProperty("z", m_outputs.count());

	Q_EMIT outputsChanged();
}

QMLOutput* QMLOutputView::getPrimaryOutput() const
{
	Q_FOREACH (QMLOutput *output, m_outputs) {
		if (output->output()->isPrimary()) {
			return output;
		}
	}

	return 0;
}

void QMLOutputView::outputClicked()
{
	for (int i = 0; i < m_outputs.count(); i++) {
		QMLOutput *output = m_outputs.at(i);

		/* Find clicked child and move it above all it's siblings */
		if (output == sender()) {
			for (int j = i + 1; j < m_outputs.count(); j++) {
				int z = m_outputs.at(j)->property("z").toInt();
				m_outputs.at(j)->setProperty("z", z - 1);
				m_outputs.at(j)->setProperty("focus", false);
			}
			output->setProperty("z", m_outputs.length());
			output->setProperty("focus", true);
			m_activeOutput = output;
			emit activeOutputChanged();

			break;
		}

		output->setProperty("focus", false);
	}
}


void QMLOutputView::outputMoved()
{
	QMLOutput *output = dynamic_cast<QMLOutput*>(sender());

	int x = output->x();
	int y = output->y();
	int width =  output->width();
	int height = output->height();

	/* FIXME: The size of the active snapping area should depend on size of
	 * the output */

	Q_FOREACH (QMLOutput *otherOutput, m_outputs) {
		if (otherOutput == output) {
			continue;
		}

		int x2 = otherOutput->x();
		int y2 = otherOutput->y();
		int height2 = otherOutput->height();
		int width2 = otherOutput->width();
		int centerX = x + (width / 2);
		int centerY = y + (height / 2);
		int centerX2 = x2 + (width2 / 2);
		int centerY2 = y2 + (height2 / 2);

		/* @output is left of @otherOutput */
		if ((x + width > x2 - 30) && (x + width < x2 + 30) &&
		    (y + height > y2) && (y < y2 + height2)) {

			output->setX(x2 - width);
			x = output->x();
			centerX = x + (width / 2);

			/* @output is snapped to @otherOutput on left and their
			 * upper sides are aligned */
			if ((x + width == x2) && (y < y2 + 5) && (y > y2 - 5)) {
				output->setY(y2);
				return;
			}

			/* @output is snapped to @otherOutput on left and they
			 * are centered */
			if ((x + width == x2) && (centerY < centerY2 + 5) && (centerY > centerY2 - 5)) {
				output->setY(centerY2 - (height / 2));
				return;
			}

			/* @output is snapped to @otherOutput on left and their
			 * bottom sides are aligned */
			if ((x + width == x2) && (y + height < y2 + height2 + 5) && (y + height > y2 + height2 - 5)) {
				output->setY(y2 + height2 - height);
				return;
			}
		}


		/* @output is right of @otherOutput */
		if ((x > x2 + width2 - 30) && (x < x2 + width2 + 30) &&
		    (y + height > y2) && (y < y2 + height2)) {

			output->setX(x2 + width2);
			x = output->x();
			centerX = x + (width / 2);

			/* @output is snapped to @otherOutput on right and their
			 * upper sides are aligned */
			if ((x == x2 + width2) && (y < y2 + 5) && (y > y2 - 5)) {
				output->setY(y2);
				return;
			}

			/* @output is snapped to @otherOutput on right and they
			 * are centered */
			if ((x == x2 + width2) && (centerY < centerY2 + 5) && (centerY > centerY2 - 5)) {
				output->setY(centerY2 - (height / 2));
				return;
			}

			/* @output is snapped to @otherOutput on right and their
			 * bottom sides are aligned */
			if ((x == x2 + width2) && (y + height < y2 + height2 + 5) && (y + height > y2 + height2 -5)) {
				output->setY(y2 + height2 - height);
				return;
			}
		}


		/* @output is above @otherOutput */
		if ((y + height > y2 - 30) && (y + height < y2 + 30) &&
		    (x + width > x2) && (x < x2 + width2)) {

			output->setY(y2 - height);
			y = output->y();
			centerY = y + (height / 2);

			/* @output is snapped to @otherOutput on top and their
			 * left sides are aligned */
			if ((y + height == y2) && (x < x2 + 5) && (x > x2 - 5)) {
				output->setX(x2);
				return;
			}

			/* @output is snapped to @otherOutput on top and they
			 * are centered */
			if ((y + height == y2) && (centerX < centerX2 + 5) && (centerX > centerX2 - 5)) {
				output->setX(centerX2 - (width / 2));
				return;
			}

			/* @output is snapped to @otherOutput on top and their
			 * right sides are aligned */
			if ((y + height == y2) && (x + width < x2 + width2 + 5) && (x + width > x2 + width2 - 5)) {
				output->setX(x2 + width2 - width);
				return;
			}
		}


		/* @output is below @otherOutput */
		if ((y > y2 + height2 - 30) && (y < y2 + height2 + 30) &&
		    (x + width > x2) && (x < x2 + width2)) {


			output->setY(y2 + height2);
			y = output->y();;

			/* @output is snapped to @otherOutput on bottom and their
			 * left sides are aligned */
			if ((y == y2 + height2) && (x < x2 + 5) && (x > x2 - 5)) {
				output->setX(x2);
				return;
			}

			/* @output is snapped to @otherOutput on bottom and they
			 * are centered */
			if ((y == y2 + height2) && (centerX < centerX2 + 5) && (centerX > centerX2 - 5)) {
				output->setX(centerX2 - (width / 2));
				return;
			}

			/* @output is snapped to @otherOutput on bottom and their
			 * right sides are aligned */
			if ((y == y2 + height2) && (x + width < x2 + width2 + 5) && (x + width > x2 + width2 - 5)) {
				output->setX(x2 + width2 - width);
				return;
			}
		}


		/* @output is centered with @otherOutput */
		if ((centerX > centerX2 - 30) && (centerX < centerX2 + 30) &&
		    (centerY > centerY2 - 30) && (centerY < centerY2 + 30)) {

			output->setY(centerY2 - (height / 2));
 			output->setX(centerX2 - (width / 2));
			return;
		}
	}
}

void QMLOutputView::primaryOutputChanged()
{
	/*KScreen::*/Output *newPrimary = dynamic_cast</*KScreen::*/Output*>(sender());

	/* Unset primary flag on all other outputs */
	Q_FOREACH(QMLOutput *qmlOutput, m_outputs) {
		if (qmlOutput->output() != newPrimary) {
			/* Prevent endless recursion, disconnect this handler before
			 * changing the primary flag */
			disconnect(qmlOutput->output(), SIGNAL(isPrimaryChanged()),
				   this, SLOT(primaryOutputChanged()));

			qmlOutput->output()->setPrimary(false);

			connect(qmlOutput->output(), SIGNAL(isPrimaryChanged()),
				this, SLOT(primaryOutputChanged()));
		}
	}
}


QDeclarativeContext* QMLOutputView::context() const
{
	QList< QGraphicsView* > views;
	QDeclarativeView *view;

	views = scene()->views();
	if (views.count() == 0) {
		kWarning() << "This view is not in any scene!";
		return 0;
	}

	view = dynamic_cast< QDeclarativeView* >(views.at(0));
	return view->rootContext();
}

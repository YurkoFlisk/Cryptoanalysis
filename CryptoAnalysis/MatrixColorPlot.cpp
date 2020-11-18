#include "MatrixColorPlot.h"
#include <QPainter>

MatrixColorPlot::MatrixColorPlot(QWidget* parent)
	: QWidget(parent)
{}

MatrixColorPlot::MatrixColorPlot(QWidget* parent, const Axis& xLabels,
	const QString& xCaption, const Axis& yLabels,
	const QString& yCaption, const Data& data)
	: QWidget(parent), xCaption(xCaption), yCaption(yCaption),
	xLabels(xLabels), yLabels(yLabels), data(data)
{}

MatrixColorPlot::~MatrixColorPlot()
{}

void MatrixColorPlot::paintEvent(QPaintEvent* paintEvent)
{
	QPainter painter(this);
	painter.setBackground(Qt::white);
	painter.setFont(QFont("Arial", 12));

	painter.save();
	painter.translate(CAPTIONRECT_WIDTH / 2, height() / 2);
	painter.rotate(-90);
	painter.drawText(QRect(-height() / 2, -CAPTIONRECT_WIDTH / 2,
		height(), CAPTIONRECT_WIDTH), Qt::AlignCenter, yCaption);
	painter.restore();

	painter.drawText(QRect(0, 0, width(), CAPTIONRECT_WIDTH),
		Qt::AlignCenter, xCaption);
	for (int i = 0; i < yLabels.size(); ++i)
	{
		painter.drawText(QRect(0, 0, LABEL_SIZE, TILE_SIZE).translated(
			YLABELS_START_X, YLABELS_START_Y + i * TILE_SIZE),
			Qt::AlignCenter, yLabels[i]);
	}
	for (int j = 0; j < xLabels.size(); ++j)
	{
		painter.drawText(QRect(0, 0, TILE_SIZE, LABEL_SIZE).translated(
			XLABELS_START_X + j * TILE_SIZE, XLABELS_START_Y),
			Qt::AlignCenter, yLabels[j]);
	}
	for (int i = 0; i < yLabels.size(); ++i)
		for (int j = 0; j < xLabels.size(); ++j)
		{
			const int elem = data[i][j] * 255;
			painter.setBrush(QColor(elem, elem, elem, 255));
			painter.drawRect(QRect(0, 0, TILE_SIZE, TILE_SIZE).translated(
				TILES_START_X + j * TILE_SIZE, TILES_START_Y + i * TILE_SIZE));
		}

	const QSize t = size();
}

QSize MatrixColorPlot::sizeHint() const
{
	return QSize(
		TILES_START_X + xLabels.size() * TILE_SIZE,
		TILES_START_Y + yLabels.size() * TILE_SIZE);
}

QSize MatrixColorPlot::minimumSizeHint() const
{
	return sizeHint();
}

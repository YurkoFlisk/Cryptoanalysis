#pragma once

#include <QFrame>

class MatrixColorPlot : public QWidget
{
	Q_OBJECT

public:
	static constexpr qreal CAPTIONRECT_WIDTH = 30;
	static constexpr qreal LABEL_SIZE = 20;
	static constexpr qreal TILE_SIZE = 20;
	static constexpr qreal XLABELS_START_X = CAPTIONRECT_WIDTH + LABEL_SIZE;
	static constexpr qreal XLABELS_START_Y = CAPTIONRECT_WIDTH;
	static constexpr qreal YLABELS_START_X = CAPTIONRECT_WIDTH;
	static constexpr qreal YLABELS_START_Y = CAPTIONRECT_WIDTH + LABEL_SIZE;
	static constexpr qreal TILES_START_X = CAPTIONRECT_WIDTH + LABEL_SIZE;
	static constexpr qreal TILES_START_Y = CAPTIONRECT_WIDTH + LABEL_SIZE;

	using Axis = std::vector<QString>;
	using Data = std::vector<std::vector<qreal>>;

	void paintEvent(QPaintEvent* paintEvent) override;
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;
	
	MatrixColorPlot(QWidget* parent = nullptr);
	MatrixColorPlot(QWidget* parent, const Axis& xLabels, const QString& xCaption,
		const Axis& yLabels, const QString& yCaption, const Data& data);
	~MatrixColorPlot();

	void setXCaption(const QString& xC) { xCaption = xC; }
	void setYCaption(const QString& yC) { yCaption = yC; }
	void setXLabels(const Axis& xL) { xLabels = xL; }
	void setYLabels(const Axis& yL) { yLabels = yL; }
	void setData(const Data& d) { data = d; }

private:
	QString xCaption;
	QString yCaption;
	Axis xLabels;
	Axis yLabels;
	Data data;
};

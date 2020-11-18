#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <unordered_map>

#include "CryptoAnalysis.h"

CryptoAnalysis::CryptoAnalysis(QWidget *parent)
    : QMainWindow(parent), collator(QLocale(QLocale::Ukrainian, QLocale::Ukraine)),
	caseSensitive(false)
{
    ui.setupUi(this);
	ui.caseSensitiveCheckBox->setChecked(caseSensitive);
	ui.alphabetLE->setText(UKRAINIAN_ALPHABET);

    connect(ui.actionOpenPlaintext, &QAction::triggered,
		[this]() {sFileOpen(false); });
	connect(ui.actionOpenCiphertext, &QAction::triggered,
		[this]() {sFileOpen(true); });
	connect(ui.actionSavePlaintext, &QAction::triggered,
		[this]() {sFileSave(false); });
	connect(ui.actionSaveCiphertext, &QAction::triggered,
		[this]() {sFileSave(true); });
	connect(ui.actionExit, &QAction::triggered, this, &QWidget::close);

	connect(ui.analyzePlaintextPB, &QPushButton::clicked,
		this, &CryptoAnalysis::analyze);
	connect(ui.analyzeCiphertextPB, &QPushButton::clicked,
		this, &CryptoAnalysis::analyze);
	connect(ui.encryptPB, &QPushButton::clicked,
		this, &CryptoAnalysis::encrypt);
	connect(ui.decryptPB, &QPushButton::clicked,
		this, &CryptoAnalysis::decrypt);
}

void CryptoAnalysis::sFileOpen(bool cipherText)
{
	const QString path = QFileDialog::getOpenFileName(this,
		tr("Open file"), "", tr("Text files (*.txt);;All files (*)"));
	if (path.isEmpty())
		return;
	
	QFile file(path);
	file.open(QFile::ReadOnly | QFile::Text);
	if (!file.isOpen())
	{
		QMessageBox::critical(this, tr("Error"), tr("Error opening file"));
		return;
	}

	QTextStream textStream(&file);
	(cipherText ? ui.ciphertextTE : ui.plaintextTE)->
		setPlainText(textStream.readAll());
	file.close();
}

void CryptoAnalysis::sFileSave(bool cipherText)
{
	const QString path = QFileDialog::getSaveFileName(this,
		tr("Save file"), "", tr("Text files (*.txt);;All files (*)"));
	if (path.isEmpty())
		return;

	QFile file(path);
	file.open(QFile::WriteOnly | QFile::Text);
	if (!file.isOpen())
	{
		QMessageBox::critical(this, tr("Error"), tr("Error saving to file"));
		return;
	}

	QTextStream textStream(&file);
	textStream << (cipherText ?
		ui.ciphertextTE : ui.plaintextTE)->toPlainText();
	file.close();
}

void CryptoAnalysis::analyze()
{
	QString text = (sender() == ui.analyzePlaintextPB
		? ui.plaintextTE : ui.ciphertextTE)->toPlainText();
	if (text.isEmpty())
	{
		QMessageBox::warning(this, tr("Invalid operation"),
			tr("Text is empty, nothing to analyze"));
		return;
	}

	refreshAlphabet();
	analyzeChars(text);
	analyzeBigrams(text);
	analyzeTrigrams(text);
}

void CryptoAnalysis::refreshAlphabet()
{
	caseSensitive = ui.caseSensitiveCheckBox->isChecked();
	alphabet = ui.alphabetLE->text();
	charIndex.clear();
	for (int i = 0; i < alphabet.size(); ++i)
	{
		const QChar ch = (caseSensitive ? alphabet[i] : alphabet[i].toLower());
		if (charIndex.contains(ch))
		{
			QMessageBox::warning(this, tr("Invalid operation"), tr(
				"Alphabet contains identical characters. "
				"Note: if 'case sensitive' is false, check that you "
				"don't include same letter in both upper and lower case"));
			return;
		}
		alphabet[i] = ch;
		charIndex[ch] = i;
	}
	// Now alphabet chars have (by convention) lower case if not caseSensitive
	ui.alphabetLE->setText(alphabet);
}

std::vector<int> CryptoAnalysis::getCharCounts(const QString& text)
{
	std::vector<int> charCounts(alphabet.size());
	for (QChar ch : text)
	{
		auto it = charIndex.find(caseSensitive ? ch : ch.toLower());
		if (it != charIndex.end())
			++charCounts[it.value()];
	}
	return charCounts;
}

void CryptoAnalysis::analyzeChars(const QString& text)
{
	auto charCounts = getCharCounts(text);
	const int allChars = std::accumulate(std::begin(charCounts),
		std::end(charCounts), 0);
	FrequencyData charFreqs;
	for (int i = 0; i < alphabet.size(); ++i)
		charFreqs.emplace_back(alphabet[i], (qreal)charCounts[i] / allChars);

	std::sort(std::begin(charFreqs), std::end(charFreqs),
		[this](const auto& l, const auto& r) {
		return collator.compare(l.first, r.first) < 0;
	});
	displayBarChart(ui.charsLexChartView, charFreqs);

	sortByFrequencyAndShrink(charFreqs, charFreqs.size());
	displayBarChart(ui.charsFreqChartView, charFreqs);
	displayTable(ui.charsTableWidget, charFreqs, tr("Character"));
}

void CryptoAnalysis::analyzeBigrams(const QString& text)
{
	if (text.size() < 2)
	{
		QMessageBox::warning(this, tr("Invalid operation"),
			tr("Text is too short and does not contain any bigrams"));
		return;
	}

	QHash<QString, int> bigramCounts;
	int allBigrams = 0;
	QChar c0 = (caseSensitive ? text[0] : text[0].toLower()), c1;
	for (int i = 1; i < text.size(); ++i)
	{
		c1 = (caseSensitive ? text[i] : text[i].toLower());
		if (charIndex.contains(c0) && charIndex.contains(c1))
			++bigramCounts[QString(c0).append(c1)], ++allBigrams;
		c0 = c1;
	}
	FrequencyData bigramFreqs;
	for (const QString& bigram : bigramCounts.keys())
		bigramFreqs.emplace_back(bigram,
			(qreal)bigramCounts[bigram] / allBigrams);

	sortByFrequencyAndShrink(bigramFreqs, 30);
	displayBarChart(ui.bigramsChartView, bigramFreqs);
	displayTable(ui.bigramsTableWidget, bigramFreqs, tr("Bigram"));

	const int maxCount = std::max_element(std::begin(bigramCounts),
		std::end(bigramCounts)).value();
	MatrixColorPlot::Data matrixData(alphabet.size());
	for (int i = 0; i < alphabet.size(); ++i)
	{
		matrixData[i].resize(alphabet.size());
		for (int j = 0; j < alphabet.size(); ++j)
			matrixData[i][j] = (qreal)bigramCounts[
				QString(alphabet[i]) + alphabet[j]] / maxCount;
	}
	MatrixColorPlot::Axis axis(alphabet.size());
	std::transform(std::begin(alphabet), std::end(alphabet),
		std::begin(axis), [](QChar ch) {return QString(ch); });
	ui.bigramsMCP->setXCaption(tr("Second letter"));
	ui.bigramsMCP->setYCaption(tr("First letter"));
	ui.bigramsMCP->setXLabels(axis);
	ui.bigramsMCP->setYLabels(axis);
	ui.bigramsMCP->setData(matrixData);
	ui.bigramsMCP->adjustSize();
}

void CryptoAnalysis::analyzeTrigrams(const QString& text)
{
	if (text.size() < 3)
	{
		QMessageBox::warning(this, tr("Invalid operation"),
			tr("Text is too short and does not contain any trigrams"));
		return;
	}

	QHash<QString, int> trigramCounts;
	int allTrigrams = 0;
	QChar c0 = (caseSensitive ? text[0] : text[0].toLower()),
		  c1 = (caseSensitive ? text[1] : text[1].toLower()), c2;
	for (int i = 2; i < text.size(); ++i)
	{
		c2 = (caseSensitive ? text[i] : text[i].toLower());
		if (charIndex.contains(c0) && charIndex.contains(c1)
			&& charIndex.contains(c2))
			++trigramCounts[QString(c0).append(c1).append(c2)], ++allTrigrams;
		c0 = c1;
		c1 = c2;
	}
	FrequencyData trigramFreqs;
	for (const QString& trigram : trigramCounts.keys())
		trigramFreqs.emplace_back(trigram,
			(qreal)trigramCounts[trigram] / allTrigrams);

	sortByFrequencyAndShrink(trigramFreqs, 30);
	displayBarChart(ui.trigramsChartView, trigramFreqs);
	displayTable(ui.trigramsTableWidget, trigramFreqs, tr("Bigram"));
}

void CryptoAnalysis::encrypt()
{
	refreshAlphabet();

	const int m = alphabet.size();
	int a = ui.aSB->value(), b = ui.bSB->value();
	a %= m, b %= m;
	if (std::gcd(a, m) != 1)
	{
		QMessageBox::warning(this, tr("Invalid operation"), tr(
			"Affine cipher multiplier (A) is not coprime with alphabet size"));
		return;
	}

	QString text = ui.plaintextTE->toPlainText();
	for (QChar& ch : text)
	{
		const bool originalUpper = ch.isUpper();
		QChar newCh = (caseSensitive ? ch : ch.toLower());
		if (charIndex.contains(newCh))
		{
			newCh = alphabet[(a * charIndex[newCh] + b) % m];
			if (!caseSensitive && originalUpper)
				newCh = newCh.toUpper();
			ch = newCh;
		}
	}
	ui.ciphertextTE->setText(text);
}

void CryptoAnalysis::decrypt()
{
	auto twoMaxFreqChars = [this](const QString& str) {
		auto charCounts = getCharCounts(str);
		int maxIdx = -1, preMaxIdx = -1;
		for (int i = 0; i < alphabet.size(); ++i)
			if (maxIdx == -1 || charCounts[i] > charCounts[maxIdx])
				preMaxIdx = maxIdx, maxIdx = i;
			else if (preMaxIdx == -1 || charCounts[i] > charCounts[preMaxIdx])
				preMaxIdx = i;
		return std::pair{ maxIdx, preMaxIdx };
	};

	refreshAlphabet();

	QMessageBox::information(this, tr("Information"), tr(
		"Choose baseline text, frequencies of which will be compared to "
		"frequencies of current ciphertext and used for analysis"));
	const QString path = QFileDialog::getOpenFileName(this,
		tr("Baseline text"), "", tr("Text files (*.txt);;All files (*)"));
	if (path.isEmpty())
		return;
	QFile baselineFile(path);
	baselineFile.open(QFile::ReadOnly | QFile::Text);
	if (!baselineFile.isOpen())
	{
		QMessageBox::critical(this, tr("Error"), tr("Error opening file"));
		return;
	}

	QString baseline = QTextStream(&baselineFile).readAll();
	auto [baselineMax, baseLinePreMax] = twoMaxFreqChars(baseline);
	if (baselineMax == -1 || baseLinePreMax == -1)
	{
		QMessageBox::warning(this, tr("Invalid operation"), tr(
			"Baseline is too short to perform auto-decrypt"));
		return;
	}
	QString ciphertext = ui.ciphertextTE->toPlainText();
	auto [ciphertextMax, ciphertextPreMax] = twoMaxFreqChars(ciphertext);
	if (ciphertextMax == -1 || ciphertextPreMax == -1)
	{
		QMessageBox::warning(this, tr("Invalid operation"), tr(
			"Ciphertext is too short to perform auto-decrypt"));
		return;
	}

	const int m = alphabet.size();
	// We suppose that:
	// a * baselineMax    + b == ciphertextMax    mod m
	// a * baseLinePreMax + b == ciphertextPreMax mod m
	int a, b;
	for (a = 0; a < m; ++a)
		if (std::gcd(a, m) == 1)
		{
			b = ((ciphertextMax - a * baselineMax) % m + m) % m;
			if ((b + a * baseLinePreMax) % m == ciphertextPreMax)
				break;
		}
	if (a == m) // If a != m, there was a break on some valid (a, b) pair
	{
		QMessageBox::information(this, tr("Failure"), tr(
			"Can't perform auto-decrypt"));
		return;
	}
	// Since a is found, gcd(a, m) == 1, so a_inv will be found
	int a_inv = 1;
	for (; (a * a_inv) % m != 1; ++a_inv);
	// Now decrypt according to this
	for (QChar& ch : ciphertext)
	{
		const bool originalUpper = ch.isUpper();
		QChar newCh = (caseSensitive ? ch : ch.toLower());
		if (charIndex.contains(newCh))
		{
			newCh = alphabet[a_inv * (charIndex[newCh] - b + m) % m];
			if (!caseSensitive && originalUpper)
				newCh = newCh.toUpper();
			ch = newCh;
		}
	}
	QMessageBox::information(this, tr("Success"), tr(
		"Successfully decrypted: a = %0, b = %1").arg(a).arg(b));
	ui.aSB->setValue(a);
	ui.bSB->setValue(b);
	ui.plaintextTE->setText(ciphertext);
}

void CryptoAnalysis::sortByFrequencyAndShrink(FrequencyData& data,
	size_t cnt) const
{
	const auto midIter = std::begin(data) + std::min(cnt, data.size());
	std::partial_sort(std::begin(data), midIter, std::end(data),
		[this](const auto& l, const auto& r) {
		return l.second > r.second || l.second == r.second &&
			collator.compare(l.first, r.first) < 0;
	});
	data.erase(midIter, std::end(data));
}

void CryptoAnalysis::displayBarChart(QChartView* chartView,
	const FrequencyData& data)
{
	QChart* chart = new QChart;
	chart->setTheme(QChart::ChartThemeBlueCerulean);
	QBarSeries* series = new QBarSeries;
	QBarSet* set = new QBarSet(tr("Frequencies"));
	set->setLabelFont(QFont("Arial", 20));
	QBarCategoryAxis* charAxis = new QBarCategoryAxis;
	QValueAxis* countAxis = new QValueAxis;

	qreal maxFreq = 0;
	for (const auto& [str, freq] : data)
	{
		if (freq > maxFreq)
			maxFreq = freq;
		charAxis->append(str);
		set->append(freq);
	}

	countAxis->setRange(0, maxFreq);
	series->append(set);
	chart->addSeries(series);
	chart->addAxis(charAxis, Qt::AlignBottom);
	chart->addAxis(countAxis, Qt::AlignLeft);
	countAxis->applyNiceNumbers();

	QChart* oldChart = chartView->chart();
	chartView->setChart(chart);
	delete oldChart;
}

void CryptoAnalysis::displayTable(QTableWidget* table,
	const FrequencyData& data, const QString& dataColumnName)
{
	table->setRowCount(data.size());
	table->setColumnCount(2);
	table->setHorizontalHeaderLabels({ dataColumnName, tr("Frequency") });
	for (int i = 0; i < data.size(); ++i)
	{
		const auto& [str, freq] = data[i];
		table->setItem(i, 0, new QTableWidgetItem(str));
		table->setItem(i, 1, new QTableWidgetItem(QString::number(freq)));
	}
}

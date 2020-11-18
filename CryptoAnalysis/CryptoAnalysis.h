#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CryptoAnalysis.h"

class CryptoAnalysis : public QMainWindow
{
    Q_OBJECT

public:
    using FrequencyData = std::vector<std::pair<QString, qreal>>;
    inline static const QString UKRAINIAN_ALPHABET
        = QString::fromWCharArray(L"אבגד´הו÷זחט³¿יךכלםמןנסעףפץצקרשü‏ÿ");

    CryptoAnalysis(QWidget *parent = Q_NULLPTR);

// public slots:
    void sFileOpen(bool cipherText);
    void sFileSave(bool cipherText);

    void analyze();
    void encrypt();
    void decrypt();
private:
    static void displayBarChart(QChartView* chartView,
        const FrequencyData& data);
    static void displayTable(QTableWidget* chartView,
        const FrequencyData& data, const QString& dataColumnName);

    void refreshAlphabet();
    std::vector<int> getCharCounts(const QString&);
    void analyzeChars(const QString&);
    void analyzeBigrams(const QString&);
    void analyzeTrigrams(const QString&);
    void sortByFrequencyAndShrink(FrequencyData& data, size_t cnt) const;

    Ui::CryptoAnalysisClass ui;
    QCollator collator;
    QString alphabet;
    QHash<QChar, int> charIndex;
    bool caseSensitive;
};

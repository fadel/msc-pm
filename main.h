#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <armadillo>

class Main : public QObject
{
    Q_OBJECT
public:
    static Main *instance()
    {
        static Main *m = 0;
        if (m == 0) {
            m = new Main();
        }

        return m;
    }

    Q_INVOKABLE bool saveData() const
    {
        bool ret = true;
        if (m_subsample.n_elem > 0 && m_indicesSavePath.size() > 0) {
            ret = ret && m_subsample.save(m_subsampleSavePath, arma::raw_ascii);
        }
        if (m_subsampleIndices.n_elem > 0 && m_subsampleSavePath.size() > 0) {
            ret = ret && m_subsampleIndices.save(m_indicesSavePath, arma::raw_ascii);
        }

        return ret;
    }

    Q_INVOKABLE bool loadDataset(const std::string &path) { return m_dataset.load(path, arma::raw_ascii); }

    Q_INVOKABLE void setIndicesSavePath(const std::string &path)   { m_indicesSavePath = path; }
    Q_INVOKABLE void setIndicesSavePath(const QString &path)       { setIndicesSavePath(path.toStdString()); }
    Q_INVOKABLE void setSubsampleSavePath(const std::string &path) { m_subsampleSavePath = path; }
    Q_INVOKABLE void setSubsampleSavePath(const QString &path)     { setSubsampleSavePath(path.toStdString()); }

    arma::mat X() const { return m_dataset.cols(0, m_dataset.n_cols - 2); }
    arma::vec labels() const { return m_dataset.col(m_dataset.n_cols - 1); }

public slots:
    void setSubsampleIndices(const arma::uvec &indices) { m_subsampleIndices = indices; }
    void setSubsample(const arma::mat &subsample) {
        if (subsample.n_cols != 2
            || subsample.n_rows != m_subsampleIndices.n_elem) {
            return;
        }

        m_subsample = subsample;
    }

private:
    Main(QObject *parent = 0) : QObject(parent) {}
    ~Main() {}
    Q_DISABLE_COPY(Main)

    arma::mat m_dataset, m_subsample;
    arma::uvec m_subsampleIndices;
    std::string m_indicesSavePath, m_subsampleSavePath;
};

#endif // MAIN_H

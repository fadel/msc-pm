#ifndef MAIN_H
#define MAIN_H

#include <QObject>
#include <armadillo>

#include "interactionhandler.h"

class Main : public QObject
{
    Q_OBJECT
public:
    static Main *instance() {
        // FIXME: Possibly dangerous
        static Main *m = 0;
        if (m == 0) {
            m = new Main();
        }

        return m;
    }

    Q_INVOKABLE bool saveData() const {
        bool ret = true;
        if (m_cp.n_elem > 0 && m_indicesSavePath.size() > 0) {
            ret = ret && m_cp.save(m_cpSavePath, arma::raw_ascii);
        }
        if (m_cpIndices.n_elem > 0 && m_cpSavePath.size() > 0) {
            ret = ret && m_cpIndices.save(m_indicesSavePath, arma::raw_ascii);
        }

        return ret;
    }

    Q_INVOKABLE bool loadDataset(const std::string &path) { return m_dataset.load(path, arma::raw_ascii); }

    Q_INVOKABLE void setIndicesSavePath(const std::string &path) {
        m_indicesSavePath = path;
    }
    Q_INVOKABLE void setIndicesSavePath(const QString &path) {
        setIndicesSavePath(path.toStdString());
    }
    Q_INVOKABLE void setCPSavePath(const std::string &path) {
        m_cpSavePath = path;
    }
    Q_INVOKABLE void setCPSavePath(const QString &path) {
        setCPSavePath(path.toStdString());
    }

    void setInteractionHandler(InteractionHandler *interactionHandler) {
        m_interactionHandler = interactionHandler;
    }

    Q_INVOKABLE void setTechnique(int technique) {
        if (m_interactionHandler) {
            m_interactionHandler->setTechnique((InteractionHandler::Technique) technique);
        }
    }

    arma::mat X() const { return m_dataset.cols(0, m_dataset.n_cols - 2); }
    arma::vec labels() const { return m_dataset.col(m_dataset.n_cols - 1); }

public slots:
    void setCPIndices(const arma::uvec &indices) { m_cpIndices = indices; }
    void setCP(const arma::mat &cp) {
        if (cp.n_cols != 2
            || cp.n_rows != m_cpIndices.n_elem) {
            return;
        }

        m_cp = cp;
    }

private:
    Main(QObject *parent = 0)
        : QObject(parent)
        , m_interactionHandler(0)
    {}
    ~Main() {}
    Q_DISABLE_COPY(Main)

    arma::mat m_dataset, m_cp;
    arma::uvec m_cpIndices;
    std::string m_indicesSavePath, m_cpSavePath;
    InteractionHandler *m_interactionHandler;
};

#endif // MAIN_H
